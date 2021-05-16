#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QMessageBox>
#include <QPainter>
#include <QTimer>
#include <QDateTime>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle("CameraToRtmp");
    ui->mainToolBar->setVisible(false);
    resize(QSize(800,600));

    mVideoThread = NULL;

    //init device
    avdevice_register_all();
    //init av net
    avformat_network_init();
    avcodec_register_all();
    avdevice_register_all();

    connect(ui->pushButtonStart,&QPushButton::clicked,
            this,slotBtnClicked);
    connect(ui->pushButtonStop,&QPushButton::clicked,
            this,slotBtnClicked);

    ui->pushButtonStart->setEnabled(true);
    ui->pushButtonStop->setEnabled(false);

    timer = new QTimer(this);
    timer->start(1000);
    connect(timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::startRecord()
{
    bool isSucceed = false;
    ErrorCode errVideo, errAudio;
    do
    {
        if(mVideoThread != NULL)
            delete mVideoThread;
        mVideoThread = new CaptureVideoThread;

        connect(mVideoThread,&CaptureVideoThread::sigGetOneImage,this,&MainWindow::slotGetOneImage);
        /*errAudio = mAudioThread->init("麦克风 (2- Realtek(R) Audio)");
        if (errAudio == SUCCEED)
        {
            mAudioThread->startRecord();
        }
        else
        {
            QMessageBox::critical(NULL,"提示","出错了,初始化音频设备失败！");
            qDebug() << "errorAudioCode:" << errAudio;
            break;
        }*/

        errVideo = mVideoThread->init("0");
        if (errVideo == SUCCEED)
        {
            mVideoThread->startRecord();
        }
        else
        {
            QMessageBox::critical(NULL,"提示","出错了,初始化视频设备失败！");
            qDebug() << "errorVideoCode:" << errVideo;
            break;
        }

        ui->pushButtonStart->setEnabled(false);
        ui->pushButtonStop->setEnabled(true);

        isSucceed = true;

        ui->pushButtonStart->setEnabled(false);
        ui->pushButtonStop->setEnabled(true);

    }while(0);

    return isSucceed;
}

void MainWindow::stopRecord()
{
    if (mVideoThread != NULL)
    {
        disconnect(mVideoThread, &CaptureVideoThread::sigGetOneImage, this, &MainWindow::slotGetOneImage);
        mVideoThread->stopRecord();
        //delete mVideoThread;
    }

    ui->pushButtonStart->setEnabled(true);
    ui->pushButtonStop->setEnabled(false);
}

void MainWindow::slotBtnClicked()
{
    if(QObject::sender() == ui->pushButtonStart)
    {
        startRecord();
    }
    else if(QObject::sender() == ui->pushButtonStop)
    {
        stopRecord();
    }
    else
    {
        //qDebug() << "slotGetOneImage";
    }
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setBrush(Qt::white);
    painter.drawRect(0, 0, this->width(), this->height());

    if (mImage.size().width() <= 0) return;

    ///将图像按比例缩放成和窗口一样大小
    QImage img = mImage.scaled(this->size(),Qt::KeepAspectRatio);

    int x = this->width() - img.width();
    int y = this->height() - img.height();

    x /= 2;
    y /= 2;
    //qDebug() << x << "," << y;
    painter.drawImage(QPoint(x,y),img); //画出图像
}

void MainWindow::slotGetOneImage(QImage img)
{
    mImage = img;
    update(); //调用update将执行 paintEvent函数
}

void MainWindow::onTimeout()
{
    QDateTime dateTime = QDateTime::currentDateTime();
    ui->label->setText(dateTime.toString("yyyy-MM-dd hh:mm:ss"));
}
