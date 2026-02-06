#include "faultinfowidget.h"
#include <QGridLayout>
#include <QLabel>

static QLabel* makeValueLabel() {
    auto* l = new QLabel("-");
    l->setStyleSheet("QLabel { color: #eaeaea; font-weight: 600; }");
    return l;
}

FaultInfoWidget::FaultInfoWidget(QWidget* parent) : QWidget(parent) {
    setStyleSheet("QLabel { color: #cfcfcf; }");

    auto* layout = new QGridLayout(this);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setHorizontalSpacing(18);
    layout->setVerticalSpacing(6);

    auto* t1 = new QLabel("故障类型:");
    auto* t2 = new QLabel("发生时间:");
    auto* t3 = new QLabel("故障位置:");
    auto* t4 = new QLabel("故障持续:");

    m_faultType = makeValueLabel();
    m_occurTime = makeValueLabel();
    m_location  = makeValueLabel();
    m_duration  = makeValueLabel();

    layout->addWidget(t1, 0, 0); layout->addWidget(m_faultType, 0, 1);
    layout->addWidget(t2, 0, 2); layout->addWidget(m_occurTime, 0, 3);
    layout->addWidget(t3, 1, 0); layout->addWidget(m_location,  1, 1);
    layout->addWidget(t4, 1, 2); layout->addWidget(m_duration,  1, 3);

    layout->setColumnStretch(1, 1);
    layout->setColumnStretch(3, 1);
}

void FaultInfoWidget::setFaultType(const QString& t) { m_faultType->setText(t); }
void FaultInfoWidget::setOccurTime(const QString& t) { m_occurTime->setText(t); }
void FaultInfoWidget::setLocation(const QString& t)  { m_location->setText(t); }
void FaultInfoWidget::setDuration(const QString& t)  { m_duration->setText(t); }
