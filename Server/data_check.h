#ifndef DATA_CHECK_H
#define DATA_CHECK_H

#include <QDialog>
#include <QSqlDatabase> //数据库管理器
#include <QSqlQuery>    //命令管理器
namespace Ui {
class Data_Check;
}

//数据库管理器
static QSqlDatabase db;

class Data_Check : public QDialog
{
    Q_OBJECT

public:
    explicit Data_Check(QWidget *parent = nullptr);
    ~Data_Check();

private slots:
    void on_lineEdit_textChanged(const QString &arg1);

    void on_comboBox_currentTextChanged(const QString &arg1);

    void on_tableView_clicked(const QModelIndex &index);

private:
    Ui::Data_Check *ui;
};

#endif // DATA_CHECK_H
