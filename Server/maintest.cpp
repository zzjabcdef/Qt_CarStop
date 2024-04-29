#include "maintest.h"
#include <stdarg.h> //windows&linux for va_start
#include <OcrLiteCApi.h>
#include <stdexcept>
#include <cstdlib>
#ifdef _WIN32
#include <windows.h>
#endif

//RaPid OCR
int OCR(const char *path,const char *name,char * text) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    //gpuIndex: -1 means use cpu, 0 means use gpu0, 1 means use gpu1...
    //OCR初始化
    OCR_HANDLE handle = OcrInit(DET_MODEL, CLS_MODEL, REC_MODEL, KEY_FILE, THREAD_NUM,0);
    if (!handle) {
        printf("cannot initialize the OCR Engine.\n");
        return -1;
    }

    OCR_PARAM param = {0};
    //识别文字
    OCR_BOOL bRet = OcrDetect(handle, path, name, &param);
    if (bRet) {
        //文字长度
        int nLen = OcrGetLen(handle);
        if (nLen > 0) {
            //给文字申请堆空间
            char *szInfo = (char *) malloc(nLen);
            if (szInfo) {
                if (OcrGetResult(handle, szInfo, nLen)) {
//                    printf("%s", szInfo);
                    //将文字拷贝到数组中
                    sprintf(text,"%s",szInfo);
                }
                //释放堆空间
                free(szInfo);
            }
        }
    }
    if (handle) {
        OcrDestroy(handle);
    }
    return 0;
}

OCR::OCR()
{
    //gpuIndex: -1 means use cpu, 0 means use gpu0, 1 means use gpu1...
    //OCR初始化
    handle = OcrInit(DET_MODEL, CLS_MODEL, REC_MODEL, KEY_FILE, THREAD_NUM,0);
    if (!handle) {
        printf("cannot initialize the OCR Engine.\n");

    }
}

OCR::~OCR()
{

    OcrDestroy(handle);

}

int OCR::OCR_TextRecognition(const char *path, const char *name, char *text)
{
    OCR_PARAM param = {0};
    //识别文字
    OCR_BOOL bRet = OcrDetect(handle, path, name, &param);
    if (bRet) {
        //文字长度
        int nLen = OcrGetLen(handle);
        if (nLen > 0) {
            //给文字申请堆空间
            char *szInfo = (char *) malloc(nLen);
            if (szInfo) {
                if (OcrGetResult(handle, szInfo, nLen)) {
                    //将文字拷贝到数组中
                    sprintf(text,"%s",szInfo);
                }
                //释放堆空间
                free(szInfo);
            }
        }
    }
}
