#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSerialPort>
#include <QDebug>
#include <QTimer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QDebug>
#include <QTcpServer>
#include <QDateTime>
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <stdio.h>
#include <QUdpSocket>
#include <QBuffer>
#include <QWidget>
#include <QPainter>
#include <QApplication>
#include <QProcess>
#include <QWidget>
#include <QPaintEvent>
#include <QEvent>

#define WINDOWS  0

//TCP
QTcpSocket *clien;
//UDP
QUdpSocket *udp;
//串口
QSerialPort *serial;
QTimer *t1;
QTimer *t2;
QTimer *t3;

int sum11=0;
bool sendflag = false;

using namespace std;
using namespace cv;

QTimer *t;
//定义摄像头
VideoCapture cap(0);

QProcess mplayerProcess;
QStringList arguments;

myWidget * mw;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //设置背景颜色
    this->setStyleSheet("background-color: rgb(255,255,255);");

    mw = new myWidget(this);

    //实例化发送端
    clien = new QTcpSocket(this);
    //连接服务器
    clien->connectToHost(QHostAddress("192.168.52.34"),8889);
    //连接信号与槽，执行接收处理函数
    connect(clien,&QTcpSocket::readyRead,this,&MainWindow::myserver);

    //设置串口通信
    serial = new QSerialPort(this);
    serial->setPortName("/dev/ttySAC2");//端口名
    serial->setBaudRate(QSerialPort::Baud9600);//波特率
    serial->setDataBits(QSerialPort::Data8);//数据位
    serial->setStopBits(QSerialPort::OneStop);//停止位
    serial->setParity(QSerialPort::NoParity);//校验位
    serial->setFlowControl(QSerialPort::NoFlowControl);//控制流
    if(serial->open(QIODevice::ReadWrite))//打开权限
    {
        qDebug() << "open QSerialPort success";
    }else
    {
         qDebug() << "open QSerialPort failed";
    }
    connect(serial,&QSerialPort::readyRead,this,&MainWindow::Read_RFID);
    //读卡定时器
    t1 = new QTimer(this);
    connect(t1,&QTimer::timeout,this,&MainWindow::Send_ReadRFID);
    t1->start(500);//500毫秒读一次

    //实时时间显示
    t2 = new QTimer(this);
    connect(t2,&QTimer::timeout,this,&MainWindow::Show_Time);
    t2->start(1000);

    //播放广告
    t3 = new QTimer(this);
    connect(t3,&QTimer::timeout,this,&MainWindow::Show_Advertising);
    t3->start(1000);

#if WINDOWS

     int deviceID = 0; // 0 = open default camera  0 默认 摄像头
     int apiID = cv::CAP_ANY; // 0 = autodetect default API  0 自动 监测  摄像头采集的 格式
     cap.open(deviceID, apiID);  //打开摄像头
     if (!cap.isOpened())   //判断摄像头 是否打开
     {
         cerr << "ERROR! Unable to open camera\n";
         return ;
     }

     //创建UDP通信对象
     udp  = new  QUdpSocket(this);
     //打开UDP套接字
     udp->open(QIODevice::ReadWrite);
     //虚连接
     udp->connectToHost(QHostAddress("172.20.10.2"),9999);


#else

     cap.open("/dev/video7");  //打开摄像头
     if (!cap.isOpened())   //判断摄像头 是否打开
     {
         cerr << "ERROR! Unable to open camera\n";
         return ;
     }

     //创建UDP通信对象
     udp  = new  QUdpSocket(this);
     //打开UDP套接字
     udp->open(QIODevice::ReadWrite);
     //虚连接
     udp->connectToHost(QHostAddress("192.168.52.34"),9999);

#endif

    //实时监控
    t  =  new  QTimer(this);
    connect(t,&QTimer::timeout,this,&MainWindow::show_frame);
    t->start(40);

    //连接信号与槽，先将MPlayer进程杀死在显示界面
    connect(this,&MainWindow::Mplayer_Stop,this,&MainWindow::MplayerDisplay_Stop);


}


void MainWindow::show_frame()
{
    Mat frame;
    cap >> frame; // 从摄像头获取一帧
    //判断是否获取到
    if(!frame.empty())
    {
        //将BGR转换为RGB
        cvtColor(frame,frame, cv::COLOR_BGR2RGB);
        //将Mat转化为QImage
        QImage image((unsigned char *)(frame.data),frame.cols,frame.rows,QImage::Format_RGB888);
        QByteArray byte;
        //将QByteArray转化为QBuffer
        QBuffer buff(&byte);
        //设置打开权限
        buff.open(QIODevice::WriteOnly);
        //保存图片类型为jpeg
        image.save(&buff,"jpeg");

        //发送数据
        udp->write(byte);
    }

}


void MainWindow::Show_Advertising()
{
    sum11++;

    //每秒判断是否有车出入
    if(sendflag == true)//确保只打开一个MPlayer进程
    {
        sendflag = false;
        //清空计数器
        sum11=0;
    }
    //挂机 5 秒
    else if(sum11 == 5 && sendflag == false)
    {
        //重置计数
        sum11 = 0;
        //暂停显示时间
        t2->stop();

        //循环播放广告
        mplayerProcess.start("mplayer -loop 0 ./m1.mp4");

    }
}

void MainWindow::MplayerDisplay_Stop()
{
    //打开计数器
    t2->start(1000);
    //杀死MPlayer进程
    mplayerProcess.kill();
}

void MainWindow::paintEvent(QPaintEvent *event)
{


}



//接收处理函数
void MainWindow::myserver()
{
    QString msg = clien->readAll();
    //将读取的数据按照空格进行切割
    QStringList list = msg.split(" ");
    // 分别存储子串的QString对象
    QString str1 = list.value(0);
    QString str2 = list.value(1); //flag
    QString str3 = list.value(2); //id

    if(str1 == "open_door")
    {

        //停止MPlayer
        emit Mplayer_Stop();

        //重新计时
        sendflag = true;


        //显示时间
        MainWindow::Show_Time();

        //显示车牌号
        ui->label_2->setText(str3);

        //判断是出库还是入库
        if(str2 == "1") //出库
        {
            ui->label_3->clear();
            ui->label_3->setText("一路顺风");

        }
        else if(str2 == "0") //入库
        {
            ui->label_3->clear();
            ui->label_3->setText("欢迎回家");


        }

    }

}

//显示实时时间
void MainWindow::Show_Time()
{
    // 获取当前日期和时间
    QDateTime currentDateTime = QDateTime::currentDateTime();

    // 将日期和时间转换为字符串
    QString currentDateTimeString = currentDateTime.toString("yyyy-MM-dd hh:mm:ss");

    ui->label->setText(currentDateTimeString);
}


//发送读卡指令
void MainWindow::Send_ReadRFID()
{
    char msg[8];
    msg[0] = 0x07;
    msg[1] = 0x02;
    msg[2] = 0x41;
    msg[3] = 0x01;
    msg[4] = 0x52;
    char BCC = 0;
    for(int i=0; i<(msg[0]-2); i++) {
    BCC ^= msg[i];
    }
    msg[msg[0]-2] = ~BCC;
    msg[6] = 0x03;

    serial->write(msg,7);
}


//读取一张卡
void MainWindow::Read_RFID()
{
    char buf[1024]={0};
    int size = serial->read(buf,1024);

    if(size == 0x08 && buf[2] == 0x00)
    {
        qDebug() <<  "RequestSuccessful";

        //发送防碰撞协议
        char   Anticollision[9]={0};
        Anticollision[0] = 0x08;
        Anticollision[1] = 0x02;
        Anticollision[2] = 0x42;
        Anticollision[3] = 0x02;
        Anticollision[4] = 0x93;
        Anticollision[5] = 0x00;
        char BCC=0;
        for(int i=0; i < Anticollision[0] -2;i++)
        {
              BCC ^= Anticollision[i];
        }
        Anticollision[6]  = ~BCC;
        Anticollision[7] = 0x03;
        serial->write(Anticollision,8); //发送防碰撞协议

    }else if(size == 0x08 && buf[2] != 0x00)
    {
         qDebug() <<  "Requestfail";
    }

    //成功获取到ID
    if(size == 0x0a && buf[2] == 0x00)
    {
        qDebug() << "Anticollision Successful";

        QString id =  QString("%1%2%3%4").arg(int(buf[7])).arg(int(buf[6])).arg(int(buf[5])).arg(int(buf[4]));

        //防止无效ID
        if(id != "0000")
        {
            //将ID发送给服务端
            QString l= QString("%1").arg(id.length());
            QString msg = QString("00 %1 %2").arg(l).arg(id);
            clien->write(msg.toUtf8(),msg.toUtf8().length());
        }
    }else if(size == 0x0a  && buf[2] != 0x00)
    {
        qDebug() << "Anticollision fail";
    }

}

MainWindow::~MainWindow()
{
    delete ui;
}





void myWidget::paintEvent(QPaintEvent *event)
{
//    QPainter painter(this);
//    painter.drawText(-scrollPos, 0, text());
}
