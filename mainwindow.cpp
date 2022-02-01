#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QMetaEnum>
#include <QSettings>

// utilities
namespace {
template<typename QEnum>
auto qtEnumToString(const QEnum value)
{
    return QMetaEnum::fromType<QEnum>().valueToKey(value);
}
} // namespace

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow{parent},
    m_ui{std::make_unique<Ui::MainWindow>()},
    m_webSocket{QString{}, QWebSocketProtocol::VersionLatest, this}
{
    m_ui->setupUi(this);

    connect(m_ui->lineEditUrl, &QLineEdit::returnPressed, this, &MainWindow::connectClicked);
    connect(m_ui->pushButtonConnect, &QAbstractButton::clicked, this, &MainWindow::connectClicked);

    connect(m_ui->lineEditSend, &QLineEdit::returnPressed, this, &MainWindow::sendClicked);
    connect(m_ui->pushButtonSend, &QAbstractButton::clicked, this, &MainWindow::sendClicked);

    connect(m_ui->saveSlot, &QComboBox::currentIndexChanged, this, &MainWindow::loadSelectedUrl);
    connect(m_ui->pushButtonSave, &QAbstractButton::clicked, this, &MainWindow::saveSettings);

    connect(&m_webSocket, &QWebSocket::connected, this, &MainWindow::connected);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &MainWindow::disconnected);
    connect(&m_webSocket, &QWebSocket::stateChanged, this, &MainWindow::stateChanged);
    connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &MainWindow::textMessageReceived);
    connect(&m_webSocket, &QWebSocket::binaryMessageReceived, this, &MainWindow::binaryMessageReceived);
    connect(&m_webSocket, qOverload<QAbstractSocket::SocketError>(&QWebSocket::error), this, &MainWindow::error);
    connect(&m_webSocket, &QWebSocket::pong, this, &MainWindow::pong);

    for (uint8_t i = 1; i <= m_url_slots.size(); i++) {
        m_ui->saveSlot->addItem("Slot "+QString::number(i));
    }

    loadSettings();

    stateChanged(m_webSocket.state());
}

MainWindow::~MainWindow() = default;

void MainWindow::connectClicked()
{
    if (m_webSocket.state() == QAbstractSocket::UnconnectedState)
    {
        connectToWebsocket();
    }
    else
        m_webSocket.close();
}

void MainWindow::sendClicked()
{
    if (m_webSocket.state() != QAbstractSocket::ConnectedState)
    {
        QMessageBox::warning(this, tr("WebSocket not connected!"), tr("WebSocket not connected!"));
        return;
    }

    const auto msg = m_ui->lineEditSend->text();
    m_webSocket.sendTextMessage(msg);
    m_ui->lineEditSend->clear();

    m_ui->plainTextEdit->appendHtml(QStringLiteral("<b>%0</b> <span style=\"color: %1;\">%2</span>: %3<br/>")
                                    .arg(QTime::currentTime().toString())
                                    .arg("red")
                                    .arg(tr("SEND"))
                                        .arg(msg));
}

void MainWindow::connectToWebsocket()
{
    const auto url = QUrl::fromUserInput(m_ui->lineEditUrl->text());
    if (url.isEmpty())
    {
        QMessageBox::warning(this, tr("Invalid url entered!"), tr("Invalid url entered!"));
        return;
    }

    if (url.scheme().toLower() != "ws" &&
        url.scheme().toLower() != "wss")
    {
        QMessageBox::warning(this, tr("Invalid url entered!"), tr("Only urls starting with ws:// or wss:// are allowed!"));
        return;
    }

    m_ui->plainTextEdit->appendHtml(QStringLiteral("<b>%0</b> <i>%1</i><br/>")
                                        .arg(QTime::currentTime().toString())
                                        .arg(tr("Connecting to %0").arg(url.toString())));

    m_webSocket.open(url);
}

void MainWindow::connected()
{
    m_ui->plainTextEdit->appendHtml(QStringLiteral("<b>%0</b> <i>%1</i><br/>")
                                        .arg(QTime::currentTime().toString())
                                        .arg(tr("Connected")));
}

void MainWindow::disconnected()
{
    m_ui->plainTextEdit->appendHtml(QStringLiteral("<b>%0</b> <i>%1</i><br/>")
                                        .arg(QTime::currentTime().toString())
                                        .arg(tr("Disconnected")));
}

void MainWindow::stateChanged(QAbstractSocket::SocketState state)
{
    m_ui->lineEditUrl->setEnabled(state == QAbstractSocket::UnconnectedState);
    m_ui->pushButtonConnect->setText(state == QAbstractSocket::UnconnectedState ? tr("Connect") : tr("Disconnect"));
    m_ui->labelStatus->setText(qtEnumToString(state));
    m_ui->lineEditSend->setEnabled(state == QAbstractSocket::ConnectedState);
    m_ui->pushButtonSend->setEnabled(state == QAbstractSocket::ConnectedState);
}

void MainWindow::textMessageReceived(const QString &message)
{
    m_ui->plainTextEdit->appendHtml(QStringLiteral("<b>%0</b> <span style=\"color: %1;\">%2</span>: %3<br/>")
                                        .arg(QTime::currentTime().toString())
                                        .arg("green")
                                        .arg(tr("RECV"))
                                        .arg(message));
}

void MainWindow::binaryMessageReceived(const QByteArray &message)
{
    m_ui->plainTextEdit->appendHtml(QStringLiteral("<b>%0</b> <span style=\"color: %1;\">%2</span>: %3<br/>")
                                        .arg(QTime::currentTime().toString())
                                        .arg("blue")
                                        .arg(tr("RECV"))
                                        .arg(tr("&lt;BINARY&gt;")));
}

void MainWindow::error(QAbstractSocket::SocketError error)
{
    QMessageBox::warning(this, tr("WebSocket error occured!"), tr("WebSocket error occured!") + "\n\n" + qtEnumToString(error));
}

void MainWindow::pong(quint64 elapsedTime, const QByteArray &payload)
{
    qDebug() << "pong" << elapsedTime;
}

void MainWindow::saveSettings()
{
    QSettings settings;
    m_url_slots[m_ui->saveSlot->currentIndex()] = m_ui->lineEditUrl->text();
    for (uint8_t i = 1; i <= m_url_slots.size(); i++) {
        settings.setValue("slot"+QString::number(i-1), m_url_slots[i-1]);
    }
}

void MainWindow::loadSettings()
{
    QSettings settings;
    for (uint8_t i = 1; i <= m_url_slots.size(); i++) {
        QString key = "slot"+QString::number(i-1);
        m_url_slots[i-1] = settings.value(key, "ws://localhost:1234/path/to/ws").toString();
    }
    loadSelectedUrl();
    saveSettings();
}

void MainWindow::loadSelectedUrl()
{
    const auto tmpSocketState = m_webSocket.state();
    m_webSocket.close();
    m_ui->lineEditUrl->setText(m_url_slots[m_ui->saveSlot->currentIndex()]);

    if (tmpSocketState == QAbstractSocket::ConnectedState)
        connectToWebsocket();
}
