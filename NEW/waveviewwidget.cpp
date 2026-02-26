#include "waveviewwidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <algorithm>
#include <cmath>

WaveViewWidget::WaveViewWidget(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(260);
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(20, 20, 20));
    setPalette(pal);
}

void WaveViewWidget::setMultiWaveData(const QVector<QVector<QPointF>>& lines) {
    m_lines = lines;

    // 如果处于自动滚动模式（且有数据），就锁定在最新的数据上
    if (m_autoScroll && !m_lines.isEmpty() && !m_lines[0].isEmpty()) {
        int totalPoints = m_lines[0].size();
        // 让 offset 指向 (总长度 - 当前显示长度)，即显示最后一段
        m_displayOffset = totalPoints - m_displayCount;
        if (m_displayOffset < 0) m_displayOffset = 0;
    }

    update(); // 触发重绘
}

// --- 缩放逻辑 ---
void WaveViewWidget::zoomIn() {
    // 放大：减少屏幕显示的点数 (最小显示 50 个点)
    m_displayCount = std::max(50, (int)(m_displayCount * 0.8));
    m_autoScroll = false; // 用户操作后，停止自动滚动
    update();
}

void WaveViewWidget::zoomOut() {
    // 缩小：增加屏幕显示的点数 (最大显示 10000 个点)
    m_displayCount = std::min(10000, (int)(m_displayCount * 1.25));
    update();
}

void WaveViewWidget::resetView() {
    m_displayCount = 800; // 恢复默认视窗大小
    m_autoScroll = true;  // 恢复自动滚动
    update();
}

// --- 拖拽交互 (滑动) ---
void WaveViewWidget::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_lastMouseX = e->x();
        m_autoScroll = false; // 用户一点屏幕，就停止自动滚动
    }
}

void WaveViewWidget::mouseMoveEvent(QMouseEvent* e) {
    if (m_isDragging && !m_lines.isEmpty() && !m_lines[0].isEmpty()) {
        int dx = e->x() - m_lastMouseX;

        // 计算滑动的灵敏度：屏幕像素差 -> 数据点差
        // 简单算法：dx 像素对应多少个数据点？
        double pointsPerPixel = (double)m_displayCount / (double)width();
        int deltaPoints = (int)(dx * pointsPerPixel);

        // 像手机相册一样：往左滑(dx<0)是看后面的数据(Offset变大)
        // 往右滑(dx>0)是看前面的数据(Offset变小)
        m_displayOffset -= deltaPoints;

        // 边界检查
        int totalPoints = m_lines[0].size();
        if (m_displayOffset < 0) m_displayOffset = 0;
        if (m_displayOffset > totalPoints - m_displayCount)
            m_displayOffset = totalPoints - m_displayCount;

        m_lastMouseX = e->x();
        update();
    }
}

void WaveViewWidget::mouseReleaseEvent(QMouseEvent* e) {
    Q_UNUSED(e);
    m_isDragging = false;
}
void WaveViewWidget::clear() {
    m_lines.clear();      // 清空所有历史数据
    m_displayOffset = 0;  // 重置显示偏移
    m_autoScroll = true;  // 恢复自动滚动
    update();             // 刷新界面
}
// --- 绘图逻辑 ---
void WaveViewWidget::paintEvent(QPaintEvent* e) {
    Q_UNUSED(e);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // 1. 画背景网格
    p.fillRect(rect(), QColor(20, 20, 20));
    p.setPen(QPen(QColor(60, 60, 60), 1, Qt::DotLine));
    const int stepX = width() / 10;
    const int stepY = height() / 6;
    for (int x = 0; x < width(); x += stepX) p.drawLine(x, 0, x, height());
    for (int y = 0; y < height(); y += stepY) p.drawLine(0, y, width(), y);

    // 边框
    p.setPen(QPen(QColor(120, 120, 120), 1));
    p.drawRect(rect().adjusted(0, 0, -1, -1));

    if (m_lines.isEmpty() || m_lines[0].isEmpty()) return;

    // 2. 确定当前显示的数据范围 [startIdx, endIdx]
    int totalPoints = m_lines[0].size();

    // 安全检查，防止 offset 越界
    if (m_displayOffset > totalPoints - m_displayCount)
        m_displayOffset = std::max(0, totalPoints - m_displayCount);
    if (m_displayOffset < 0) m_displayOffset = 0;

    int startIdx = m_displayOffset;
    int endIdx = std::min(totalPoints, startIdx + m_displayCount);

    // 如果当前窗口没有数据，直接返回
    if (startIdx >= endIdx) return;

    // 3. 计算可见区域内的 Y 轴极值 (动态调整 Y 轴)
    double globalMin = 1e9, globalMax = -1e9;
    for (const auto& line : m_lines) {
        for (int i = startIdx; i < endIdx && i < line.size(); ++i) {
            double y = line[i].y();
            if (y < globalMin) globalMin = y;
            if (y > globalMax) globalMax = y;
        }
    }

    if (globalMax - globalMin < 1e-6) { globalMax += 1.0; globalMin -= 1.0; }
    double range = globalMax - globalMin;
    globalMax += range * 0.1; // 上下留白
    globalMin -= range * 0.1;

    QRectF r = rect().adjusted(10, 10, -10, -10);
    QColor lineColors[] = { QColor(0, 255, 255), QColor(255, 255, 0), QColor(255, 0, 255) };

    // 4. 绘制
    for (int k = 0; k < m_lines.size(); ++k) {
        const auto& pts = m_lines[k];
        if (pts.size() <= startIdx) continue;

        QPolygonF poly;
        // 遍历当前视窗内的数据
        for (int i = startIdx; i < endIdx && i < pts.size(); ++i) {
            // X轴：映射 i 在当前视窗 (0 ~ displayCount) 中的位置
            double relativeIndex = (double)(i - startIdx);
            double nx = relativeIndex / (double)(m_displayCount - 1);

            // Y轴映射
            double ny = (pts[i].y() - globalMin) / (globalMax - globalMin);

            QPointF pCanvas(r.left() + nx * r.width(), r.bottom() - ny * r.height());
            poly << pCanvas;
        }
        p.setPen(QPen(lineColors[k % 3], 2));
        p.drawPolyline(poly);
    }

    // 5. 如果处于“非自动滚动”状态（查看历史），在右上角画个提示
    if (!m_autoScroll) {
        p.setPen(Qt::red);
        p.drawText(rect().adjusted(0, 10, -10, 0), Qt::AlignRight | Qt::AlignTop, "History View (Paused)");
    }
}
