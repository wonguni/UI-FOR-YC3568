#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>
#include <termios.h>

class QSocketNotifier;

class SerialService : public QObject
{
    Q_OBJECT
public:
    explicit SerialService(QObject* parent = nullptr);
    ~SerialService();

    void setPortName(const QString& name);
    void setBaudRate(int baud);

    QString portName() const { return m_portName; }
    int baudRate()   const { return m_baud; }

public slots:
    void connectDevice(const QString& name);   // "SIM" or "/dev/ttyUSB5"
    void disconnectDevice();
    void sendCommand(const QString& cmd);

signals:
    void rxText(const QString& s);
    void txText(const QString& s);
    void errorText(const QString& s);
    void connectedChanged(bool ok);

private slots:
    void onReadable();

private:
    bool isSim() const { return m_portName.trimmed().compare("SIM", Qt::CaseInsensitive) == 0; }
    bool openPort();
    void closePort();
    bool applyTermios(int fd);
    static speed_t baudToSpeed(int baud);

private:
    QString m_portName = "SIM";
    int m_baud = 115200;

    int m_fd = -1;
    QSocketNotifier* m_notifier = nullptr;
    QByteArray m_rxBuf;
};
