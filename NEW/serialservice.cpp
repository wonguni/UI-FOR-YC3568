#include "serialservice.h"

#include <QSocketNotifier>
#include <QTimer>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <termios.h>

SerialService::SerialService(QObject* parent)
    : QObject(parent)
{
}

SerialService::~SerialService()
{
    closePort();
}

void SerialService::setPortName(const QString& name)
{
    m_portName = name.trimmed();
}

void SerialService::setBaudRate(int baud)
{
    m_baud = baud;
}

void SerialService::connectDevice(const QString& name)
{
    setPortName(name);

    if (isSim()) {
        emit connectedChanged(true);
        emit rxText("[SIM] Device connected.");
        return;
    }

    closePort();

    if (!openPort()) {
        emit connectedChanged(false);
        return;
    }

    emit connectedChanged(true);
    emit rxText(QString("[INFO] Connected: %1 @ %2").arg(m_portName).arg(m_baud));
}

void SerialService::disconnectDevice()
{
    closePort();
    emit connectedChanged(false);
    emit rxText("[INFO] Disconnected.");
}

//void SerialService::sendCommand(const QString& cmd)
//{
//    const QString s = cmd.trimmed();
//    if (s.isEmpty()) return;

//    emit txText(QString("[TX] %1").arg(s));

//    if (isSim()) {
//        QTimer::singleShot(50, this, [=]{
//            emit rxText(QString("[RX] OK (%1)").arg(s));
//        });
//        return;
//    }

//    if (m_fd < 0) {
//        emit errorText("[ERR] Serial not connected.");
//        return;
//    }

//    QByteArray out = s.toUtf8();
//    out.append('\n');

//    const ssize_t n = ::write(m_fd, out.constData(), (size_t)out.size());
//    if (n < 0) {
//        emit errorText(QString("[ERR] write failed: %1").arg(QString::fromLocal8Bit(strerror(errno))));
//    }
//}
void SerialService::sendCommand(const QString& cmd)
{
    if (m_fd < 0) { emit errorText("[ERR] port not open"); return; }

    QByteArray data = cmd.toUtf8();

    // ✅ 很多设备需要回车/换行作为结束符；XShell 常常会自动加
    if (!data.endsWith('\n') && !data.endsWith('\r')) {
        data.append("\r");  // 不确定就先用 \r\n
        // 如果设备只认 \r，把上面改成 data.append("\r");
    }

    // ✅ 确保写完整
    const char* p = data.constData();
    int left = data.size();
    while (left > 0) {
        ssize_t n = ::write(m_fd, p, left);
        if (n < 0) {
            if (errno == EINTR) continue;
            emit errorText(QString("[ERR] write failed: %1").arg(strerror(errno)));
            return;
        }
        p += n;
        left -= int(n);
    }

    ::tcdrain(m_fd); // 等待发送完成
    emit txText(QString("[TXHEX] %1").arg(QString(data.toHex(' '))));
}


bool SerialService::openPort()
{
    m_fd = ::open(m_portName.toUtf8().constData(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (m_fd < 0) {
        emit errorText(QString("[ERR] Open %1 failed: %2").arg(m_portName, QString::fromLocal8Bit(strerror(errno))));
        return false;
    }

    if (!applyTermios(m_fd)) {
        emit errorText(QString("[ERR] Configure %1 failed.").arg(m_portName));
        closePort();
        return false;
    }

    int flags = fcntl(m_fd, F_GETFL, 0);
    if (flags >= 0) fcntl(m_fd, F_SETFL, flags & ~O_NONBLOCK);

    m_notifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &SerialService::onReadable);

    return true;
}

void SerialService::closePort()
{
    if (m_notifier) {
        m_notifier->setEnabled(false);
        m_notifier->deleteLater();
        m_notifier = nullptr;
    }
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
    m_rxBuf.clear();
}

speed_t SerialService::baudToSpeed(int baud)
{
    switch (baud) {
    case 9600: return B9600;
    case 19200: return B19200;
    case 38400: return B38400;
    case 57600: return B57600;
    case 115200: return B115200;
#ifdef B230400
    case 230400: return B230400;
#endif
#ifdef B460800
    case 460800: return B460800;
#endif
#ifdef B921600
    case 921600: return B921600;
#endif
    default: return B115200;
    }
}

bool SerialService::applyTermios(int fd)
{
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        return false;
    }

    cfmakeraw(&tty);

    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag |= CREAD | CLOCAL;

#ifdef CRTSCTS
    tty.c_cflag &= ~CRTSCTS;
#endif
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);

    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 1;

    speed_t spd = baudToSpeed(m_baud);
    cfsetispeed(&tty, spd);
    cfsetospeed(&tty, spd);

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        return false;
    }

    tcflush(fd, TCIOFLUSH);
    return true;
}

void SerialService::onReadable()
{
    if (m_fd < 0) return;

    char buf[512];
    const ssize_t n = ::read(m_fd, buf, sizeof(buf));
    if (n <= 0) return;

    m_rxBuf.append(buf, (int)n);

    while (true) {
        int idx = m_rxBuf.indexOf('\n');
        if (idx < 0) break;

        QByteArray line = m_rxBuf.left(idx);
        m_rxBuf.remove(0, idx + 1);

        emit rxText(QString("[RX] %1").arg(QString::fromUtf8(line).trimmed()));
    }
}
