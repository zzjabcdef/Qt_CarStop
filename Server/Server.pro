QT       += core gui sql network

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
    data_check.cpp \
    main.cpp \
    maintest.cpp \
    mainwindow.cpp \
    register.cpp

HEADERS += \
    data_check.h \
    maintest.h \
    mainwindow.h \
    register.h

FORMS += \
    data_check.ui \
    mainwindow.ui \
    register.ui

#WINDOS OpenCV 环境
#添加头文件
INCLUDEPATH += E:\OpenCV\include
INCLUDEPATH += E:\OpenCV\include\opencv2
#添加库文件
LIBS += E:\OpenCV\x64\mingw\lib\libopencv_*.a

#添加OCR头文件
INCLUDEPATH += E:\OcrLib\win-CLIB-CPU-x64\include
#添加OCR库文件
LIBS +=-LE:\OcrLib\win-CLIB-CPU-x64\bin  -lRapidOcrNcnn

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
