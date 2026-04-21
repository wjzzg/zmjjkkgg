#include "aichat.h"
#include "ui_aichat.h"

#include <QFile>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QUrlQuery>
#include <QNetworkRequest>
#include <QDebug>
#include <QDateTime>
#include <QFileInfo>
#include <QMessageBox>


AiChatWidget::AiChatWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AiChatWidget),
    currentLlmReply(nullptr),
    llmStreaming(false)
{
    ui->setupUi(this);

    recordProcess = new QProcess(this);
    netManager = new QNetworkAccessManager(this);

    // 加载系统提示词
    QFile file(":/prompt.txt");
    if (file.open(QIODevice::ReadOnly)) {
        systemPrompt = QString::fromUtf8(file.readAll());
        file.close();
    } else {
        systemPrompt = "你是一个智能家庭管家，名叫“小智”。";
    }

    appendMessage("小智", "你好！我是你的智能管家小智，有什么可以帮你的？");

    ui->pushButton_record->setStyleSheet(
        "QPushButton { background-color: #428bca; color: white; border: none; border-radius: 20px; }"
        "QPushButton:hover { background-color: #3071a9; }"
        "QPushButton:pressed { background-color: #1d4568; }"
    );
}

AiChatWidget::~AiChatWidget()
{
    if (recordProcess && recordProcess->state() != QProcess::NotRunning) {
        recordProcess->terminate();
        recordProcess->waitForFinished(1000);
    }
    delete ui;
}

void AiChatWidget::appendMessage(const QString &sender, const QString &message)
{
    QString timeStr = QDateTime::currentDateTime().toString("hh:mm");
    QString displayText;
    if (sender == "我") {
        displayText = QString("<div align='right'><b>%1 %2</b><br>"
                              "<span style='background-color:#95ec69; padding:8px 12px; border-radius:15px;'>%3</span></div><br>")
                              .arg(sender, timeStr, message);
    } else {
        displayText = QString("<div align='left'><b>%1 %2</b><br>"
                              "<span style='background-color:#ffffff; padding:8px 12px; border-radius:15px; border:1px solid #d0d0d0;'>%3</span></div><br>")
                              .arg(sender, timeStr, message);
    }
    ui->textBrowser_chat->append(displayText);
}

// ================== 录音 ==================
void AiChatWidget::on_pushButton_record_pressed()
{
    startRecord();
    ui->pushButton_record->setText("松开结束");
    ui->pushButton_record->setStyleSheet(
        "QPushButton { background-color: #d9534f; color: white; border: none; border-radius: 20px; }"
    );
}

void AiChatWidget::on_pushButton_record_released()
{
    stopRecord();
    ui->pushButton_record->setText("按住录音");
    ui->pushButton_record->setStyleSheet(
        "QPushButton { background-color: #428bca; color: white; border: none; border-radius: 20px; }"
        "QPushButton:hover { background-color: #3071a9; }"
    );
    sendAsrRequest(inputWavPath);
}

void AiChatWidget::startRecord()
{
    if (recordProcess->state() != QProcess::NotRunning) {
        recordProcess->terminate();
        recordProcess->waitForFinished(1000);
    }
    QFile::remove(inputWavPath);

    QString program = "arecord";
    QStringList args;
    args << "-D" << "plughw:1,0"
         << "-f" << "S16_LE"
         << "-r" << "16000"
         << "-c" << "1"
         << "-t" << "wav"
         << inputWavPath;

    recordProcess->start(program, args);
    if (!recordProcess->waitForStarted(1000)) {
        appendMessage("系统", "录音启动失败，请检查麦克风");
    }
}

void AiChatWidget::stopRecord()
{
    if (recordProcess->state() == QProcess::NotRunning)
        return;
    recordProcess->terminate();
    if (!recordProcess->waitForFinished(1500)) {
        recordProcess->kill();
        recordProcess->waitForFinished(1000);
    }
}

// ================== ASR ==================
void AiChatWidget::sendAsrRequest(const QString &wavFilePath)
{
    QFile file(wavFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        appendMessage("系统", "无法读取录音文件");
        return;
    }
    QByteArray wavData = file.readAll();
    file.close();
    if (wavData.isEmpty()) {
        appendMessage("系统", "录音文件为空");
        return;
    }

    QJsonObject obj;
    obj["format"] = "wav";
    obj["rate"] = 16000;
    obj["dev_pid"] = 1537;
    obj["channel"] = 1;
    obj["cuid"] = "baidu_workshop";
    obj["len"] = wavData.size();
    obj["speech"] = QString::fromLatin1(wavData.toBase64());

    QNetworkRequest req{QUrl(asrUrl)};
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", QString("Bearer %1").arg(accessToken).toUtf8());

    QNetworkReply *reply = netManager->post(req, QJsonDocument(obj).toJson());
    connect(reply, &QNetworkReply::finished, this, &AiChatWidget::onAsrReplyFinished);
    appendMessage("系统", "正在识别语音...");
}

void AiChatWidget::onAsrReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    QByteArray data = reply->readAll();
    if (reply->error() != QNetworkReply::NoError) {
        appendMessage("系统", "语音识别失败: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();
    int errNo = obj.value("err_no").toInt(-1);
    if (errNo != 0) {
        appendMessage("系统", "识别失败: " + obj.value("err_msg").toString());
        reply->deleteLater();
        return;
    }

    QString resultText;
    if (obj.contains("result") && obj["result"].isArray()) {
        QJsonArray arr = obj["result"].toArray();
        if (!arr.isEmpty()) resultText = arr.at(0).toString();
    }

    if (resultText.isEmpty()) {
        appendMessage("系统", "未识别到有效语音");
    } else {
        appendMessage("我", resultText);
        sendLlmRequest(resultText);
    }
    reply->deleteLater();
}

// ================== LLM (千帆) ==================
void AiChatWidget::sendLlmRequest(const QString &userMessage)
{
    QJsonArray messages;
    messages.append(QJsonObject{{"role", "system"}, {"content", systemPrompt}});
    messages.append(QJsonObject{{"role", "user"}, {"content", userMessage}});

    QJsonObject body;
    body["messages"] = messages;
    body["model"] = "ernie-5.0";
    body["stream"] = true;

    QNetworkRequest req{QUrl(llmUrl)};

    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", llmApiKey.toUtf8());

    currentLlmReply = netManager->post(req, QJsonDocument(body).toJson());
    connect(currentLlmReply, &QNetworkReply::readyRead, this, &AiChatWidget::onLlmReadyRead);
    connect(currentLlmReply, &QNetworkReply::finished, this, &AiChatWidget::onLlmReplyFinished);

    llmBuffer.clear();
    llmStreaming = true;
    appendMessage("小智", "（思考中...）");
}

void AiChatWidget::onLlmReadyRead()
{
    if (currentLlmReply)
        llmBuffer.append(currentLlmReply->readAll());
}

void AiChatWidget::onLlmReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() != QNetworkReply::NoError) {
        appendMessage("系统", "大模型请求失败: " + reply->errorString());

        qDebug() << reply->errorString() << endl;
        reply->deleteLater();
        currentLlmReply = nullptr;
        return;
    }

    llmBuffer.append(reply->readAll());

    QString fullResponse;
    QString dataStr = QString::fromUtf8(llmBuffer);
    QStringList lines = dataStr.split("\n", Qt::SkipEmptyParts);

    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.startsWith("data: ")) {
            QString jsonStr = trimmed.mid(6);
            if (jsonStr == "[DONE]") continue;

            QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
            if (doc.isObject()) {
                QJsonArray choices = doc.object()["choices"].toArray();
                if (!choices.isEmpty()) {
                    fullResponse += choices.first().toObject()["delta"].toObject()["content"].toString();
                }
            }
        }
    }

    if (!fullResponse.isEmpty()) {
        handleLlmResponse(fullResponse.trimmed());
    } else {
        appendMessage("小智", "抱歉，我没有理解你的意思。");
    }

    reply->deleteLater();
    currentLlmReply = nullptr;
    llmBuffer.clear();
    llmStreaming = false;
}

void AiChatWidget::handleLlmResponse(const QString &response)
{
    // 尝试解析为 JSON 指令
    if (response.startsWith("{") && response.endsWith("}")) {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8(), &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            executeDeviceCommand(response);
            return;
        }
    }

    // 普通文本回复
    appendMessage("小智", response);
    sendTtsRequest(response);
}

void AiChatWidget::executeDeviceCommand(const QString &jsonStr)
{
    QJsonObject cmd = QJsonDocument::fromJson(jsonStr.toUtf8()).object();
    QString dev = cmd["dev"].toString();
    QString stat = cmd["stat"].toString();
    int temp = cmd["temp"].toInt();

    QString resultMsg;

    if (dev == "led") {
        if (stat == "on") {
            leds.on(fsmpLeds::LED1);
            resultMsg = "好的，已打开灯光。";
        } else if (stat == "off") {
            leds.off(fsmpLeds::LED1);
            resultMsg = "好的，已关闭灯光。";
        }
    } else if (dev == "ac") {
        if (temp != 0) {
            if (temp > 0) {
                fan.setSpeed(150);
                fan.start();
                resultMsg = QString("已调高温度到 %1 度").arg(temp);
            } else {
                fan.setSpeed(80);
                fan.start();
                resultMsg = QString("已调低温度 %1 度").arg(-temp);
            }
        } else {
            if (stat == "on") {
                fan.start();
                resultMsg = "已开启空调。";
            } else if (stat == "off") {
                fan.stop();
                resultMsg = "已关闭空调。";
            }
        }
    } else {
        resultMsg = "收到指令，但暂不支持该设备。";
    }

    appendMessage("小智", resultMsg);
    sendTtsRequest(resultMsg);
}

// ================== TTS ==================
void AiChatWidget::sendTtsRequest(const QString &text)
{
    QUrlQuery query;
    query.addQueryItem("tex", text);
    query.addQueryItem("tok", accessToken);
    query.addQueryItem("cuid", "baidu_workshop");
    query.addQueryItem("ctp", "1");
    query.addQueryItem("lan", "zh");
    query.addQueryItem("aue", "6");      // MP3 格式
    query.addQueryItem("spd", "5");
    query.addQueryItem("pit", "5");
    query.addQueryItem("vol", "5");
    query.addQueryItem("per", "4105");

    QNetworkRequest req{QUrl(ttsUrl)};
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    req.setRawHeader("Authorization", QString("Bearer %1").arg(accessToken).toUtf8());

    QNetworkReply *reply = netManager->post(req, query.query(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, &AiChatWidget::onTtsReplyFinished);
}

void AiChatWidget::onTtsReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "TTS Error:" << reply->errorString();
        appendMessage("系统", "语音合成失败: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QByteArray contentType = reply->header(QNetworkRequest::ContentTypeHeader).toByteArray();

    qDebug() << "TTS data size:" << data.size();
    qDebug() << "Content-Type:" << contentType;

    if (data.size() > 1000) {
        QString filePath = "/tmp/tts_output.wav";
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(data);
            file.close();
            qDebug() << "Audio saved to:" << filePath;
            qDebug() << "File size:" << QFileInfo(filePath).size();

            playAudioFile(filePath);
        } else {
            qDebug() << "Failed to open file for writing";
        }
    } else {
        qDebug() << "TTS response too small:" << QString::fromUtf8(data);
        appendMessage("系统", "语音合成返回数据异常");
    }
    reply->deleteLater();
}

void AiChatWidget::playAudioFile(const QString &filePath)
{
    if (!QFile::exists(filePath)) {
        qDebug() << "[PLAY] 音频文件不存在:" << filePath;
        return;
    }

    qDebug() << "[PLAY] 播放:" << filePath;

    // 使用默认声卡播放
    QString cmd = QString("aplay -f S16_LE -r 16000 -c 1 \"%1\" > /dev/null 2>&1 &").arg(filePath);
    system(cmd.toUtf8().constData());
}
// ================== 其他按钮 ==================
void AiChatWidget::on_pushButton_send_clicked()
{
    QString text = ui->textEdit_input->toPlainText().trimmed();
    if (text.isEmpty())
        return;

    appendMessage("我", text);
    ui->textEdit_input->clear();
    sendLlmRequest(text);
}

void AiChatWidget::on_pushButton_file_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "选择音频文件", "", "WAV文件 (*.wav)");
    if (!fileName.isEmpty()) {
        sendAsrRequest(fileName);
    }
}

void AiChatWidget::on_pushButton_disconnect_clicked()
{
    if (currentLlmReply) {
        currentLlmReply->abort();
        currentLlmReply->deleteLater();
        currentLlmReply = nullptr;
    }
    appendMessage("系统", "已断开云端连接");
}
