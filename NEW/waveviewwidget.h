#pragma once
#include <QWidget>
#include <QVector>
#include <QPointF>

class WaveViewWidget : public QWidget {
    Q_OBJECT
public:
    explicit WaveViewWidget(QWidget* parent = nullptr);

    void setWaveData(const QVector<QPointF>& pts);
    void clear();

protected:
    void paintEvent(QPaintEvent* e) override;

private:
    QVector<QPointF> m_pts;
};
