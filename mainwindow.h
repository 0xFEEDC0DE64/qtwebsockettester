#pragma once

#include <QMainWindow>
#include <QWebSocket>

#include <memory>

namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void connectClicked();
    void sendClicked();
    void settingsClicked();

    void connected();
    void disconnected();
    void stateChanged(QAbstractSocket::SocketState state);
    void textMessageReceived(const QString &message);
    void binaryMessageReceived(const QByteArray &message);
    void error(QAbstractSocket::SocketError error);
    void pong(quint64 elapsedTime, const QByteArray &payload);

private:
    const std::unique_ptr<Ui::MainWindow> m_ui;
    QWebSocket m_webSocket;
};
