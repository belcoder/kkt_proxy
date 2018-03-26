#ifndef DEFS_H
#define DEFS_H

// Заголовочные файлы ДТО9
#include "ifptr.h"
#include "dto_errors.h"
#include "dto_const.h"

#define REQUEST_IPPORT          5555

#define EOR                     "EOR"
#define EOR_LEN                 3

// список выполняемых приложением команд
#define REQUEST_CMD_PAYMENT             1
#define REQUEST_CMD_RETURN              2
#define REQUEST_CMD_OPENSESSION         3
#define REQUEST_CMD_CLOSESESSION        4
#define REQUEST_CMD_PRINT               6
#define REQUEST_CMD_GETSTATUS_FULL      7
#define REQUEST_CMD_GETSTATUS_LITE      8
#define REQUEST_CMD_OPENDRAWLER         9


enum PRINT_ITEM_TYPE {
    TEXT_ITEM = 0,
    IMAGE_ITEM
};

#endif // DEFS_H
