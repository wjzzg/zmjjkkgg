#include "widget.h"
#include "ui_widget.h"
#include "QDebug"
#include "fsmpLed.h"
#include "fsmpBeeper.h"
#include "QPixmap"
#include "form.h"
#include "aichat.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , time(new QTimer(this))
    , form_ui(nullptr)
{
    ui->setupUi(this);

    ui->horizontalSlider->setRange(0,9999);

    //QPixmap pixmap(":/nailong.jpg");
    //QSize labelSize = ui->label->size();
    //QPixmap scaledPixmap = pixmap.scaled(labelSize, Qt :: KeepAspectRatio, Qt :: SmoothTransformation);
    //ui->label->setPixmap(scaledPixmap);

    connect(&myevent, &fsmpEvents::keyPressed, this, &Widget::pushbutton);

    connect(time, &QTimer::timeout, this, &Widget::timeout);
    time->start(1000);
}

Widget::~Widget()
{
    delete ui;
}


void Widget::on_checkBox_stateChanged(int arg1)
{
    if(arg1)
    {
        myled.on(fsmpLeds::LED2);
    }
    else
    {
        myled.off(fsmpLeds::LED2);
    }
}


void Widget::on_checkBox_2_stateChanged(int arg1)
{
    if(arg1)
    {
        mybeeper.setRate(1000);
        mybeeper.start();
    }
    else
    {
        mybeeper.stop();
    }
}


void Widget::on_horizontalSlider_valueChanged(int value)
{
    qDebug() << ui->horizontalSlider->value();
    mybeeper.setRate(value);
    mybeeper.start();

    ui->progressBar->setValue((ui->horizontalSlider->value()+1)/100);
}


void Widget::on_checkBox_3_stateChanged(int arg1)

{
    if(arg1)
    {
        myfan.setSpeed(100);
        myfan.start();
    }
    else
    {
        myfan.stop();
    }
}

void Widget::pushbutton(int num)
{
    switch(num)
    {
        case 1:
            myled.on(fsmpLeds::LED1);
            break;
        case 2:
            myled.on(fsmpLeds::LED2);
            break;

        case 3:
            myled.on(fsmpLeds::LED3);
            break;
        default:
            break;
    }
}






void Widget::on_checkBox_4_stateChanged(int arg1)
{
    if(arg1)
    {
        myled.on(fsmpLeds::LED1);
    }
    else
    {
        myled.off(fsmpLeds::LED1);
    }
}


void Widget::on_checkBox_5_stateChanged(int arg1)
{
    if(arg1)
    {
        myled.on(fsmpLeds::LED2);
    }
    else
    {
        myled.off(fsmpLeds::LED2);
    }
}


void Widget::on_checkBox_6_stateChanged(int arg1)
{
    if(arg1)
    {
        myled.on(fsmpLeds::LED3);
    }
    else
    {
        myled.off(fsmpLeds::LED3);
    }
}


void Widget::timeout()
{
    ui->doubleSpinBox->setValue(myth.temperature());
    ui->doubleSpinBox_2->setValue(myth.humidity());
}




void Widget::on_pushButton_clicked()
{
    if(!form_ui)
    {
        form_ui = new Form(nullptr);
    }

    this->hide();

    form_ui->show();
}


void Widget::on_pushButton_ai_clicked()
{
    if (!aiChatWindow) {
        aiChatWindow = new AiChatWidget();
        // 可选：连接信号，当AI窗口关闭时自动删除或隐藏
        aiChatWindow->setAttribute(Qt::WA_DeleteOnClose);
        connect(aiChatWindow, &QObject::destroyed, this, [this]() {
            aiChatWindow = nullptr;
        });
    }
    aiChatWindow->show();
    aiChatWindow->raise();
    aiChatWindow->activateWindow();
}
