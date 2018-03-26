#ifndef KKTPROXYAPPLICATION_H
#define KKTPROXYAPPLICATION_H

#include <QCoreApplication>

class KKT;
class QTcpServer;
class KKTProxyApplication : public QCoreApplication
{
    Q_OBJECT
    KKT *kkt;
    QTcpServer *tcpServer;
public:
    KKTProxyApplication(int &argc, char **argv);
    virtual ~KKTProxyApplication();
    void initialize();
signals:
    
public slots:
    void quit();
    void newRequest();
};

#endif // KKTPROXYAPPLICATION_H
