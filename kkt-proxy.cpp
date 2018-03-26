#include <QDebug>
#include "kktproxyapplication.h"
#include "exception.h"


#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#  include <QTextCodec>

static class TrInit
{
public:
    TrInit()
    {
        QTextCodec::setCodecForTr(QTextCodec::codecForName("utf-8"));
        QTextCodec::setCodecForCStrings(QTextCodec::codecForName("utf-8"));
        QTextCodec::setCodecForLocale(QTextCodec::codecForName("utf-8"));
    }
} g_TrInit;
#endif

int main(int argc, char *argv[])
{
    KKTProxyApplication app(argc, argv);

    try {
        app.initialize();
    }
    catch (Exception &ex) {
        return -1;
    }
    return app.exec();
}
