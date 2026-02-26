#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QPointF>
#include <QTimer>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class WaveViewWidget;
class FaultInfoWidget;
class ConsoleDock;
class SerialService;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void applyDarkStyle();
    void buildUi();
    void generateFakeWave();

    //QVector<QPointF> m_wavePts;
    //int m_waveIndex = 0;
    // [修改] 不再使用单个 m_wavePts，改用三个 (或者用数组)
    QVector<QPointF> m_waveBuf1;
    QVector<QPointF> m_waveBuf2;
    QVector<QPointF> m_waveBuf3;

    int m_waveIndex = 0; // X轴计数器
    bool m_waveDirty = false;
    QTimer* m_waveTimer = nullptr;
    void promptSerialConfigAndConnect();


private:
    Ui::MainWindow *ui;

    WaveViewWidget*  m_wave{nullptr};
    FaultInfoWidget* m_fault{nullptr};
    ConsoleDock*     m_console{nullptr};
    SerialService*   m_serial{nullptr};
};

#endif // MAINWINDOW_H
