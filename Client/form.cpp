#include "form.h"
#include "ui_form.h"

//#include <QApplication>
//#include <QMediaPlayer>
//#include <QVideoWidget>


Form::Form(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Form)
{
    ui->setupUi(this);

//    QMediaPlayer *player = new QMediaPlayer;
//    QVideoWidget *videoWidget = new QVideoWidget;
//    player->setVideoOutput(videoWidget);

//    // 替换为你要播放的视频文件路径
//    player->setMedia(QUrl::fromLocalFile("video.mp4"));

//    videoWidget->show();
//    player->play();




}

Form::~Form()
{
    delete ui;
}
