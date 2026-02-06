#include "waveviewwidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QPalette>
#include <algorithm>

WaveViewWidget::WaveViewWidget(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(260);
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(18, 18, 18));
    setPalette(pal);
}

void WaveViewWidget::setWaveData(const QVector<QPointF>& pts) {
    m_pts = pts;
    update();
}

void WaveViewWidget::clear() {
    m_pts.clear();
    update();
}

void WaveViewWidget::paintEvent(QPaintEvent* e) {
    Q_UNUSED(e);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // 背景网格
    p.setPen(QPen(QColor(60, 60, 60), 1));
    const int step = 40;
    for (int x = 0; x < width(); x += step) p.drawLine(x, 0, x, height());
    for (int y = 0; y < height(); y += step) p.drawLine(0, y, width(), y);

    // 边框
    p.setPen(QPen(QColor(120, 120, 120), 1));
    p.drawRect(rect().adjusted(0, 0, -1, -1));

    if (m_pts.isEmpty()) {
        p.setPen(QColor(200, 200, 200));
        p.drawText(rect(), Qt::AlignCenter, "WaveView (No Data)\nPlaceholder");
        return;
    }

    QRectF r = rect().adjusted(18, 18, -18, -18);
    p.setPen(QPen(QColor(0, 220, 180), 2));

    // 计算缩放
    double minY = m_pts[0].y(), maxY = m_pts[0].y();
    for (const auto& pt : m_pts) {
        minY = std::min(minY, pt.y());
        maxY = std::max(maxY, pt.y());
    }
    if (maxY - minY < 1e-9) maxY = minY + 1.0;

    QPolygonF poly;
    poly.reserve(m_pts.size());
    for (int i = 0; i < m_pts.size(); ++i) {
        double nx = (double)i / (double)(m_pts.size() - 1);
        double ny = (m_pts[i].y() - minY) / (maxY - minY);
        QPointF wpt(r.left() + nx * r.width(), r.bottom() - ny * r.height());
        poly << wpt;
    }
    p.drawPolyline(poly);
}
