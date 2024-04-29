#ifndef __MAIN_TEST_H__
#define __MAIN_TEST_H__
#include <stdarg.h> //windows&linux for va_start
#include <OcrLiteCApi.h>
#include <stdexcept>
#include <cstdlib>

#define DET_MODEL "E:/models/ch_PP-OCRv3_det_infer"
#define CLS_MODEL "E:/models/ch_ppocr_mobile_v2.0_cls_infer"
#define REC_MODEL "E:/models/ch_PP-OCRv3_rec_infer"
#define KEY_FILE  "E:/models/ppocr_keys_v1.txt"

#define THREAD_NUM 3

class OCR
{
public:
    explicit OCR();
    ~OCR();
    int OCR_TextRecognition(const char *path,const char *name,char * text);

private:
    OCR_HANDLE handle;
};



#endif //__MAIN_TEST_H__
