#include "consoledock.h"
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QWidget>

static QPushButton* makeCmdBtn(const QString& text) {
    auto* b = new QPushButton(text);
    b->setMinimumHeight(32);
    return b;
}

ConsoleDock::ConsoleDock(QWidget* parent) : QDockWidget("命令行调试", parent) {
    auto* root = new QWidget(this);
    auto* v = new QVBoxLayout(root);
    v->setContentsMargins(10, 10, 10, 10);
    v->setSpacing(8);

    m_log = new QPlainTextEdit;
    m_log->setReadOnly(true);
    m_log->setMinimumHeight(200);
    m_log->setStyleSheet("QPlainTextEdit { background:#0f0f0f; color:#eaeaea; }");

    auto* grid = new QGridLayout;
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(8);

    QPushButton* b1 = makeCmdBtn("读取状态");
    QPushButton* b2 = makeCmdBtn("查询参数");
    QPushButton* b3 = makeCmdBtn("开始采集");
    QPushButton* b4 = makeCmdBtn("停止采集");

    grid->addWidget(b1, 0, 0);
    grid->addWidget(b2, 0, 1);
    grid->addWidget(b3, 1, 0);
    grid->addWidget(b4, 1, 1);

    auto* h = new QHBoxLayout;
    m_input = new QLineEdit;
    m_input->setPlaceholderText("输入命令（ASCII/HEX 后续可扩展）");
    m_sendBtn = new QPushButton("发送");
    m_sendBtn->setMinimumWidth(90);

    h->addWidget(m_input, 1);
    h->addWidget(m_sendBtn);

    connect(b1, &QPushButton::clicked, this, [this]{ m_input->setText("READ_STATUS"); });
    connect(b2, &QPushButton::clicked, this, [this]{ m_input->setText("QUERY_PARAM"); });
    connect(b3, &QPushButton::clicked, this, [this]{ m_input->setText("START_MONITOR"); });
    connect(b4, &QPushButton::clicked, this, [this]{ m_input->setText("STOP_MONITOR"); });

    connect(m_sendBtn, &QPushButton::clicked, this, &ConsoleDock::onSend);
    connect(m_input, &QLineEdit::returnPressed, this, &ConsoleDock::onSend);

    v->addWidget(m_log, 2);
    v->addLayout(grid);
    v->addLayout(h);

    setWidget(root);
}

void ConsoleDock::appendRx(const QString& text)   { m_log->appendPlainText(text.trimmed()); }
void ConsoleDock::appendTx(const QString& text)   { m_log->appendPlainText(text.trimmed()); }
void ConsoleDock::appendInfo(const QString& text) { m_log->appendPlainText(text.trimmed()); }

void ConsoleDock::onSend() {
    const QString cmd = m_input->text().trimmed();
    if (cmd.isEmpty()) return;
    emit sendRequested(cmd);
    m_input->clear();
}
