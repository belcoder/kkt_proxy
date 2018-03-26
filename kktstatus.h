#ifndef KKTSTATUS_H
#define KKTSTATUS_H

#include <QString>

struct KKTStatus
{
    int summPointPosition;
    int checkState;
    int checkNumber;
    int docNumber;
    int charLineLength;
    int pixelLineLength;
    int rcpCharLineLength;
    int rcpPixelLineLength;
    int jrnCharLineLength;
    int jrnPixelLineLength;
    int slipCharLineLength;
    int slipPixelLineLength;
    wchar_t serialNumber[50];
    int session;
    int day, month, year;
    int hour, min, sec;
    int oper;
    int logicalNumber;
    int sessionOpened;
    int fiscal;
    int drawerOpened;
    int coverOpened;
    int checkPaperPresent;
    int controlPaperPresent;
    int model;
    int mode;
    int advancedMode;
    int slotNumber;
    double summ;
    int fnFiscal;
    int outOfPaper;
    int printerConnectionFailed;
    int printerMechanismError;
    int printerCutMechanismError;
    int printerOverheatError;
    wchar_t verHi[30];
    wchar_t verLo[30];
    wchar_t build[30];
    wchar_t deviceDescription[255];
    wchar_t inn[30];
    wchar_t machineNumber[50];
    int batteryLow;

    KKTStatus() {}
};
#endif // KKTSTATUS_H
