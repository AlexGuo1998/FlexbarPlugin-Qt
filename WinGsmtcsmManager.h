#pragma once


#include <QtCore/QObject>
#include <QtCore/QThread>


class WinGsmtcsmManager final : public QObject {
    Q_OBJECT

public:
    WinGsmtcsmManager();
    ~WinGsmtcsmManager();

private:
    class Worker;

    QThread workerThread{this};
    Worker *worker;
};
