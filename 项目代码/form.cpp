#include "form.h"
#include "ui_form.h"
#include "fsmpCamera.h"

Form::Form(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Form)
  , main_ui(nullptr)
  , time(new QTimer)
{
    ui->setupUi(this);

    //初始化摄像头
    mycamera = new fsmpCamera("/dev/video1", 640, 480, this);

    connect(mycamera, &fsmpCamera::pixReady, this, &Form::open_camera);
    mycamera->start();

    //定时器
    connect(time, &QTimer::timeout, this, &Form::timeout);
    time->start(1000);
}

Form::~Form()
{
    delete ui;
}

void Form::on_pushButton_clicked()
{
    //调到主界面

    this->hide();

    if(main_ui)
    {
        main_ui->show();
    }
    else
    {
        //如果没有设置,作为一种临时方案,查找顶级窗口中的 Widget
        //这不是最好的方法,但对于当前场景可以工作
        QWidgetList topWidgets = QApplication:: topLevelWidgets();
        for(QWidget *w : topWidgets)
        {
            if (Widget *mainWidget = qobject_cast<Widget*>(w))
            {
                mainWidget->show();
                break;
            }
        }
    }
}

void Form::open_camera(const QImage &pix)
{
    //转换图片格式pixmap
    ui->label->setPixmap(QPixmap :: fromImage(pix));

    //设置大小
    ui->label->setPixmap(
    QPixmap :: fromImage(pix).scaled(ui->label->size(),
    Qt :: KeepAspectRatio, Qt :: FastTransformation));

}

void Form::timeout()
{

}

