#ifndef KKT_H
#define KKT_H

#include <QObject>
#include <QList>

#include "defs.h"
#include "chequeitem.h"
#include "printitem.h"
#include "kktstatus.h"

class KKT : public QObject
{
    Q_OBJECT
private:
    TED::Fptr::IFptr *ifptr;
    CreateFptrInterfacePtr createPtr;
    ReleaseFptrInterfacePtr releasePtr;
    static KKT *instance;
protected:
    explicit KKT(QObject *parent = 0);
    void newDocument();
public:
    static KKT *getInstance();
    // начальная инициализация драйвера
    void initialize(const QString &addr, int port);
    void enableDevice(bool enable);
    // освобождение ресурсов, связанных с драйвером
    void release();
    // проверка ошибок и вывод ошибки на консоль
    void checkError(const QString &message = "");
    //
    void reset();
    //
    void setMode(TED::Fptr::Mode mode);
    // получает какой-то Z-отчет
    void reportZ();
    //
    void getStatus(KKTStatus &status);
    // открыть новый чек
    void openCheque(TED::Fptr::ChequeType type);
    // отмена чека
    void cancelCheck();
    // регистрация товарных позиций в чеке
    void registerChequeItems(QList<ChequeItem*> items, TED::Fptr::RegistrationType registrationType);
    // оплата чека
    void payment(double summ, int paymentType);
    // возврат продажи
    void ret(int paymentType);
    // печать текста
    void printItems(QList<PrintItem*> items, bool header, bool footer);
    // частичное отрезание ленты
    void partialCut();
    // открыть денежный ящик
    void openDrawler();
    // открыть/закрыть сессию
    void openSession(const QString &caption);
signals:
    
public slots:
    
};

#endif // KKT_H
