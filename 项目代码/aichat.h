#ifndef AICHAT_H
#define AICHAT_H

#include <QWidget>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTextBrowser>

// 硬件控制类
#include "fsmpLed.h"
#include "fsmpFan.h"
#include "fsmpBeeper.h"

namespace Ui {
class AiChatWidget;
}

class AiChatWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AiChatWidget(QWidget *parent = nullptr);
    ~AiChatWidget();

private slots:
    void on_pushButton_record_pressed();
    void on_pushButton_record_released();
    void on_pushButton_send_clicked();
    void on_pushButton_file_clicked();
    void on_pushButton_disconnect_clicked();

    void onAsrReplyFinished();
    void onLlmReplyFinished();
    void onTtsReplyFinished();
    void onLlmReadyRead();

private:
    void startRecord();
    void stopRecord();
    void sendAsrRequest(const QString &wavFilePath);
    void sendLlmRequest(const QString &userMessage);
    void sendTtsRequest(const QString &text);
    void playAudioFile(const QString &filePath);
    void appendMessage(const QString &sender, const QString &message);

    void handleLlmResponse(const QString &response);
    void executeDeviceCommand(const QString &jsonStr);

private:
    Ui::AiChatWidget *ui;

    QProcess *recordProcess = nullptr;
    QNetworkAccessManager *netManager = nullptr;
    QNetworkReply *currentLlmReply = nullptr;

    QString inputWavPath = "input.wav";
    QString outputWavPath = "tts_output.wav";

    // API 配置
    QString asrUrl = "http://vop.baidu.com/server_api";
    QString ttsUrl = "https://tsn.baidu.com/text2audio";
    QString llmUrl = "https://qianfan.baidubce.com/v2/chat/completions";

    // 新 Key
    QString accessToken = "bce-v3/ALTAK-fiAVqegbAfiy8oWOMvH1s/a89310c9b58eabcdc3ac730636241965ff81296a";
    QString llmApiKey = "Bearer bce-v3/ALTAK-fiAVqegbAfiy8oWOMvH1s/a89310c9b58eabcdc3ac730636241965ff81296a";

    QString systemPrompt;

    // 硬件控制
    fsmpLeds leds;
    fsmpFan fan;
    fsmpBeeper beeper;

    // 流式缓冲区
    QByteArray llmBuffer;
    bool llmStreaming = false;
};

#endif // AICHAT_H
