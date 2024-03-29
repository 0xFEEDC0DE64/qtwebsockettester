#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QMetaEnum>
#include <QWebSocketHandshakeOptions>

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

    connect(m_ui->pushButtonSettings, &QAbstractButton::clicked, this, &MainWindow::settingsClicked);

    connect(&m_webSocket, &QWebSocket::connected, this, &MainWindow::connected);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &MainWindow::disconnected);
    connect(&m_webSocket, &QWebSocket::stateChanged, this, &MainWindow::stateChanged);
    connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &MainWindow::textMessageReceived);
    connect(&m_webSocket, &QWebSocket::binaryMessageReceived, this, &MainWindow::binaryMessageReceived);
    connect(&m_webSocket, qOverload<QAbstractSocket::SocketError>(&QWebSocket::error), this, &MainWindow::error);
    connect(&m_webSocket, &QWebSocket::pong, this, &MainWindow::pong);

    stateChanged(m_webSocket.state());
}

MainWindow::~MainWindow() = default;

void MainWindow::connectClicked()
{
    if (m_webSocket.state() == QAbstractSocket::UnconnectedState)
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

        QWebSocketHandshakeOptions options;
        if (m_ui->checkBoxUseSubprotocol->isChecked())
            options.setSubprotocols({m_ui->lineEditSubprotocol->text()});
        m_webSocket.open(url, std::move(options));
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

void MainWindow::settingsClicked()
{
    const auto supportsSsl = QSslSocket::supportsSsl();
    auto status = QString("supportsSsl = %0\n"
                          "sslLibraryVersionNumber = %1\n"
                          "sslLibraryVersionString = %2\n"
                          "sslLibraryBuildVersionNumber = %3\n"
                          "sslLibraryBuildVersionString = %4\n"
                          "availableBackends = %5\n"
                          "activeBackend = %6")
            .arg(supportsSsl ? "true" : "false")
            .arg(QSslSocket::sslLibraryVersionNumber())
            .arg(QSslSocket::sslLibraryVersionString())
            .arg(QSslSocket::sslLibraryBuildVersionNumber())
            .arg(QSslSocket::sslLibraryBuildVersionString())
            .arg(QSslSocket::availableBackends().join(", "))
            .arg(QSslSocket::activeBackend());

    if (supportsSsl)
        QMessageBox::information(this, tr("SSL Support Status"), std::move(status));
    else
        QMessageBox::warning(this, tr("SSL Support Status"), std::move(status));
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
    m_ui->checkBoxUseSubprotocol->setEnabled(state == QAbstractSocket::UnconnectedState);
    m_ui->lineEditSubprotocol->setEnabled(state == QAbstractSocket::UnconnectedState ? m_ui->checkBoxUseSubprotocol->isChecked() : false);
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
