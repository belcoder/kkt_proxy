#ifndef PRINTITEM_H
#define PRINTITEM_H

#include <QString>
#include "defs.h"

class PrintItem
{
protected:
    PRINT_ITEM_TYPE _type;
public:
    PrintItem();
    PRINT_ITEM_TYPE type() { return _type; }
    void setType(PRINT_ITEM_TYPE value) { _type = value; }
};


class TextItem : public PrintItem {
    QString _text;
    int _alignment;
    int _wrap;
    int _purpose;
    int _receiptFont;
    int _receiptFontHeight;
    int _receiptLineSpacing;
    int _receiptBrightness;
    int _journalFont;
    int _journalFontHeight;
    int _journalLineSpacing;
    int _journalBrightness;
public:
    TextItem() {
        _text = "";
        _alignment = 0;
        _wrap = 0;
        _purpose = 0;
        _receiptFont = 0;
        _receiptFontHeight = 0;
        _receiptLineSpacing = 0;
        _receiptBrightness = 0;
        _journalFont = 0;
        _journalFontHeight = 0;
        _journalLineSpacing = 0;
        _journalBrightness = 0;
    }
    QString text() { return _text; }
    void setText(const QString &value) { _text = value; }
    int alignment() { return _alignment; }
    void setAlignment(int value) { _alignment = value; }
    int textWrap() { return _wrap; }
    void setTextWrap(int value) { _wrap = value; }
    int printPurpose() { return _purpose; }
    void setPrintPurpose(int value) { _purpose = value; }
    int receiptFont() { return _receiptFont; }
    void setReceiptFont(int value) { _receiptFont = value; }
    int receiptFontHeight() { return _receiptFontHeight; }
    void setReceiptFontHeight(int value) { _receiptFontHeight = value; }
    int receiptLineSpacing() { return _receiptLineSpacing; }
    void setReceiptLineSpacing(int value) { _receiptLineSpacing = value; }
    int receiptBrightness() { return _receiptBrightness; }
    void setReceiptBrightness(int value) { _receiptBrightness = value; }
    int journalFont() { return _journalFont; }
    void setJournalFont(int value) { _journalFont  = value; }
    int journalFontHeight() { return _journalFontHeight; }
    void setJournalFontHeight(int value) { _journalFontHeight = value; }
    int journalLineSpacing() { return _journalLineSpacing; }
    void setJournalLineSpacing(int value) { _journalLineSpacing = value; }
    int journalBrightness() { return _journalBrightness; }
    void setJournalBrightness(int value) { _journalBrightness = value; }
};

class ImageItem : public PrintItem {
    QString _filename;
    double _scale;
    int _leftMargin;
    int _alignment;
public:
    ImageItem() {
        _filename = "";
        _scale = 100;
        _leftMargin = 0;
        _alignment = 0;
    }
    QString filename() { return _filename; }
    void setFilename(const QString &value) { _filename = value; }
    double scale() { return _scale; }
    void setScale(double value) { _scale = value; }
    int leftMargin() { return _leftMargin; }
    void setLeftMargin(int value) { _leftMargin = value; }
    int alignment() { return _alignment; }
    void setAlignment(int value) { _alignment = value; }
};

#endif // PRINTITEM_H
