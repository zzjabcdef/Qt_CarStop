#include "register.h"
#include "ui_register.h"
#include "data_check.h"
#include <QDebug>
Register::Register(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Register)
{
    ui->setupUi(this);
    //设置背景颜色
    this->setStyleSheet("background-color: rgb(255,255,255);");
    //显示提示信息
    ui->lineEdit->setPlaceholderText("账号");
    ui->lineEdit_2->setPlaceholderText("密码");
    //密文输入
    ui->lineEdit_2->setEchoMode(QLineEdit::Password);


    db = QSqlDatabase::addDatabase("QSQLITE");  //设置数据库软件类型
    db.setDatabaseName("my.db"); //设置远程数据库文件名
    if(!db.open())   //打开数据库文件
    {
        qDebug() << "opencv sql err";
        return;
    }
    QSqlQuery  cmd1(db);
    //创建一个用户表格
    cmd1.exec("create table myregister(id primary key,pwd not null);");

    cmd1.exec("insert into myregister values('123','123')");
    QString sql = QString("select * from myregister;");
    if(cmd1.exec(sql))
    {
        db.setDatabaseName("my.db"); //设置远程数据库文件名
        db.open();
    }


}

Register::~Register()
{
    delete ui;
}

//点击登录
void Register::on_pushButton_clicked()
{
    QSqlQuery  cmd1(db);
    cmd1.exec("select * from myregister;");
    while (cmd1.next()) {

        if(ui->lineEdit->text() == cmd1.value(0).toString() && ui->lineEdit_2->text() == cmd1.value(1).toString())
        {
            //发送登录成功信号
            emit Register_Hide(ui->lineEdit->text(),ui->lineEdit_2->text());

        }
    }
}
