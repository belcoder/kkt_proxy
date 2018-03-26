#ifndef GOODSITEM_H
#define GOODSITEM_H

#include <QObject>
#include <QString>

class ChequeItem : public QObject
{
    QString _name;
    double _price;
    double _quantity;
    double _summ;
    int _taxNumber;
    int _enableCheckSumm;

    Q_OBJECT
public:
    ChequeItem();
public:
    const QString name() const { return _name; }
    void setName(const QString &value) { _name = value; }
    double price() const { return _price; }
    void setPrice(double value) { _price = value; }
    double quantity() const { return _quantity; }
    void setQuantity(double value) { _quantity = value; }
    double summ() const { return _summ; }
    void setSumm(double value) { _summ = value; }
    int taxNumber() { return _taxNumber; }
    void setTaxNumber(int value) { _taxNumber = value; }
    int enableCheckSumm() { return _enableCheckSumm; }
    void setEnableCheckSumm(int value) { _enableCheckSumm = value; }
};

#endif // GOODSITEM_H
