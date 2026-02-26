#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "waveviewwidget.h"
#include "serialservice.h"

#include <QVBoxLayout>
#include <QDateTime>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <cmath>
#include <QInputDialog>
#include <QSettings>


static void logLine(QPlainTextEdit* log, const QString& s)
{
    if (!log) return;
    log->appendPlainText(s.trimmed());
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    applyDarkStyle();

    // 1) 把 WaveViewWidget 放进 ui 的 waveHost（不需要改 .ui）
    {
        auto* host = ui->waveHost;                 // .ui 里这个名字存在
        auto* lay = host->layout();
        if (!lay) {
            auto* v = new QVBoxLayout(host);
            v->setContentsMargins(0,0,0,0);
            v->setSpacing(0);
            lay = v;
        }
        m_wave = new WaveViewWidget(host);
        lay->addWidget(m_wave);
    }

    // 2) 串口服务（先用模拟）
    m_serial = new SerialService(this);

//    connect(m_serial, &SerialService::rxText, this, [=](const QString& s){
//            if (ui->editLog) ui->editLog->appendPlainText(s);

//            // 解析格式: "[RX] 2.00,4.00,2.00"
//            if (s.contains("[RX]")) {
//                int startIdx = s.indexOf(']') + 1;
//                QString content = s.mid(startIdx).trimmed();
//                QStringList parts = content.split(',');

//                // 必须确保解析出 3 个数
//                if (parts.size() >= 3) {
//                    double v1 = parts[0].toDouble();
//                    double v2 = parts[1].toDouble();
//                    double v3 = parts[2].toDouble();

//                    // 增加 X 轴索引
//                    m_waveIndex++;

//                    // 限制最大点数 (例如800点，也就是屏幕宽度)
//                    const int MAX_PTS = 800;

//                    // 辅助 lambda：往 buffer 里加点，并保持长度
//                    auto addPoint = [&](QVector<QPointF>& buf, double val) {
//                        buf.append(QPointF(m_waveIndex, val));
//                        if (buf.size() > MAX_PTS) buf.removeFirst();
//                    };

//                    addPoint(m_waveBuf1, v1);
//                    addPoint(m_waveBuf2, v2);
//                    addPoint(m_waveBuf3, v3);

//                    // [关键] 打包这三条线，传给控件
//                    QVector<QVector<QPointF>> allLines;
//                    allLines << m_waveBuf1 << m_waveBuf2 << m_waveBuf3;

//                    if (m_wave) m_wave->setMultiWaveData(allLines);
//                }
//            }
//        });
    // ... 你的其他初始化代码 ...

        // 1. 修改数据接收逻辑：允许数据无限增长，或者设置一个很大的上限
    connect(m_serial, &SerialService::rxText, this, [=](const QString& s){
            // 建议注释掉日志打印以提升性能，特别是在数据量大时
             if (ui->editLog) ui->editLog->appendPlainText(s);

            if (s.contains("[RX]")) {
                int startIdx = s.indexOf(']') + 1;
                QString content = s.mid(startIdx).trimmed();
                QStringList parts = content.split(',');

                // 只有当成功解析出3个数据时，才进入处理
                if (parts.size() >= 3) {
                    // 1. 在这里定义 v1, v2, v3
                    double v1 = parts[0].toDouble();
                    double v2 = parts[1].toDouble();
                    double v3 = parts[2].toDouble();

                    m_waveIndex++;
                    const int MAX_HISTORY = 50000;

                    // 2. 定义 lambda 工具函数
                    auto addPoint = [&](QVector<QPointF>& buf, double val) {
                        buf.append(QPointF(m_waveIndex, val));
                        if (buf.size() > MAX_HISTORY) buf.removeFirst();
                    };

                    // 3. 【关键】在同一个花括号内调用它
                    addPoint(m_waveBuf1, v1);
                    addPoint(m_waveBuf2, v2);
                    addPoint(m_waveBuf3, v3);

                    // 4. 更新 UI
                    QVector<QVector<QPointF>> allLines;
                    allLines << m_waveBuf1 << m_waveBuf2 << m_waveBuf3;
                    if (m_wave) m_wave->setMultiWaveData(allLines);
                }
            }
        });

        // 2. 连接右侧按钮 (ZoomIn, ZoomOut, Reset)
        // 假设你在 .ui 文件里的按钮名字分别是 toolZoomIn, toolZoomOut, toolReset

        connect(ui->toolZoomIn, &QToolButton::clicked, this, [=]{
            if (m_wave) m_wave->zoomIn();
        });

        connect(ui->toolZoomOut, &QToolButton::clicked, this, [=]{
            if (m_wave) m_wave->zoomOut();
        });

        connect(ui->toolReset, &QToolButton::clicked, this, [=]{
            if (m_wave) m_wave->resetView();
        });

    connect(m_serial, &SerialService::txText, this, [=](const QString& s){
        if (ui->editLog) ui->editLog->appendPlainText(s);
    });
    connect(m_serial, &SerialService::errorText, this, [=](const QString& s){
        if (ui->editLog) ui->editLog->appendPlainText(s);
    });
    connect(m_serial, &SerialService::connectedChanged, this, [=](bool ok){
        if (ui->editLog) ui->editLog->appendPlainText(ok ? "[INFO] CONNECTED" : "[INFO] DISCONNECTED");
    });

promptSerialConfigAndConnect();
    //m_serial->connectDevice("/dev/ttyUSB4");

    logLine(ui->editLog, "[INFO] UI started.");

    // 3) 绑定按钮：开始/停止/导出
    connect(ui->btnStart, &QPushButton::clicked, this, [=]{
        ui->labelFaultTypeV->setText("单相接地（示例）");
        ui->labelOccurTimeV->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
        ui->labelLocationV->setText("K12+300m（示例）");
        ui->labelDurationV->setText("0.8s（示例）");

        generateFakeWave();
        logLine(ui->editLog, "[INFO] Start monitoring.");
    });

    connect(ui->btnStop, &QPushButton::clicked, this, [=]{
        ui->labelFaultTypeV->setText("—");
        ui->labelOccurTimeV->setText("—");
        ui->labelLocationV->setText("—");
        ui->labelDurationV->setText("—");

        if (m_wave) m_wave->clear();
        logLine(ui->editLog, "[INFO] Stop monitoring.");
    });

    connect(ui->btnExport, &QPushButton::clicked, this, [=]{
        const QString path = QFileDialog::getSaveFileName(this, "导出数据", "wave.csv", "CSV Files (*.csv)");
        if (path.isEmpty()) return;

        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            logLine(ui->editLog, "[ERROR] Export failed: cannot open file.");
            return;
        }

        QTextStream out(&f);
        out << "index,value\n";
        for (int i = 0; i < 800; ++i) {
            double y = std::sin(i * 0.03) + 0.2 * std::sin(i * 0.2);
            out << i << "," << y << "\n";
        }
        f.close();
        logLine(ui->editLog, QString("[INFO] Exported: %1").arg(path));
    });

    // 4) 命令发送（btnSend / editCmd）
    connect(ui->btnSend, &QPushButton::clicked, this, [=]{
        const QString cmd = ui->editCmd->text().trimmed();
        if (cmd.isEmpty()) return;
        m_serial->sendCommand(cmd);
        ui->editCmd->clear();
    });
    connect(ui->editCmd, &QLineEdit::returnPressed, ui->btnSend, &QPushButton::click);

    // 快捷命令按钮
    connect(ui->btnReadStatus, &QPushButton::clicked, this, [=]{ ui->editCmd->setText("READ_STATUS"); ui->btnSend->click(); });
    connect(ui->btnQueryParam, &QPushButton::clicked, this, [=]{ ui->editCmd->setText("QUERY_PARAM"); ui->btnSend->click(); });
    connect(ui->btnStartCollect,&QPushButton::clicked, this, [=]{ ui->editCmd->setText("START_COLLECT"); ui->btnSend->click(); });
    connect(ui->btnStopCollect, &QPushButton::clicked, this, [=]{ ui->editCmd->setText("STOP_COLLECT"); ui->btnSend->click(); });

    // 右侧工具按钮：先写日志占位
    connect(ui->toolZoomIn,  &QToolButton::clicked, this, [=]{ logLine(ui->editLog, "[UI] Zoom In"); });
    connect(ui->toolZoomOut, &QToolButton::clicked, this, [=]{ logLine(ui->editLog, "[UI] Zoom Out"); });
    connect(ui->toolReset,   &QToolButton::clicked, this, [=]{ logLine(ui->editLog, "[UI] Reset"); });
    connect(ui->toolCursor,  &QToolButton::clicked, this, [=]{ logLine(ui->editLog, "[UI] Cursor"); });

    // 启动就显示一份一致的内容（可选）
    ui->btnStart->click();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::applyDarkStyle()
{
    // 用 .ui 的标题也行；你也可以保留这句覆盖
    setWindowTitle("故障定位终端 - 界面原型（MVP）");
    resize(1200, 720);

    setStyleSheet(R"(
        QMainWindow { background: #141414; }
        QWidget { font-size: 14px; }
        QStatusBar { background: #1b1b1b; color: #dcdcdc; }
        QLabel { color: #dcdcdc; }
        QPlainTextEdit { background:#0f0f0f; color:#eaeaea; }
        QPushButton {
            background: #2b2b2b;
            color: #eaeaea;
            border: 1px solid #3a3a3a;
            border-radius: 6px;
            padding: 6px 10px;
        }
        QPushButton:hover { background: #333333; }
        QToolButton { background:#2b2b2b; color:#eaeaea; border: 1px solid #3a3a3a; border-radius: 6px; padding: 6px; }
        QToolButton:hover { background:#333333; }
    )");
}


void MainWindow::generateFakeWave()
{
    if (!m_wave) return;

    QVector<QPointF> pts;
    pts.reserve(800);

    // 生成一条正弦波数据
    for (int i = 0; i < 800; ++i) {
        double y = std::sin(i * 0.03) + 0.2 * std::sin(i * 0.2);
        pts.push_back(QPointF(i, y));
    }

    // --- 修改开始 ---
    // 新代码: 构造成多条线的格式 (这里只有一条线)
    QVector<QVector<QPointF>> allLines;
    allLines.append(pts);

    m_wave->setMultiWaveData(allLines);
    // --- 修改结束 ---
}
void MainWindow::promptSerialConfigAndConnect()
{
    QSettings st("yctek", "FaultTerminalUI");

    QString lastPort = st.value("serial/port", "/dev/ttyUSB4").toString();
    int lastBaud = st.value("serial/baud", 115200).toInt();

    bool ok = false;

    // 你也可以改成更常见的默认：/dev/ttyS1 或 /dev/ttyUSB0
    QString port = QInputDialog::getText(
        this,
        "串口设置",
        "串口设备（例如 /dev/ttyUSB5）：",
        QLineEdit::Normal,
        lastPort,
        &ok
    );
    if (!ok || port.trimmed().isEmpty()) {
        if (ui->editLog) ui->editLog->appendPlainText("[INFO] 串口设置已取消，未连接串口。");
        return;
    }

    int baud = QInputDialog::getInt(
        this,
        "串口设置",
        "波特率：",
        lastBaud,
        1200,
        4000000,
        1,
        &ok
    );
    if (!ok) {
        if (ui->editLog) ui->editLog->appendPlainText("[INFO] 波特率设置已取消，未连接串口。");
        return;
    }

    port = port.trimmed();

    // 保存配置（下次启动默认填这个）
    st.setValue("serial/port", port);
    st.setValue("serial/baud", baud);

    // 如果你的 SerialService 支持 setBaudRate / connectDevice，这样写：
    m_serial->setBaudRate(baud);

    // 如果之前已连接，最好先断开再连（如果你有 disconnectDevice 就调用）
    // m_serial->disconnectDevice();

    m_serial->connectDevice(port);

    if (ui->editLog) {
        ui->editLog->appendPlainText(QString("[CFG] port=%1, baud=%2").arg(port).arg(baud));
    }
}



