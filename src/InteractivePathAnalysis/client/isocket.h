#pragma once

#include <QByteArray>
#include <QObject>

class ISocket : public QObject {
    Q_OBJECT
public:
    virtual bool connect()=0;
    virtual bool isConnected()=0;
    virtual bool write(const QByteArray&)=0;
//    virtual QByteArray waitResponse()=0;

signals:
    void connectedChanged(bool);
    void dataRecieved(QByteArray);
};

