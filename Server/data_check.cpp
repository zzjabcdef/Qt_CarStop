#include "data_check.h"
#include "ui_data_check.h"
#include <QDebug>

#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>
#include <QStandardItemModel>

#include "mainwindow.h"

//图表模型
QStandardItemModel  *model;

Data_Check::Data_Check(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Data_Check)
{
    ui->setupUi(this);

    //获取数据库的表格信息
    db = QSqlDatabase::addDatabase("QSQLITE");  //设置数据库软件类型
    db.setDatabaseName("my.db"); //设置远程数据库文件名
    db.open();   //打开数据库文件

    //构造数据库视图模型
    model = new QStandardItemModel;

    int row=0;
    //获取表格数据的总行数
    QSqlQuery  cmd(db);
    cmd.exec("select * from rfid;");
    while (cmd.next()) {

        //设置模型中的内容
        model->setItem(row,0,new QStandardItem(cmd.value(0).toString()));
        model->setItem(row,1,new QStandardItem(cmd.value(1).toString()));
        model->setItem(row,2,new QStandardItem(cmd.value(2).toString()));
        model->setItem(row,3,new QStandardItem(cmd.value(3).toString()));
        model->setItem(row,4,new QStandardItem(cmd.value(4).toString()));

        row++;

    }
}

Data_Check::~Data_Check()
{
    delete ui;
}

//搜索
void Data_Check::on_lineEdit_textChanged(const QString &arg1)
{
    //构造数据库视图模型
    model = new QStandardItemModel;
    int row=0;
    QSqlQuery  cmd(db);
    QString sql;
    //获取关键字
    QString str = ui->lineEdit->text();
    QString key = ui->comboBox->currentText();
    if(key == "已出的车")
        sql = QString("select * from rfid where out_time <> '00:00:00' and id like '%1%';").arg(str);
    if(key == "车牌号")
        sql = QString("select * from rfid where id like '%1%';").arg(str);
    if(key == "花费金额")
        sql = QString("select * from rfid where money like '%1%';").arg(str);
    if(key == "未出的车")
        sql = QString("select * from rfid where out_time = '00:00:00' and id like '%1%';").arg(str);
    cmd.exec(sql);
    while (cmd.next()) {

        //设置模型中的内容
        model->setItem(row,0,new QStandardItem(cmd.value(0).toString()));
        model->setItem(row,1,new QStandardItem(cmd.value(1).toString()));
        model->setItem(row,2,new QStandardItem(cmd.value(2).toString()));
        model->setItem(row,3,new QStandardItem(cmd.value(3).toString()));
        model->setItem(row,4,new QStandardItem(cmd.value(4).toString()));
        row++;

    }

    //设置表格的行与列
    model->setRowCount(row);
    model->setColumnCount(5);

    QStringList head;
    head<<"卡号" << "入场时间" << "出场时间" << "金额" << "车牌号";
    model->setHorizontalHeaderLabels(head);

    //把模型设置到视图中
    ui->tableView->setModel(model);

}

void Data_Check::on_comboBox_currentTextChanged(const QString &arg1)
{
    //构造数据库视图模型
    model = new QStandardItemModel;
    int row=0;
    QSqlQuery  cmd(db);
    QString sql;
    //获取关键字
    QString str = ui->lineEdit->text();
    QString key = ui->comboBox->currentText();
    if(key == "已出的车")
        sql = QString("select * from rfid where out_time <> '未出库' and car like '%1%';").arg(str);
    if(key == "车牌号")
        sql = QString("select * from rfid where car like '%1%';").arg(str);
    if(key == "花费金额")
        sql = QString("select * from rfid where money like '%1%';").arg(str);
    if(key == "未出的车")
        sql = QString("select * from rfid where out_time = '未出库' and car like '%1%';").arg(str);
    if(key == "ID卡号")
        sql = QString("select * from rfid where id like '%1%';").arg(str);
    cmd.exec(sql);
    while (cmd.next()) {

        //设置模型中的内容
        model->setItem(row,0,new QStandardItem(cmd.value(0).toString()));
        model->setItem(row,1,new QStandardItem(cmd.value(1).toString()));
        model->setItem(row,2,new QStandardItem(cmd.value(2).toString()));
        model->setItem(row,3,new QStandardItem(cmd.value(3).toString()));
        model->setItem(row,4,new QStandardItem(cmd.value(4).toString()));
        row++;

    }

    //设置表格的行与列
    model->setRowCount(row);
    model->setColumnCount(5);

    QStringList head;
    head<<"卡号" << "入场时间" << "出场时间" << "金额" << "车牌号";
    model->setHorizontalHeaderLabels(head);

    //把模型设置到视图中
    ui->tableView->setModel(model);

}

void Data_Check::on_tableView_clicked(const QModelIndex &index)
{
    //获取当前的活跃行
    QModelIndex index1 = ui->tableView->currentIndex();

    qDebug() << index1;

    int row1 =  index1.row();

    //获取表格的卡号
    QString id = model->item(row1,0)->text();

    QString in_time = model->item(row1,1)->text();
    //寻找对应的图片路径
    QString str3 = QString("select * from rfid where in_time='%1';").arg(in_time);
    db.setDatabaseName("my.db"); //设置远程数据库文件名
    db.open();
    QSqlQuery  cmd4(db);
    cmd4.exec(str3);

    // 遍历结果并输出
    while (cmd4.next()) {

        //获取活跃行的出库时间
        QString Car_out_time = model->item(row1,2)->text();
        //通过出库时间来查找指定的图片路径
        QString str1 = QString("select * from rfid where out_time='%1';").arg(Car_out_time);
        cmd4.exec(str1);
        while (cmd4.next()) {

            QString in_path = cmd4.value(5).toString();
            QString out_path = cmd4.value(6).toString();

            QString IN_Path = QString("E:/Qt_CarStop/Server/priture/%1").arg(in_path);
            QString OUT_Path = QString("E:/Qt_CarStop/Server/priture/%1").arg(out_path);

            // 加载入库图片
            QPixmap pixmap(IN_Path);
            // 设置图片到标签
            ui->label->setPixmap(pixmap);
            // 调整标签大小以适应图片
            ui->label->setScaledContents(true);

            //如果车辆已经出库了
            if(Car_out_time != "未出库")
            {
                // 加载出库图片
                QPixmap pixmap1(OUT_Path);
                // 设置图片到标签
                ui->label_2->setPixmap(pixmap1);
                // 调整标签大小以适应图片
                ui->label_2->setScaledContents(true);
             }
            //没出库清楚标签内容
            else
            {
                ui->label_2->clear();
            }
           return;
        }
    }
}
