#pragma once


#include <QtCore/QObject>
#include <QtCore/QThread>
// #include <QtCore/QMutex>
#include <QtCore/QSharedPointer>


class WinGsmtcsmManager final : public QObject {
    Q_OBJECT

public:
    Q_SIGNAL void infoUpdated(QString info, QImage thumbnail);

    WinGsmtcsmManager();

    ~WinGsmtcsmManager();

private:
    class Worker;

    QThread *workerThread;
    QSharedPointer<Worker> worker;
};
