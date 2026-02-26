#pragma once
#include <QWidget>
#include <QVector>
#include <QPointF>
#include <QMouseEvent>

class WaveViewWidget : public QWidget {
    Q_OBJECT
public:
    explicit WaveViewWidget(QWidget* parent = nullptr);

    //void setWaveData(const QVector<QPointF>& pts);
    // [修改] 接口改为接收 多条线 (每条线是一个 QVector<QPointF>)
    void setMultiWaveData(const QVector<QVector<QPointF>>& lines);
    // 缩放接口
    void zoomIn();
    void zoomOut();
    void resetView(); // 复位（回到最新数据）
    void clear();

protected:
    void paintEvent(QPaintEvent* e) override;

    // [新增] 鼠标/触摸事件，用于拖拽滑动
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

private:
    //QVector<QPointF> m_pts;
    // [修改] 存储多条线的数据
    QVector<QVector<QPointF>> m_lines;

    // [新增] 视窗控制变量
        int m_displayOffset = 0;   // 从第几个点开始画
        int m_displayCount = 800;  // 当前屏幕画多少个点 (越小波形越宽，即放大)
        bool m_autoScroll = true;  // 是否自动跟随最新数据

        // 拖拽辅助
        int m_lastMouseX = 0;
        bool m_isDragging = false;
};
