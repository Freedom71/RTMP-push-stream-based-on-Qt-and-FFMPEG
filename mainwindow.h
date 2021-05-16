#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "capturevideothread.h"
#include <QImage>
#include <QPaintEvent>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    bool startRecord();
    void stopRecord();

private slots:
    void slotBtnClicked();
    void slotGetOneImage(QImage img);
    void onTimeout();
protected:
    void paintEvent(QPaintEvent *event);

private:
    Ui::MainWindow *ui;
    CaptureVideoThread *mVideoThread;
    QImage mImage;
    QTimer *timer;
};

#endif // MAINWINDOW_H
