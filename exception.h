#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <exception>
#include <QString>

class Exception : public std::exception
{
public:
    Exception(const QString &msg)
        : m_message(msg)
    {
    }

    virtual ~Exception() throw()
    {
    }

    virtual const char* what() const throw()
    {
        static QByteArray rawMessage;

        rawMessage.clear();
        rawMessage = m_message.toUtf8();
        return rawMessage.data();
    }

private:
    QString m_message;
};

#endif // EXCEPTION_H
