QT       += core gui serialport network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui


#添加opencv 的头文件   ARM-OpenCV 环境
INCLUDEPATH  += /opencv4-arm/include
INCLUDEPATH  += /opencv4-arm/include/opencv4
INCLUDEPATH  += /opencv4-arm/include/opencv4/opencv2

##添加OpenCV库文件
LIBS += -L/opencv4-arm/lib  -lopencv_world

#WINDOS OpenCV 环境
#添加头文件
#INCLUDEPATH += E:\OpenCV\include
#INCLUDEPATH += E:\OpenCV\include\opencv2
##添加库文件
#LIBS += E:\OpenCV\x64\mingw\lib\libopencv_*.a

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
