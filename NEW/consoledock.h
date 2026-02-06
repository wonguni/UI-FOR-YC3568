#pragma once
#include <QDockWidget>

class QPlainTextEdit;
class QLineEdit;
class QPushButton;

class ConsoleDock : public QDockWidget {
    Q_OBJECT
public:
    explicit ConsoleDock(QWidget* parent = nullptr);

    void appendRx(const QString& text);
    void appendTx(const QString& text);
    void appendInfo(const QString& text);

signals:
    void sendRequested(const QString& text);

private slots:
    void onSend();

private:
    QPlainTextEdit* m_log{};
    QLineEdit* m_input{};
    QPushButton* m_sendBtn{};
};
