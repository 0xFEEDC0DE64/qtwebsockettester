#pragma once

#include <QMainWindow>
#include <QWebSocket>
#include <QComboBox>

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
    void connectToWebsocket();

    void connected();
    void disconnected();
    void stateChanged(QAbstractSocket::SocketState state);
    void textMessageReceived(const QString &message);
    void binaryMessageReceived(const QByteArray &message);
    void error(QAbstractSocket::SocketError error);
    void pong(quint64 elapsedTime, const QByteArray &payload);
    void saveSettings();
    void loadSettings();
    void loadSelectedUrl();
private:
    const std::unique_ptr<Ui::MainWindow> m_ui;
    QWebSocket m_webSocket;
    std::array<QString, 10> m_url_slots;
};
