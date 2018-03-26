#include <QDebug>
#include <QLibrary>
#include <QVector>
#include <QFileInfo>
#include "kkt.h"
#include "exception.h"

KKT* KKT::instance = NULL;

KKT::KKT(QObject *parent) :
    QObject(parent)
{
}

KKT* KKT::getInstance()
{
    if (instance == NULL)
        instance = new KKT();
    return instance;
}

void KKT::initialize(const QString &addr, int port)
{
#if defined(Q_OS_LINUX)
    QLibrary lib("libs/linux-x64/libfptr.so");
    if(!lib.load())
    {
        lib.setFileName("libs/linux-x86/libfptr.so");
        lib.load();
    }
#elif defined(Q_OS_WIN32)
    QLibrary lib("libs/nt-x86-mingw/fptr.dll");
    lib.load();
#else
#   error "Unknown OS"
#endif

    if (!lib.isLoaded())
        throw Exception(QObject::trUtf8("Не удалось загрузить библиотеку fptr - [%1]").arg(lib.errorString()));

    createPtr = (CreateFptrInterfacePtr) lib.resolve("CreateFptrInterface");
    releasePtr = (ReleaseFptrInterfacePtr) lib.resolve("ReleaseFptrInterface");

    // создаем экземпляр драйвера
    ifptr = createPtr(DTO_IFPTR_VER1);
    if(!ifptr)
    {
        throw Exception(QObject::trUtf8("Ошибка при инициализации драйвера ККТ"));
    }

    // Выставляем рабочий каталог. В нем дККМ будет искать требуемые ему библиотеки.
    QFileInfo finfo(lib.fileName());
    if(ifptr->put_DeviceSingleSetting(S_SEARCHDIR, finfo.absolutePath().toStdWString().c_str()) < 0)
        checkError();
    if(ifptr->ApplySingleSettings() < 0)
        checkError();

    if(ifptr->put_DeviceSingleSetting(S_PORT, SV_PORT_TCPIP) < 0)
        checkError();
    // адрес TCPIP
    wchar_t buff[255];
    if (addr.length() > 254)
        throw Exception(trUtf8("IP адрес слишком длинный"));
    addr.toWCharArray(buff);
    if (ifptr->put_DeviceSingleSetting(S_IPADDRESS, buff) < 0)
        checkError();
    // TCPIP порт
    if (ifptr->put_DeviceSingleSetting(S_IPPORT, port) < 0)
        checkError();
    // Версия протокола
    if(ifptr->put_DeviceSingleSetting(S_PROTOCOL, TED::Fptr::ProtocolAtol30) < 0)
        checkError();
    // Модель ККТ
    if(ifptr->put_DeviceSingleSetting(S_MODEL, TED::Fptr::ModelATOL22F) < 0)
        checkError();
    // применение настроект
    if(ifptr->ApplySingleSettings() < 0)
        checkError();
}

void KKT::enableDevice(bool enable)
{
    try {
        // активируем/деактивируем ККТ
        if (ifptr->put_DeviceEnabled(enable ? 1 : 0) < 0)
            checkError(trUtf8("Не могу активировать ККТ"));
    }
    catch (const Exception &ex) {
        // если ошибка произошла при деактивации ККТ - ирнорировать
        if (enable) throw;
        else qDebug() << "Не могу деактивировать ККТ";
    }
}

void KKT::release()
{
    ifptr->ResetMode();
    releasePtr(&ifptr);
}

void KKT::checkError(const QString &message)
{
    if(!ifptr)
        throw Exception("Invalid interface");

    int rc = EC_OK;
    ifptr->get_ResultCode(&rc);
    if(rc < 0)
    {
        QString resultDescription, badParamDescription;
        QVector<wchar_t> v(256);
        int size = ifptr->get_ResultDescription(&v[0], v.size());
        if (size <= 0)
            throw Exception("get_ResultDescription error");
        if (size > v.size())
        {
            v.clear();
            v.resize(size + 1);
            ifptr->get_ResultDescription(&v[0], v.size());
        }
        resultDescription = QString::fromWCharArray(&v[0]);
        if(rc == EC_INVALID_PARAM) {
            QVector<wchar_t> v(256);
            int size = ifptr->get_BadParamDescription(&v[0], v.size());
            if (size <= 0)
                throw Exception("get_BadParamDescription error");
            if (size > v.size())
            {
                v.clear();
                v.resize(size + 1);
                ifptr->get_ResultDescription(&v[0], v.size());
            }
            badParamDescription = QString::fromWCharArray(&v[0]);
        }
        if(badParamDescription.isEmpty())
            throw Exception(QObject::trUtf8("%1 ([%2] %3)")
                            .arg(message)
                            .arg(rc)
                            .arg(resultDescription));
        else
            throw Exception(QObject::trUtf8("%1 ([%2] %3 (%4))")
                            .arg(message)
                            .arg(rc)
                            .arg(resultDescription)
                            .arg(badParamDescription));
    }
}

void KKT::cancelCheck()
{
    try {
        if (ifptr->CancelCheck() < 0)
            checkError(trUtf8("Не могу отменить предыдущий чек"));
    }
    catch (const Exception &ex) {
        int rc = EC_OK;
        ifptr->get_ResultCode(&rc);
        if (rc != EC_INVALID_MODE && rc != EC_3801)
            throw;
    }
}

void KKT::setMode(TED::Fptr::Mode mode)
{
    if (ifptr->put_Mode(mode) < 0)
        checkError(trUtf8("Не могу задать режим: %1").arg(mode));
    if (ifptr->SetMode() < 0)
        checkError(trUtf8("Не могу установить режим: %1").arg(mode));
}

void KKT::reset()
{
    // пытаемся отменить чек
    cancelCheck();
    // задаем режим выбора - начальный режим
    setMode(TED::Fptr::ModeSelect);
}

void KKT::getStatus(KKTStatus &status)
{
    if(ifptr->GetStatus() < 0)
        checkError(trUtf8("Не могу получить статус ККТ"));
    // заполнение структуры
    ifptr->get_SummPointPosition(&status.summPointPosition);
    ifptr->get_CheckState(&status.checkState);
    ifptr->get_CheckNumber(&status.checkNumber);
    ifptr->get_DocNumber(&status.docNumber);
    ifptr->get_CharLineLength(&status.charLineLength);
    ifptr->get_PixelLineLength(&status.pixelLineLength);
    ifptr->get_RcpCharLineLength(&status.rcpCharLineLength);
    ifptr->get_RcpPixelLineLength(&status.rcpPixelLineLength);
    ifptr->get_JrnCharLineLength(&status.jrnCharLineLength);
    ifptr->get_JrnPixelLineLength(&status.jrnPixelLineLength);
    ifptr->get_SlipCharLineLength(&status.slipCharLineLength);
    ifptr->get_SlipPixelLineLength(&status.slipPixelLineLength);
    ifptr->get_SerialNumber(status.serialNumber, 50);
    ifptr->get_Session(&status.session);
    ifptr->get_Date(&status.day, &status.month, &status.year);
    ifptr->get_Time(&status.hour, &status.min, &status.sec);
    ifptr->get_Operator(&status.oper);
    ifptr->get_LogicalNumber(&status.logicalNumber);
    ifptr->get_SessionOpened(&status.sessionOpened);
    ifptr->get_Fiscal(&status.fiscal);
    ifptr->get_DrawerOpened(&status.drawerOpened);
    ifptr->get_CoverOpened(&status.coverOpened);
    ifptr->get_CheckPaperPresent(&status.checkPaperPresent);
    ifptr->get_ControlPaperPresent(&status.controlPaperPresent);
    ifptr->get_Model(&status.model);
    ifptr->get_Mode(&status.mode);
    ifptr->get_AdvancedMode(&status.advancedMode);
    ifptr->get_SlotNumber(&status.slotNumber);
    ifptr->get_Summ(&status.summ);
    ifptr->get_FNFiscal(&status.fnFiscal);
    ifptr->get_OutOfPaper(&status.outOfPaper);
    ifptr->get_PrinterConnectionFailed(&status.printerConnectionFailed);
    ifptr->get_PrinterCutMechanismError(&status.printerCutMechanismError);
    ifptr->get_PrinterMechanismError(&status.printerMechanismError);
    ifptr->get_PrinterOverheatError(&status.printerOverheatError);
    ifptr->get_VerHi(status.verHi, 30);
    ifptr->get_VerLo(status.verLo, 30);
    ifptr->get_Build(status.build, 30);
    ifptr->get_DeviceDescription(status.deviceDescription, 255);
    ifptr->get_INN(status.inn, 30);
    ifptr->get_MachineNumber(status.machineNumber, 50);
}

void KKT::reportZ()
{
    setMode(TED::Fptr::ModeReportClear);
    if(ifptr->put_ReportType(TED::Fptr::ReportZ))
        checkError(trUtf8("Не могу задать тип Z-отчета"));
    if(ifptr->Report() < 0)
        checkError(trUtf8("Не могу получить Z-отчет"));
}

void KKT::newDocument()
{
    if (ifptr->put_Mode(TED::Fptr::ModeRegistration) < 0)
        checkError(trUtf8("Не могу задать режим регистрации"));
    if (ifptr->NewDocument() < 0)
        checkError(trUtf8("Не могу выполнить NewDocument"));
}

void KKT::openCheque(TED::Fptr::ChequeType type)
{
    try {
        newDocument();
    }
    catch (Exception ex) {
        int rc = EC_OK;
        ifptr->get_ResultCode(&rc);
        // проверяем ошибку превышения смены
        if (rc == EC_3822) {
            // если ошибка именно в этом, нужно получить Z-отчет
            reportZ();
            // повторно пытаемся создать новый чек
            newDocument();
        }
        else throw;
    }
    /* 2. Открытие чека */
    if (ifptr->put_CheckType(type) < 0)
        checkError(trUtf8("Не могу задать требуемый тип чека"));
    if (ifptr->OpenCheck() < 0)
        checkError(trUtf8("Не могу открыть новый чек"));
}

void KKT::registerChequeItems(QList<ChequeItem*> items, TED::Fptr::RegistrationType registrationType)
{
    wchar_t buff[255];

    for (int nI = 0; nI < items.size(); nI++) {
        memset(buff, 0, sizeof(wchar_t) * 255);

        if (items.at(nI)->name().length() > 254)
            throw Exception(trUtf8("Название товара слишком длинное"));

        items.at(nI)->name().toWCharArray(buff);

        if (ifptr->put_Name(buff) < 0)
            checkError(trUtf8("Не могу задать название товара"));
        if (ifptr->put_PositionSum(items.at(nI)->summ()) < 0)
            checkError(trUtf8("Не могу задать стоимость позиции"));
        if (ifptr->put_Price(items.at(nI)->price()) < 0)
            checkError(trUtf8("Не могу задать цену товара"));
        if (ifptr->put_Quantity(items.at(nI)->quantity()) < 0)
            checkError(trUtf8("Не могу задать кол-во товара"));
        if (ifptr->put_Department(0) < 0)
            checkError(trUtf8("Не могу задать отдел"));
        if (ifptr->put_TaxNumber(items.at(nI)->taxNumber()))
            checkError(trUtf8("Не могу задать номер налога на товар"));
        if (ifptr->put_EnableCheckSumm(items.at(nI)->enableCheckSumm()))
            checkError(trUtf8("Не могу задать проверку наличия денег в ККТ для выплаты"));
        switch (registrationType) {
            case TED::Fptr::RegistrationSell:
                if (ifptr->Registration() < 0)
                    checkError(trUtf8("Не могу зарегистрировать продажу в чеке"));
                break;
            case TED::Fptr::RegistrationSellReturn:
                if (ifptr->Return() < 0)
                    checkError(trUtf8("Не могу зарегистрировать возврат продажи в чеке"));
                break;
            default:
                break;
        }
    }
}

void KKT::payment(double summ, int paymentType)
{
    if (ifptr->put_Summ(summ) < 0)
        checkError(trUtf8("Не могу задать сумму покупки по чеку"));
    if (ifptr->put_TypeClose(paymentType) < 0)
        checkError(trUtf8("Не могу задать тип отплаты"));
    if (ifptr->Payment() < 0)
        checkError(trUtf8("Не могу зарегистрировать платеж"));
    if (ifptr->CloseCheck() < 0)
        checkError(trUtf8("Не могу закрыть чек"));
}

void KKT::ret(int paymentType)
{
    if (ifptr->put_TypeClose(paymentType) < 0)
        checkError(trUtf8("Не могу задать тип отплаты"));
    if (ifptr->CloseCheck() < 0)
        checkError(trUtf8("Не могу закрыть чек"));
}

void KKT::printItems(QList<PrintItem *> items, bool header, bool footer)
{
    if (header) {
        if (ifptr->PrintHeader() < 0)
            checkError(trUtf8("Не могу напечатать шапку"));
    }
    wchar_t buff[255];
    for (int nI = 0; nI < items.size(); nI++) {
        switch (items.at(nI)->type()) {
            case TEXT_ITEM: {
                TextItem *textItem = (TextItem*)items.at(nI);
                memset(buff, 0, sizeof(wchar_t) * 255);
                if (textItem->text().length() > 254)
                    throw Exception(trUtf8("Слишком длинная строка"));
                textItem->text().toWCharArray(buff);
                if (ifptr->put_Caption(buff) < 0)
                    checkError(trUtf8("Не могу задать текст для печати"));
                if (ifptr->put_Alignment(textItem->alignment()) < 0)
                    checkError(trUtf8("Не могу задать выравнивание для печати"));
                if (ifptr->put_TextWrap(textItem->textWrap()))
                    checkError(trUtf8("Не могу задать перенос слов"));
                if (ifptr->put_PrintPurpose(textItem->printPurpose()) < 0)
                    checkError(trUtf8("Не могу задать назначение печати"));
                if (ifptr->put_ReceiptFont(textItem->receiptFont()) < 0)
                    checkError(trUtf8("Не могу задать шрифт печати на ЧЛ"));
                if (ifptr->put_ReceiptFontHeight(textItem->receiptFontHeight()) < 0)
                    checkError(trUtf8("Не могу задать высоту шрифта печати на ЧЛ"));
                if (ifptr->put_ReceiptLinespacing(textItem->receiptLineSpacing()) < 0)
                    checkError(trUtf8("Не могу задать величину межстрочного интервала печати на ЧЛ"));
                if (ifptr->put_ReceiptBrightness(textItem->receiptBrightness()) < 0)
                    checkError(trUtf8("Не могу задать яркость шрифта печати на ЧЛ"));
                if (ifptr->put_JournalFont(textItem->journalFont()) < 0)
                    checkError(trUtf8("Не могу задать шрифт печати на КЛ"));
                if (ifptr->put_JournalFontHeight(textItem->journalFontHeight()) < 0)
                    checkError(trUtf8("Не могу задать высоту шрифта печати на КЛ"));
                if (ifptr->put_JournalLinespacing(textItem->journalLineSpacing()) < 0)
                    checkError(trUtf8("Не могу задать величину межстрочного интервала печати на КЛ"));
                if (ifptr->put_JournalBrightness(textItem->journalBrightness()) < 0)
                    checkError(trUtf8("Не могу задать яркость шрифта печати на КЛ"));
                if (ifptr->PrintFormattedText() < 0)
                    checkError(trUtf8("Не могу напечатать требуемый текст"));
                break;
            }
            case IMAGE_ITEM: {
                ImageItem *imageItem = (ImageItem*)items.at(nI);
                memset(buff, 0, sizeof(wchar_t) * 255);
                if (imageItem->filename().length() > 254)
                    throw Exception(trUtf8("Слишком длинный путь к файлу"));
                imageItem->filename().toWCharArray(buff);
                if (ifptr->put_FileName(buff) < 0)
                    checkError(trUtf8("Не могу задать имя файла картинки"));
                if (ifptr->put_Scale(imageItem->scale()) < 0)
                    checkError(trUtf8("Не могу задать масштаб картики"));
                if (ifptr->put_LeftMargin(imageItem->leftMargin()) < 0)
                    checkError(trUtf8("Не могу задать левый отступ"));
                if (ifptr->put_Alignment(imageItem->alignment()))
                    checkError(trUtf8("Не могу задать выравнимниваени для картинки"));
                if (ifptr->PrintPicture() < 0)
                    checkError(trUtf8("Не могу напечатать картинку"));
                break;
            }
            default:
                break;
        }
    }
    if (footer) {
        if (ifptr->PrintFooter() < 0)
            checkError(trUtf8("Не могу напечатать подвал"));
    }
}

void KKT::partialCut() {
    if (ifptr->PartialCut() < 0)
        checkError(trUtf8("Не могу отрезать ленту"));
}

void KKT::openDrawler()
{
    if (ifptr->OpenDrawer() < 0)
        checkError(trUtf8("Не могу открыть денежный ящик"));
}

void KKT::openSession(const QString &caption)
{
    if (!caption.isEmpty()) {
        wchar_t buff[255];

        memset(buff, 0, sizeof(wchar_t) * 255);

        if (caption.length() > 254)
            throw Exception(trUtf8("Строка caption слишком длинная"));

        caption.toWCharArray(buff);

        if (ifptr->put_Caption(buff) < 0)
            checkError(trUtf8("Не могу задать строку caption"));
    }
    // переход в режим регистрации (нужно для открытии сессии)
    setMode(TED::Fptr::ModeRegistration);
    // открытие сессии
    if (ifptr->OpenSession())
        checkError(trUtf8("Не могу открыть сессию"));
}
