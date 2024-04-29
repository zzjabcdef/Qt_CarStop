#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "register.h"
#include <QSqlDatabase> //数据库管理器
#include <QSqlQuery>    //命令管理器
#include <QTableWidget>
#include <QDebug>
#include <QDateTime>
#include "data_check.h"
#include <QTcpSocket>
#include <QHostAddress>
#include <QSctpSocket>
#include <QTcpServer>
#include <QUdpSocket>
#include <iostream>
#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <QBuffer>
#include <QImageReader>
#include "maintest.h"
#include <QRandomGenerator>
#include <QTimer>
#include <QFile>
#include <QDateTime>
using namespace std;
using namespace cv;

//自定义车牌结构体
struct License
{
    Mat mat;  //ROI图片
    Rect rect; //ROI所在矩形
};

//车牌的省份缩写
const char *provinces[34]={"京", "津", "冀", "晋", "蒙", "辽", "吉", "黑",
                           "沪", "苏", "浙", "皖", "闽", "赣", "鲁", "豫",
                           "鄂", "湘", "粤", "桂", "琼", "渝", "川", "贵",
                           "云", "藏", "陕", "甘", "青", "宁", "新", "港",
                           "澳", "台"};

// 登录界面
Register *r;
//查询界面
Data_Check *Dcheck;
//数据库项数
int row = 0;
//TCP
QTcpServer *server;
QTcpSocket *clien;
//UDP
QUdpSocket *udp;
//OCR
OCR *ocr;
//车牌检测间隔计数器
int SUM = 0;
//车牌计时
QTimer *car_timer;
int carflag=0;
//刷卡计时
QTimer *id_timer;
int idflag=0;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //实例化接送
    server = new QTcpServer(this);
    //监听并绑定端口
    server->listen(QHostAddress::Any,8889);
    //实例化发送端
    clien = new QTcpSocket(this);
    //连接信号与槽，执行连接成功处理函数
    connect(server,&QTcpServer::newConnection,this,[&](){

        clien = server->nextPendingConnection();
        //连接信号与槽，执行接收处理函数
        connect(clien,&QTcpSocket::readyRead,this,&MainWindow::myserver);

    });

    //查询界面实例化
    Dcheck = new Data_Check(this);
    //隐藏窗体
    Dcheck->hide();

    //获取数据库的表格信息
    db = QSqlDatabase::addDatabase("QSQLITE");  //设置数据库软件类型
    db.setDatabaseName("my.db"); //设置远程数据库文件名
    if(!db.open())   //打开数据库文件
    {
        qDebug() << "opencv sql err";
        return;
    }

    //实例化界面
    r = new Register(this);
    //显示登陆界面
    r->show();

    //连接信号与槽,登录成功，切换到主页面
    connect(r,&Register::Register_Hide,this,[&](QString id,QString pwd){

        db.setDatabaseName("register.db"); //设置远程数据库文件名
        db.open();//打开数据库
        QSqlQuery  cmd1(db);
        //创建一个用户表格
        cmd1.exec("create table myregister(id primary key,pwd not null);");
        cmd1.exec("insert into myregister values('123','123')");
        QString sql = QString("select * from myregister where '%1'='%2';").arg(id).arg(pwd);
        if(cmd1.exec(sql))
        {
            this->show();
            r->hide();
            db.setDatabaseName("my.db"); //设置远程数据库文件名
            db.open();
        }

        //创建UDP通信对象
        udp  = new  QUdpSocket(this);
        //绑定接收地址信息
        if(udp->bind(QHostAddress::Any, 9999))
        {
            qDebug() << "绑定服务器成功";
        }else
        {
             qDebug() << "绑定服务器失败";
        }

        //连接信号与槽，获取UDP数据
        connect(udp,&QUdpSocket::readyRead,this,&MainWindow::myudp);

    });


    QSqlQuery  cmd(db);
    //创建一个RFID 表格
    cmd.exec("create table rfid(id,in_time not null,out_time,money,car,in_path,out_path);");


    //设置表格的行与列
    int r = 0;
    cmd.exec("select * from rfid;");
    while (cmd.next()) {
        r++;
    }
    ui->tableWidget->setRowCount(r);
    ui->tableWidget->setColumnCount(5);
    //设置表头
    QStringList head;
    head<<"卡号" << "入场时间" << "出场时间" << "金额" << "车牌号";
    ui->tableWidget->setHorizontalHeaderLabels(head);

    //获取表格数据的总行数
    cmd.exec("select * from rfid;");
    while (cmd.next()) {

        //设置TableWidget中的内容
        ui->tableWidget->setItem(row,0,new QTableWidgetItem(cmd.value(0).toString()));
        ui->tableWidget->setItem(row,1,new QTableWidgetItem(cmd.value(1).toString()));
        ui->tableWidget->setItem(row,2,new QTableWidgetItem(cmd.value(2).toString()));
        ui->tableWidget->setItem(row,3,new QTableWidgetItem(cmd.value(3).toString()));
        ui->tableWidget->setItem(row,4,new QTableWidgetItem(cmd.value(4).toString()));
        row++;

    }

    ocr = new OCR;

    //车牌识别定时器
    car_timer = new QTimer(this);
    connect(car_timer,&QTimer::timeout,this,&MainWindow::Set_CarFlag);
    //ID卡识别定时器
    id_timer = new QTimer(this);
    connect(id_timer,&QTimer::timeout,this,&MainWindow::Set_IDFlag);




}

//发送开门信号  flag:0 入库 1 出库   id:车牌号(7位) 卡号
void Send_OpenDoor(const QString id, int flag)
{
    QString msg = QString("open_door %1 %2").arg(flag).arg(id);
    clien->write(msg.toUtf8(),msg.toUtf8().length());

}


//服务器接送处理程序
void MainWindow::myserver()
{
    //解析数据包
    //下标
    int num=0;
    //读取串口数据
    QString msg = clien->readAll();

    if(idflag == 0)
    {
        //获取数据的类型
        QString flag = QString("%1%2").arg(msg.at(num)).arg(msg.at(num+1));
        num+=3;//下标偏移
        //获取数据包中数据的长度
        QString msg_length;
        if(msg.at(4) == " ")
        {
            msg_length = QString("%1").arg(msg.at(num));
            num+=2;
        }else
        {
            msg_length = QString("%1%2").arg(msg.at(num)).arg(msg.at(num+1));
            num+=3;
        }

        //接收的是ID卡号
        if(flag == "00")
        {
            int l = msg_length.toInt();

            QString ID;
            //获取ID
            for (int i = num;i < l+num; i++) {
                ID.append(msg.at(i));
            }
            qDebug() << "ID : " << ID;

            //查看出库还是入库
            QSqlQuery  cmd(db);
            QString str = QString("select * from rfid where id='%1';").arg(ID);
            //查找ID
            int n = 0;
            QString in_t;
            cmd.exec(str);
            while (cmd.next()) {
                if(cmd.value(2) == "未出库")
                    n++;
                in_t = cmd.value(1).toString();
            }
            //没找到，即是入库

            if(n == 0)
            {
                // 获取当前日期和时间
                QDateTime currentDateTime = QDateTime::currentDateTime();

                // 将日期和时间转换为字符串
                QString currentDateTimeString = currentDateTime.toString("yyyy-MM-dd hh:mm:ss");
                QString sql = QString("insert into rfid(id ,in_time,out_time,money) values('%1','%2','未出库',0);").arg(ID).arg(currentDateTimeString);

                //将ID卡数据写入数据库
                cmd.exec(sql);

                //更新表格
                int row1=0;
                //清除tableWidget数据
                ui->tableWidget->clear();
                //设置表头
                QStringList head;
                head<<"卡号" << "入场时间" << "出场时间" << "金额" << "车牌号";
                ui->tableWidget->setHorizontalHeaderLabels(head);
                //设置行数
                ui->tableWidget->setRowCount(++row);
                cmd.exec("select * from rfid;");
                while (cmd.next()) {

                    //设置TableWidget中的内容
                    ui->tableWidget->setItem(row1,0,new QTableWidgetItem(cmd.value(0).toString()));
                    ui->tableWidget->setItem(row1,1,new QTableWidgetItem(cmd.value(1).toString()));
                    ui->tableWidget->setItem(row1,2,new QTableWidgetItem(cmd.value(2).toString()));
                    ui->tableWidget->setItem(row1,3,new QTableWidgetItem(cmd.value(3).toString()));
                    ui->tableWidget->setItem(row1,4,new QTableWidgetItem(cmd.value(4).toString()));
                    row1++;

                }
                Send_OpenDoor(ID,0);
            }
            //找得到，则是出库
            else if(n == 1){
                // 获取当前日期和时间
                QDateTime currentDateTime = QDateTime::currentDateTime();

                // 将日期和时间转换为字符串
                QString currentDateTimeString = currentDateTime.toString("yyyy-MM-dd hh:mm:ss");


                // 将字符串转换为 QDateTime 对象
                QDateTime dateTime = QDateTime::fromString(in_t, "yyyy-MM-dd hh:mm:ss");

                // 将日期和时间转换为时间戳
                qint64 in_time = dateTime.toSecsSinceEpoch();
                qint64 out_time = currentDateTime.toSecsSinceEpoch();

                // 将时间戳转换为 QDateTime 对象
                QDateTime dateTime1 = QDateTime::fromSecsSinceEpoch(in_time);
                QDateTime dateTime2 = QDateTime::fromSecsSinceEpoch(out_time);

                // 计算时间差
                qint64 diffSeconds = dateTime1.secsTo(dateTime2); // 获取秒数差

                //计算钱
                int money=0;
                if(diffSeconds<=3600)
                {
                   money=0;
                }else if(diffSeconds>3600 && diffSeconds<=3600*24)
                {
                    if(diffSeconds%3600)
                        money = ((diffSeconds/3600)+1)*3;
                    else
                        money = (diffSeconds/3600)*3;
                }else if(diffSeconds>3600*24)
                {
                    money = (diffSeconds/3600)*2;
                }
                if(money >= 60)
                {
                    money=60;
                }


                QString sql = QString("update rfid set out_time='%1',money='%3' where id='%2';").arg(currentDateTimeString).arg(ID).arg(money);

                //将ID卡数据写入数据库
                cmd.exec(sql);

                //更新表格
                int row1=0;
                //清除tableWidget数据
                ui->tableWidget->clear();
                //设置表头
                QStringList head;
                head<<"卡号" << "入场时间" << "出场时间" << "金额" << "车牌号";
                ui->tableWidget->setHorizontalHeaderLabels(head);
                //设置行数
                ui->tableWidget->setRowCount(row);
                cmd.exec("select * from rfid;");
                while (cmd.next()) {

                    //设置TableWidget中的内容
                    ui->tableWidget->setItem(row1,0,new QTableWidgetItem(cmd.value(0).toString()));
                    ui->tableWidget->setItem(row1,1,new QTableWidgetItem(cmd.value(1).toString()));
                    ui->tableWidget->setItem(row1,2,new QTableWidgetItem(cmd.value(2).toString()));
                    ui->tableWidget->setItem(row1,3,new QTableWidgetItem(cmd.value(3).toString()));
                    ui->tableWidget->setItem(row1,4,new QTableWidgetItem(cmd.value(4).toString()));
                    row1++;

                }

                Send_OpenDoor(ID,1);

            }
            //禁止识别
            idflag = 1;
            //2秒后才可识别
            id_timer->start(2000);

        }
    }

}

MainWindow::~MainWindow()
{
    delete ui;
}

//手动添加ID卡
void MainWindow::on_pushButton_clicked()
{
    db.setDatabaseName("my.db"); //设置远程数据库文件名
    db.open();
    if(ui->pushButton->text() == "添加")
    {
        //添加一行数据
        ui->tableWidget->insertRow(row++);

        ui->pushButton->setText("更新数据库");
    }else if(ui->pushButton->text() == "更新数据库")
    {
        int  row  =  ui->tableWidget->rowCount() - 1 ;
        QString  id = "XXXX";
        QString  out_time = "未出库";
        QString      money = "0";
        // 获取当前日期和时间
        QDateTime currentDateTime = QDateTime::currentDateTime();

        // 将日期和时间转换为字符串
        QString currentDateTimeString = currentDateTime.toString("yyyy-MM-dd hh:mm:ss");
        //获取表中的数据
        id = ui->tableWidget->item(row,0)->text();


        //将获取的数据添加进数据库
        QSqlQuery  cmd(db);
        QString sql = QString("insert into  rfid(id,in_time,out_time,money)  values('%1','%2','%3','%4');").arg(id).arg(currentDateTimeString).arg(out_time).arg(money);

        if(cmd.exec(sql))
        {
          qDebug() << "插入成功";
        }

        //更新表格
        int row1=0;
        cmd.exec("select * from rfid;");
        //清除tableWidget数据
        ui->tableWidget->clear();
        //设置表头
        QStringList head;
        head<<"卡号" << "入场时间" << "出场时间" << "金额" << "车牌号";
        ui->tableWidget->setHorizontalHeaderLabels(head);
        //设置行数
        ui->tableWidget->setRowCount(++row);
        while (cmd.next()) {

            //设置TableWidget中的内容
            ui->tableWidget->setItem(row1,0,new QTableWidgetItem(cmd.value(0).toString()));
            ui->tableWidget->setItem(row1,1,new QTableWidgetItem(cmd.value(1).toString()));
            ui->tableWidget->setItem(row1,2,new QTableWidgetItem(cmd.value(2).toString()));
            ui->tableWidget->setItem(row1,3,new QTableWidgetItem(cmd.value(3).toString()));
            ui->tableWidget->setItem(row1,4,new QTableWidgetItem(cmd.value(4).toString()));
            row1++;

        }

        Send_OpenDoor(id,0);
        ui->pushButton->setText("添加");
    }
}

//删除一行数据
void MainWindow::on_pushButton_2_clicked()
{
    //获取活跃项
    int CurrentRow = ui->tableWidget->currentRow();

    if(CurrentRow > 0)
    {
        //获取活跃项的ID
        QString str = ui->tableWidget->item(CurrentRow,0)->text();

        //条件搜索ID,找到要删除的那一项
        db.setDatabaseName("my.db"); //设置远程数据库文件名
        db.open();
        QSqlQuery  cmd(db);
        QString sql = QString("select * from rfid where id = '%1';").arg(str);
        cmd.exec(sql);
        while(cmd.next())
        {            
            // 指定要删除的图片路径
            //删除入库图片
            //获取入库时间
            QString carinTime = ui->tableWidget->item(CurrentRow,1)->text();
            //通过入库时间找到要删除的那一项
            db.setDatabaseName("my.db"); //设置远程数据库文件名
            db.open();
            QSqlQuery  cmd3(db);
            QString sql = QString("select * from rfid where in_time='%1';").arg(carinTime);
            cmd3.exec(sql);
            while(cmd3.next())
            {
                //获取入库图片的路径
                QString carin = cmd3.value(5).toString();
                QString imagePath = QString("E:/Qt_CarStop/Server/priture/%1").arg(carin);

                // 创建QFile对象
                QFile file(imagePath);

                // 删除入库文件
                if (file.remove()) {
                    qDebug() << "Image deleted successfully";
                } else {
                    qDebug() << "Failed to delete image";
                }
                //删除出库图片
                QString stro = ui->tableWidget->item(CurrentRow,2)->text();
                if(stro != "未出库")
                {
                    //获取出库图片的路径
                    QString carout = cmd3.value(6).toString();
                    QString imagePath1 = QString("E:/Qt_CarStop/Server/priture/%1").arg(carout);
                    // 创建QFile对象
                    QFile file1(imagePath1);

                    // 删除出库文件
                    if (file1.remove()) {
                        qDebug() << "Image deleted successfully";
                    } else {
                        qDebug() << "Failed to delete image";
                    }
                }
            }



        }


        //删除对应的项
        db.setDatabaseName("my.db"); //设置远程数据库文件名
        db.open();
        QSqlQuery  cmd0(db);
        //获取活跃项的入库时间
        QString CurrentTime = ui->tableWidget->item(CurrentRow,1)->text();
        QString sql1 = QString("delete from rfid where in_time = '%1';").arg(CurrentTime);
        cmd0.exec(sql1);

        //获取表格数据的总行数
        int row1=0;
        cmd0.exec("select * from rfid;");
        //清除tableWidget数据
        ui->tableWidget->clear();
        //设置表头
        QStringList head;
        head<<"卡号" << "入场时间" << "出场时间" << "金额" << "车牌号";
        ui->tableWidget->setHorizontalHeaderLabels(head);
        //设置行数
        ui->tableWidget->setRowCount(--row);
        while (cmd0.next()) {

            //设置TableWidget中的内容
            ui->tableWidget->setItem(row1,0,new QTableWidgetItem(cmd0.value(0).toString()));
            ui->tableWidget->setItem(row1,1,new QTableWidgetItem(cmd0.value(1).toString()));
            ui->tableWidget->setItem(row1,2,new QTableWidgetItem(cmd0.value(2).toString()));
            ui->tableWidget->setItem(row1,3,new QTableWidgetItem(cmd0.value(3).toString()));
            ui->tableWidget->setItem(row1,4,new QTableWidgetItem(cmd0.value(4).toString()));
            row1++;

        }
    }
}
//查找数据
void MainWindow::on_pushButton_4_clicked()
{
    Dcheck->show();

}

//退出登录
void MainWindow::on_pushButton_3_clicked()
{
    r->show();
    this->hide();
}

//获取车牌所在ROI区域--车牌定位
bool Get_License_ROI(Mat src, License &License_ROI)
{
    Mat gray;
    cvtColor(src, gray, COLOR_BGR2GRAY);

    Mat thresh;
    threshold(gray, thresh, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);

    //使用形态学开操作去除一些小轮廓
    Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3));
    Mat open;
    morphologyEx(thresh, open, MORPH_OPEN, kernel);

    //使用 RETR_EXTERNAL 找到最外轮廓
    vector<vector<Point>>contours;
    findContours(open, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    vector<vector<Point>>conPoly(contours.size());
    for (unsigned long long i = 0; i < contours.size(); i++)
    {
        double area = contourArea(contours[i]);
        double peri = arcLength(contours[i], true);
        //根据面积筛选出可能属于车牌区域的轮廓
        if (area > 1000)
        {
            //使用多边形近似，进一步确定车牌区域轮廓
            approxPolyDP(contours[i], conPoly[i], 0.02*peri, true);

            if (conPoly[i].size() == 4)
            {
                //计算矩形区域宽高比
                Rect box = boundingRect(contours[i]);
                double ratio = double(box.width) / double(box.height);
                if (ratio > 2 && ratio < 4)
                {
                    //截取ROI区域
                    Rect rect = boundingRect(contours[i]);
                    License_ROI = { src(rect),rect };
                }
            }
        }
    }

    if (License_ROI.mat.empty())
    {
        return false;
    }
    return true;
}

//接收视频数据
void MainWindow::myudp()
{
    QByteArray buff;
    //根据可读数据来设置空间大小
    buff.resize(udp->bytesAvailable());
    //获取数据
    udp->readDatagram(buff.data(),buff.size());
    //将QByteArray转化为QBuffer
    QBuffer buffer(&buff);
    //解压缩jpeg
    QImageReader reader(&buffer,"jpeg");
    //将QByteArray转化为QImage
    QImage image;
    image.loadFromData(buff);
    //将QImage转化为QPixmap
    QPixmap pic = QPixmap::fromImage(image);
    //显示视频,并自适应窗口大小
    ui->label_7->setPixmap(pic.scaled(ui->label_7->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

    SUM++;
    //400毫秒判断一次
    if(SUM == 10)
    {
        SUM = 0;
        if(carflag == 0)
        {
            // 保存为jpeg格式，设置覆盖同名文件的选项
            pic.save("image.jpg", "JPEG", -1);
            //读取图片
            Mat src = imread("image.jpg");
            if (src.empty())
            {
                cout << "No image!" << endl;

                return;
            }
            //寻找车牌
            License License_ROI;
            if (Get_License_ROI(src, License_ROI))
            {
                //文字识别
                char text[1024]={0};
                ocr->OCR_TextRecognition("./","image.jpg",text);
                QString str0 = QString("%1").arg(text);

                //判断获取的文字是否为空
                if(str0.size()>0)
                {
                    //判断文字是否合法
                    for(int i = 0; i < 34;i++)
                    {
                        if(str0.at(0) == provinces[i] && (str0.length() == 7 || str0.length() == 8))
                        {
                            //禁止识别
                            carflag = 1;

                            //将找到车牌的图片显示出来
                            QImage image1(src.data, src.cols, src.rows, src.step, QImage::Format_RGB888);
                            QPixmap pixmap = QPixmap::fromImage(image1);

                            QString str1;
                            //处理'.'和空格" "
                            if(str0.length() == 8)
                            {
                               str1 = QString("%1%2%3%4%5%6%7").arg(str0.at(0)).arg(str0.at(1)).arg(str0.at(3)).arg(str0.at(4)).arg(str0.at(5)).arg(str0.at(6)).arg(str0.at(7));

                            }else
                            {
                                str1 = str0;
                            }


                            //查看出库还是入库
                            db.setDatabaseName("my.db"); //设置远程数据库文件名
                            db.open();
                            QSqlQuery  cmd(db);
                            QString str = QString("select * from rfid where car='%1';").arg(str1);
                            //查找Car
                            int n1 = 0;
                            QString in_t;
                            cmd.exec(str);
                            while (cmd.next()) {
                                in_t = cmd.value(1).toString();
                                if(cmd.value(2) == "未出库")
                                    n1++;
                            }
                            //入库
                            if(n1 == 0)
                            {
                                //更新表
                                // 获取当前日期和时间
                                QDateTime currentDateTime = QDateTime::currentDateTime();

                                // 将日期和时间转换为字符串
                                QString currentDateTimeString = currentDateTime.toString("yyyy-MM-dd hh:mm:ss");
                                QString currentDateTimeString1= currentDateTime.toString("yyyyMMddhhmmss");
                                //虚拟ID卡号
                                // 生成一个范围在0到999999999+1之间的随机数
                                int randomNum = QRandomGenerator::global()->bounded(999999999);

                                //保存图片
                                QString Car_Path = QString("E:/Qt_CarStop/Server/priture/%1.jpg").arg(currentDateTimeString1);
                                QString pic_path = QString("%1.jpg").arg(currentDateTimeString1);

                                pic.save(Car_Path);

                                QString sqls = QString("insert into rfid(id,in_time,out_time,money,car,in_path) values('%1','%2','未出库',0,'%3','%4');")
                                                        .arg(randomNum).arg(currentDateTimeString).arg(str1).arg(pic_path);


                                db.setDatabaseName("my.db"); //设置远程数据库文件名
                                db.open();
                                QSqlQuery  cmd(db);
                                cmd.exec(sqls);

                                //更新表格
                                int row1=0;
                                cmd.exec("select * from rfid;");
                                //清除tableWidget数据
                                ui->tableWidget->clear();
                                //设置表头
                                QStringList head;
                                head<<"卡号" << "入场时间" << "出场时间" << "金额" << "车牌号";
                                ui->tableWidget->setHorizontalHeaderLabels(head);
                                //设置行数
                                ui->tableWidget->setRowCount(++row);
                                while (cmd.next()) {

                                    //设置TableWidget中的内容
                                    ui->tableWidget->setItem(row1,0,new QTableWidgetItem(cmd.value(0).toString()));
                                    ui->tableWidget->setItem(row1,1,new QTableWidgetItem(cmd.value(1).toString()));
                                    ui->tableWidget->setItem(row1,2,new QTableWidgetItem(cmd.value(2).toString()));
                                    ui->tableWidget->setItem(row1,3,new QTableWidgetItem(cmd.value(3).toString()));
                                    ui->tableWidget->setItem(row1,4,new QTableWidgetItem(cmd.value(4).toString()));
                                    row1++;

                                }


                                Send_OpenDoor(str1,0);
                            }
                            else if(n1 == 1)
                            {
                                // 获取当前日期和时间
                                QDateTime currentDateTime = QDateTime::currentDateTime();

                                // 将日期和时间转换为字符串
                                QString currentDateTimeString = currentDateTime.toString("yyyy-MM-dd hh:mm:ss");
                                QString currentDateTimeString1= currentDateTime.toString("yyyyMMddhhmmss");

                                // 将字符串转换为 QDateTime 对象
                                QDateTime dateTime = QDateTime::fromString(in_t, "yyyy-MM-dd hh:mm:ss");

                                // 将日期和时间转换为时间戳
                                qint64 in_time = dateTime.toSecsSinceEpoch();
                                qint64 out_time = currentDateTime.toSecsSinceEpoch();

                                // 将时间戳转换为 QDateTime 对象
                                QDateTime dateTime1 = QDateTime::fromSecsSinceEpoch(in_time);
                                QDateTime dateTime2 = QDateTime::fromSecsSinceEpoch(out_time);

                                // 计算时间差
                                qint64 diffSeconds = dateTime1.secsTo(dateTime2); // 获取秒数差

                                //计算钱
                                int money=0;
                                if(diffSeconds<=3600)
                                {
                                   money=0;
                                }else if(diffSeconds>3600 && diffSeconds<=3600*24)
                                {
                                    if(diffSeconds%3600)
                                        money = ((diffSeconds/3600)+1)*3;
                                    else
                                        money = (diffSeconds/3600)*3;
                                }else if(diffSeconds>3600*24)
                                {
                                    money = (diffSeconds/3600)*2;
                                }
                                if(money >= 60)
                                {
                                    money=60;
                                }

                                //保存图片
                                QString Car_Path = QString("E:/Qt_CarStop/Server/priture/%1.jpg").arg(currentDateTimeString1);
                                QString pic_path = QString("%1.jpg").arg(currentDateTimeString1);

                                pic.save(Car_Path);


                                QString sql = QString("update rfid set out_time='%1',out_path='%3',money='%4' where car='%2' and out_time='未出库';").arg(currentDateTimeString).arg(str1).arg(pic_path).arg(money);

                                //将ID卡数据写入数据库
                                cmd.exec(sql);

                                //更新表格
                                int row1=0;
                                //清除tableWidget数据
                                ui->tableWidget->clear();
                                //设置表头
                                QStringList head;
                                head<<"卡号" << "入场时间" << "出场时间" << "金额" << "车牌号";
                                ui->tableWidget->setHorizontalHeaderLabels(head);
                                //设置行数
                                ui->tableWidget->setRowCount(row);
                                cmd.exec("select * from rfid;");
                                while (cmd.next()) {

                                    //设置TableWidget中的内容
                                    ui->tableWidget->setItem(row1,0,new QTableWidgetItem(cmd.value(0).toString()));
                                    ui->tableWidget->setItem(row1,1,new QTableWidgetItem(cmd.value(1).toString()));
                                    ui->tableWidget->setItem(row1,2,new QTableWidgetItem(cmd.value(2).toString()));
                                    ui->tableWidget->setItem(row1,3,new QTableWidgetItem(cmd.value(3).toString()));
                                    ui->tableWidget->setItem(row1,4,new QTableWidgetItem(cmd.value(4).toString()));
                                    row1++;

                                }

                                Send_OpenDoor(str1,1);
                            }

                            //3秒内不能进行文字识别，以免车还未开走就刷到出库
                            car_timer->start(3000);
                            return;
                        }
                    }
                    ui->label_5->setText("没有对应的车牌号");
                 }
            }
            else
            {
                return;
            }
        }
    }


}

void MainWindow::Set_CarFlag()
{
    //恢复识别
    carflag = 0;
    //关闭定时器
    car_timer->stop();
}

void MainWindow::Set_IDFlag()
{
    //恢复识别
    idflag = 0;
    //关闭定时器
    id_timer->stop();
}

//手动出库
void MainWindow::on_pushButton_5_clicked()
{
    //获取表中的数据
    int id_row = ui->tableWidget->currentItem()->row();
    QString id = ui->tableWidget->item(id_row,0)->text();
    QString out_t = ui->tableWidget->item(id_row,2)->text();
    if(out_t == "未出库")
    {
        // 获取当前日期和时间
        QDateTime currentDateTime = QDateTime::currentDateTime();

        // 将日期和时间转换为字符串
        QString currentDateTimeString = currentDateTime.toString("yyyy-MM-dd hh:mm:ss");

        QString sql = QString("update rfid set out_time='%1' where id='%2';").arg(currentDateTimeString).arg(id);
        db.setDatabaseName("my.db"); //设置远程数据库文件名
        db.open();
        QSqlQuery  cmd(db);
        //将ID卡数据写入数据库
        cmd.exec(sql);

        //更新表格
        int row1=0;
        //清除tableWidget数据
        ui->tableWidget->clear();
        //设置表头
        QStringList head;
        head<<"卡号" << "入场时间" << "出场时间" << "金额" << "车牌号";
        ui->tableWidget->setHorizontalHeaderLabels(head);
        //设置行数
        ui->tableWidget->setRowCount(row);
        cmd.exec("select * from rfid;");
        while (cmd.next()) {

            //设置TableWidget中的内容
            ui->tableWidget->setItem(row1,0,new QTableWidgetItem(cmd.value(0).toString()));
            ui->tableWidget->setItem(row1,1,new QTableWidgetItem(cmd.value(1).toString()));
            ui->tableWidget->setItem(row1,2,new QTableWidgetItem(cmd.value(2).toString()));
            ui->tableWidget->setItem(row1,3,new QTableWidgetItem(cmd.value(3).toString()));
            ui->tableWidget->setItem(row1,4,new QTableWidgetItem(cmd.value(4).toString()));
            row1++;

        }

        Send_OpenDoor(id,1);
    }
    else
    {
        ui->statusbar->showMessage("已出库");
    }
}


void MainWindow::on_tableWidget_currentItemChanged(QTableWidgetItem *current, QTableWidgetItem *previous)
{

}

//通过点击表中的项来查看对应的车辆信息
void MainWindow::on_tableWidget_itemClicked(QTableWidgetItem *item)
{
    //获取活跃行
    int Car_Row = item->row();
    //获取活跃行的ID
    QString Car_id = ui->tableWidget->item(Car_Row,0)->text();

    //寻找对应的图片路径
    QString str = QString("select * from rfid where id='%1';").arg(Car_id);
    db.setDatabaseName("my.db"); //设置远程数据库文件名
    db.open();
    QSqlQuery  cmd(db);
    QSqlQuery  cmd1(db);
    cmd.exec(str);

    // 遍历结果并输出
    while (cmd.next()) {

        //获取活跃行的出库时间
        QString Car_out_time = ui->tableWidget->item(Car_Row,2)->text();
        //通过出库时间来查找指定的图片路径
        QString str1 = QString("select * from rfid where out_time='%1';").arg(Car_out_time);
        cmd1.exec(str1);
        while (cmd1.next()) {

            QString in_path = cmd1.value(5).toString();
            QString out_path = cmd1.value(6).toString();

            QString IN_Path = QString("E:/Qt_CarStop/Server/priture/%1").arg(in_path);
            QString OUT_Path = QString("E:/Qt_CarStop/Server/priture/%1").arg(out_path);

            // 加载入库图片
            QPixmap pixmap(IN_Path);
            // 设置图片到标签
            ui->label_3->setPixmap(pixmap);
            // 调整标签大小以适应图片
            ui->label_3->setScaledContents(true);

            //如果车辆已经出库了
            if(Car_out_time != "未出库")
            {
                // 加载出库图片
                QPixmap pixmap1(OUT_Path);
                // 设置图片到标签
                ui->label_4->setPixmap(pixmap1);
                // 调整标签大小以适应图片
                ui->label_4->setScaledContents(true);

                //显示出库车牌号
                QString Car_InCode = cmd1.value(4).toString();
                ui->label_6->setText(Car_InCode);

                //显示入库车牌号
                QString Car_OutCode = cmd1.value(4).toString();
                ui->label_5->setText(Car_OutCode);

                //显示出入库时间
                QString IN_Time = cmd1.value(1).toString();
                ui->label->setText(IN_Time);
                QString OUT_Time = cmd1.value(2).toString();
                ui->label_2->setText(OUT_Time);
             }
            //没出库清楚标签内容
            else
            {
                ui->label_4->clear();
                ui->label_6->clear();
                ui->label_2->clear();

                //显示入库车牌号
                QString Car_OutCode = cmd1.value(4).toString();
                ui->label_5->setText(Car_OutCode);

                //显示出入库时间
                QString IN_Time = cmd1.value(1).toString();
                ui->label->setText(IN_Time);
            }
            return;
        }
    }
}
