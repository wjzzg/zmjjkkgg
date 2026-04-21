#ifndef FORM_H
#define FORM_H

#include <QWidget>
#include "widget.h"
#include <QTimer>
#include <fsmpCamera.h>

class Widget;

namespace Ui {
class Form;
}

class Form : public QWidget
{
    Q_OBJECT

public:
    explicit Form(QWidget *parent = nullptr);
    ~Form();

private slots:
    void on_pushButton_clicked();

    void open_camera(const QImage &pix);

    void timeout();

private:
    Ui::Form *ui;

    Widget *main_ui;

    fsmpCamera *mycamera;

    QTimer *time;
};

#endif // FORM_H
