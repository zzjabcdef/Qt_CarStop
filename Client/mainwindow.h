#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "form.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();



public slots:
    void Read_RFID();
    void Send_ReadRFID();
    void myserver();
    void Show_Time();
    void show_frame();
    void Show_Advertising();
signals:
    void Mplayer_Stop();
public slots:
    void MplayerDisplay_Stop();

private:
    Ui::MainWindow *ui;
    Form *f;

    // QWidget interface
protected:
    void paintEvent(QPaintEvent *event);

};

class myWidget:public QWidget
{
    Q_OBJECT
public:
    myWidget(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags()):QWidget(parent,f){}



    // QWidget interface
protected:
    void paintEvent(QPaintEvent *event)override;


};

#endif // MAINWINDOW_H
