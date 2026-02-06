#pragma once
#include <QWidget>

class QLabel;

class FaultInfoWidget : public QWidget {
    Q_OBJECT
public:
    explicit FaultInfoWidget(QWidget* parent = nullptr);

    void setFaultType(const QString& t);
    void setOccurTime(const QString& t);
    void setLocation(const QString& t);
    void setDuration(const QString& t);

private:
    QLabel* m_faultType{};
    QLabel* m_occurTime{};
    QLabel* m_location{};
    QLabel* m_duration{};
};
