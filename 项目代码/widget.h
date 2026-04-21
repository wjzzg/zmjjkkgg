#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <fsmpLed.h>
#include <fsmpBeeper.h>
#include <fsmpFan.h>
#include <fsmpEvents.h>
#include <fsmpTempHum.h>

#include <QTimer>
#include "aichat.h"

class Form;

QT_BEGIN_NAMESPACE

namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    void on_checkBox_stateChanged(int arg1);

    void on_checkBox_2_stateChanged(int arg1);

    void on_horizontalSlider_valueChanged(int value);

    void on_checkBox_3_stateChanged(int arg1);

    void pushbutton(int num);

    void timeout();

    void on_checkBox_4_stateChanged(int arg1);

    void on_checkBox_5_stateChanged(int arg1);

    void on_checkBox_6_stateChanged(int arg1);

    void on_pushButton_clicked();

    void on_pushButton_ai_clicked();

private:
    Ui::Widget *ui;

    fsmpLeds myled;
    fsmpBeeper mybeeper;
    fsmpFan myfan;
    fsmpEvents myevent;
    fsmpTempHum myth;

    QTimer *time;

    Form *form_ui;

    AiChatWidget *aiChatWindow;

};
#endif // WIDGET_H
