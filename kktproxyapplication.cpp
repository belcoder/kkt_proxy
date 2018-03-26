#include <QDebug>
#include <QCommandLineParser>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include "kktproxyapplication.h"
#include "kkt.h"
#include "exception.h"

KKTProxyApplication::KKTProxyApplication(int &argc, char **argv):
    QCoreApplication(argc, argv)
{
    tcpServer = new QTcpServer();
    kkt = KKT::getInstance();

    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(newRequest()));
}

KKTProxyApplication::~KKTProxyApplication()
{
    delete tcpServer;
    delete kkt;
}

void KKTProxyApplication::initialize()
{

    QCommandLineParser parser;
    QCommandLineOption addrOption(QStringList() << "addr", "KKT tcp/ip address", "addr", "192.168.0.1");
    QCommandLineOption portOption(QStringList() << "port", "KKT tcp/ip port", "port", "5555");

    parser.addOption(addrOption);
    parser.addOption(portOption);

    parser.process(*this);

    try {
        /* инициализация экземпляра драйвера ККТ */
        kkt->initialize(parser.value("addr"), parser.value("port").toInt());
        /* инициализация tcp-сервера принимающего запросы на печать */
        if (!tcpServer->listen(QHostAddress::LocalHost, REQUEST_IPPORT))
            throw Exception(QObject::trUtf8("Ошибка инициализации сокета"));
    }
    catch (Exception &ex) {
        qDebug() << ex.what();
        throw ex;
    }
}

void KKTProxyApplication::quit()
{
    kkt->release();
    QCoreApplication::quit();
}

void parseChequeItems(const QJsonObject data, const QString &key, QList<ChequeItem*> &items)
{
    if (!data.contains(key))
        return;

    QJsonArray goods = data.value(key).toArray();

    for (QJsonArray::iterator iter = goods.begin(); iter != goods.end(); iter++) {
        QJsonObject goodsItem = (*iter).toObject();
        ChequeItem *item = new ChequeItem();
        item->setName(goodsItem.value("name").toString());
        item->setPrice(goodsItem.value("price").toDouble());
        item->setQuantity(goodsItem.value("quantity").toDouble());
        item->setSumm(goodsItem.value("summ").toDouble(item->quantity() * item->price()));
        item->setTaxNumber(goodsItem.value("taxNumber").toInt(3));
        item->setEnableCheckSumm(goodsItem.value("enableCheckSumm").toInt(0));
        items.append(item);
    }
}

void clearChequeItems(QList<ChequeItem*> &items)
{
    for (QList<ChequeItem*>::iterator iter = items.begin(); iter != items.end(); iter++) {
        delete (*iter);
    }
}

void parsePrintItems(const QJsonObject &data, const QString &key, QList<PrintItem*> &items)
{
    if (!data.contains(key))
        return;

    QJsonArray printItems = data.value(key).toArray();

    for(QJsonArray::iterator iter = printItems.begin(); iter != printItems.end(); iter++) {
        QJsonObject printItem = (*iter).toObject();
        int itemType = printItem.value("type").toInt();
        switch(itemType) {
            case TEXT_ITEM: {
                TextItem *item = new TextItem();
                item->setType(TEXT_ITEM);
                item->setText(printItem.value("text").toString("stub"));
                item->setAlignment(printItem.value("align").toInt(0));
                item->setTextWrap(printItem.value("wrap").toInt(0));
                item->setPrintPurpose(printItem.value("purpose").toInt(0));
                item->setReceiptFont(printItem.value("receiptFont").toInt(0));
                item->setReceiptFontHeight(printItem.value("receiptFontHeight").toInt(0));
                item->setReceiptLineSpacing(printItem.value("receiptLineSpacing").toInt(0));
                item->setReceiptBrightness(printItem.value("receiptBrightness").toInt(0));
                item->setJournalFont(printItem.value("journalFont").toInt(0));
                item->setJournalFontHeight(printItem.value("journalFontHeight").toInt(0));
                item->setJournalLineSpacing(printItem.value("journalLineSpacing").toInt(0));
                item->setJournalBrightness(printItem.value("journalBrightness").toInt(0));
                items.append(item);
                break;
            }
            case IMAGE_ITEM: {
                ImageItem *item = new ImageItem();
                item->setType(IMAGE_ITEM);
                item->setFilename(printItem.value("filename").toString("stub"));
                item->setScale(printItem.value("scale").toDouble(100));
                item->setAlignment(printItem.value("align").toInt(0));
                item->setLeftMargin(printItem.value("leftMargin").toInt(0));
                items.append(item);
                break;
            }
        }
    }
}

void clearPrintItems(QList<PrintItem*> &items)
{
    for (QList<PrintItem*>::iterator iter = items.begin(); iter != items.end(); iter++) {
        delete (*iter);
    }
}

void KKTProxyApplication::newRequest()
{
    QTcpSocket *client = tcpServer->nextPendingConnection();

    qDebug() << "New incoming connection";

    // чтение запроса в формате json
    QByteArray requestData;
    bool requestReaded = false;

    while (!requestReaded) {
        if (client->waitForReadyRead()) {
            // читатем все, что имеется в сокете
            requestData.append(client->readAll());
            if (requestData.contains(EOR))
                requestReaded = true;
        }
        else {
            client->close();
            client->deleteLater();
            return;
        }
    }

    QJsonDocument requestDocument;
    QJsonParseError error;
    QJsonDocument resultDocument;
    QJsonObject result;

    // перед преобразованием строки запроса в JSON нужно удалить последний символ - маркер конца запроса
    requestData.remove(requestData.indexOf(EOR), EOR_LEN);
    requestDocument = QJsonDocument::fromJson(requestData, &error);

    // формат запроса не соответствует JSON
    if (requestDocument.isNull()) {
        result["status"] = false;
        result["error"] = error.errorString();
        resultDocument.setObject(result);
        client->write(resultDocument.toJson(QJsonDocument::Compact));
        client->close();
        return;
    }

    int requestCmd = requestDocument.object().value("cmd").toInt();

    try {
        kkt->enableDevice(true);
        kkt->reset();
        qDebug() << "Processing command: " << requestCmd;
        switch (requestCmd) {
            case REQUEST_CMD_PAYMENT:
            case REQUEST_CMD_RETURN: {
                QJsonObject data = requestDocument.object().value("data").toObject();
                QList<ChequeItem*> goods;
                QList<PrintItem*> preHeader;
                QList<PrintItem*> header;
                QList<PrintItem*> footer;

                int type = data.value("type").toInt();

                parseChequeItems(data, "goods", goods);
                parsePrintItems(data, "preheader", preHeader);
                parsePrintItems(data, "header", header);
                parsePrintItems(data, "footer", footer);

                if (goods.size() == 0)
                    throw Exception(trUtf8("Товары не найдены"));

                qDebug() << "Printing preheader";
                kkt->printItems(preHeader, false, false);
                qDebug() << "Opening cheque";
                if (requestCmd == REQUEST_CMD_PAYMENT)
                    kkt->openCheque(TED::Fptr::ChequeSell);
                else if (requestCmd == REQUEST_CMD_RETURN)
                    kkt->openCheque(TED::Fptr::ChequeSellReturn);
                qDebug() << "Printing header";
                kkt->printItems(header, false, false);
                qDebug() << "Registering cheque items";
                if (requestCmd == REQUEST_CMD_PAYMENT)
                    kkt->registerChequeItems(goods, TED::Fptr::RegistrationSell);
                else if (requestCmd == REQUEST_CMD_RETURN)
                    kkt->registerChequeItems(goods, TED::Fptr::RegistrationSellReturn);
                qDebug() << "Printing footer";
                kkt->printItems(footer, false, false);
                qDebug() << "Payment";
                if (requestCmd == REQUEST_CMD_PAYMENT) {
                    double summ = data.value("summ").toDouble();
                    kkt->payment(summ, type);
                }
                else if (requestCmd == REQUEST_CMD_RETURN)
                    kkt->ret(type);
                qDebug() << "Clearing data";
                clearChequeItems(goods);
                clearPrintItems(preHeader);
                clearPrintItems(header);
                clearPrintItems(footer);
                result["status"] = true;
                break;
            }
            case REQUEST_CMD_PRINT: {
                QJsonObject data = requestDocument.object().value("data").toObject();
                bool header = data.value("header").toBool();
                bool footer = data.value("footer").toBool();

                QList<PrintItem*> printItems;

                parsePrintItems(data, "items", printItems);

                if (printItems.size() == 0)
                    throw Exception(trUtf8("Нет ни одного элемента для печати"));

                qDebug() << "New printing request";
                kkt->printItems(printItems, header, footer);
                kkt->partialCut();
                qDebug() << "Clearing data";
                clearPrintItems(printItems);
                result["status"] = true;
                break;
            }
            case REQUEST_CMD_GETSTATUS_FULL: {
                qDebug() << "New getstatus_full request";
                KKTStatus status;
                kkt->getStatus(status);
                result["summPointPosition"] = status.summPointPosition;
                result["checkState"] = status.checkState;
                result["checkNumber"] = status.checkNumber;
                result["docNumber"] = status.docNumber;
                result["charLineLength"] = status.charLineLength;
                result["pixelLineLength"] = status.pixelLineLength;
                result["rcpCharLineLength"] = status.rcpCharLineLength;
                result["rcpPixelLineLength"] = status.rcpPixelLineLength;
                result["jrnCharLineLength"] = status.jrnCharLineLength;
                result["jrnPixelLineLength"] = status.jrnPixelLineLength;
                result["slipCharLineLength"] = status.slipCharLineLength;
                result["slipPixelLineLength"] = status.slipPixelLineLength;
                result["serialNumber"] = QString::fromWCharArray(status.serialNumber);
                result["session"] = status.session;
                result["day"] = status.day;
                result["month"] = status.month;
                result["year"] = status.year;
                result["hour"] = status.hour;
                result["min"] = status.min;
                result["sec"] = status.sec;
                result["operator"] = status.oper;
                result["logicalNumber"] = status.logicalNumber;
                result["sessionOpened"] = status.sessionOpened;
                result["fiscal"] = status.fiscal;
                result["drawerOpened"] = status.drawerOpened;
                result["coverOpened"] = status.coverOpened;
                result["checkPaperPresent"] = status.checkPaperPresent;
                result["controlPaperPresent"] = status.controlPaperPresent;
                result["model"] = status.model;
                result["mode"] = status.mode;
                result["advancedMode"] = status.advancedMode;
                result["slotNumber"] = status.slotNumber;
                result["summ"] = status.summ;
                result["fnFiscal"] = status.fnFiscal;
                result["outOfPaper"] = status.outOfPaper;
                result["printerConnectionFailed"] = status.printerConnectionFailed;
                result["printerMechanismError"] = status.printerMechanismError;
                result["printerCutMechanismError"] = status.printerCutMechanismError;
                result["printerOverheatError"] = status.printerOverheatError;
                result["verHi"] = QString::fromWCharArray(status.verHi);
                result["verLo"] = QString::fromWCharArray(status.verLo);
                result["build"] = QString::fromWCharArray(status.build);
                result["deviceDescription"] = QString::fromWCharArray(status.deviceDescription);
                result["inn"] = QString::fromWCharArray(status.inn);
                result["machineNumber"] = QString::fromWCharArray(status.machineNumber);
                result["batteryLow"] = status.batteryLow;
                result["status"] = true;
                break;
            }
            case REQUEST_CMD_GETSTATUS_LITE: {
                qDebug() << "New getstatus_lite request";
                KKTStatus status;
                kkt->getStatus(status);
                result["checkNumber"] = status.checkNumber;
                result["serialNumber"] = QString::fromWCharArray(status.serialNumber);
                result["session"] = status.session;
                result["day"] = status.day;
                result["month"] = status.month;
                result["year"] = status.year;
                result["hour"] = status.hour;
                result["min"] = status.min;
                result["sec"] = status.sec;
                result["logicalNumber"] = status.logicalNumber;
                result["sessionOpened"] = status.sessionOpened;
                result["fiscal"] = status.fiscal;
                result["drawerOpened"] = status.drawerOpened;
                result["coverOpened"] = status.coverOpened;
                result["checkPaperPresent"] = status.checkPaperPresent;
                result["controlPaperPresent"] = status.controlPaperPresent;
                result["model"] = status.model;
                result["mode"] = status.mode;
                result["slotNumber"] = status.slotNumber;
                result["inn"] = QString::fromWCharArray(status.inn);
                result["machineNumber"] = QString::fromWCharArray(status.machineNumber);
                result["batteryLow"] = status.batteryLow;
                result["status"] = true;
                break;
            }
            case REQUEST_CMD_OPENSESSION: {
                QJsonObject data = requestDocument.object().value("data").toObject();
                QString caption = data.value("caption").toString();

                qDebug() << "New opensession request";
                kkt->openSession(caption);
                result["status"] = true;
                break;
            }
            case REQUEST_CMD_CLOSESESSION: {
                qDebug() << "New closesession request";
                kkt->reportZ();
                result["status"] = true;
                break;
            }
            case REQUEST_CMD_OPENDRAWLER: {
                qDebug() << "New opendrawler request";
                kkt->openDrawler();
                result["status"] = true;
                break;
            }
            default:
                throw Exception(trUtf8("Неверный код команды"));
                break;
        }
        qDebug() << "Command has processed";
    }
    catch(Exception ex) {
        qDebug() << "Error: " << ex.what();
        result["status"] = false;
        result["error"] = ex.what();
    }
    // деактивация ККТ
    kkt->enableDevice(false);
    // формируем ответ клиенту в формате json
    resultDocument.setObject(result);
    // отправка ответа
    client->write(resultDocument.toJson(QJsonDocument::Compact));
    client->waitForBytesWritten();
    delete client;
}
