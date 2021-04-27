//
// Created by AirForHj on 2021/3/8.
//
#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <math.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <ctype.h>
#include "cac.h"

extern "C" JNIEXPORT jint JNICALL
Java_com_example_android_1c_MainActivity_fileProcess(
        JNIEnv *env, jobject obj,
        jstring jInFilePth, jstring jOutFilePth,jstring jnewFile, jint jSliceNum, jfloat jreductionRatio) {

    const char *inFilePath = (char*)env->GetStringUTFChars(jInFilePth, 0);
    const char *outDirPath = (char*)env->GetStringUTFChars(jOutFilePth, 0);
    const char *newFile = (char*)env->GetStringUTFChars(jnewFile, 0);
    int slicesNum = jSliceNum;
    float reductionRatio = jreductionRatio;

    unsigned int sliceFileSize;
    unsigned int i;
    unsigned int selectedNum;      // 抽取的分片总数
    char slicePath[256];
    char *slice = NULL;
    unsigned int *selected = NULL; // 被选中的分片序号数组
    bool *isWriten = NULL;         // 存储数据段写入状态列表的指针
    FILE *fin = NULL;
    FILE *fout = NULL;

    int ret = file2slices(inFilePath, outDirPath, slicesNum, reductionRatio);

    /* 抽取30%的分片文件，还原原始文件 */
    selectedNum = (int)floor(slicesNum * 0.1);
    printf("selected slices count: %d\n", selectedNum);
    selected = (unsigned int *)malloc(sizeof(unsigned int) * selectedNum);
    gen_and_shuffle(selected, selectedNum, 0, slicesNum);
    fout = fopen(newFile, "wb");
    for(i = 0; i < selectedNum; i++)
    {
        sprintf(slicePath, "%s/%u.slice", outDirPath, selected[i]);
        get_file_size(slicePath, &sliceFileSize);
        fin = fopen(slicePath, "rb");
        slice = (char *)malloc(sizeof(char) * sliceFileSize);
        fread(slice, sizeof(char), sliceFileSize, fin);
        if(fin != NULL) fclose(fin);
        fin = NULL;
        ret = write_from_slice(slice, fout, &isWriten);
        if(ret != CAC_SUCCESS)
        {
            printf("write_from_slice fail: %d\n", ret);
            break;
        }
        if(slice != NULL) free(slice);
        slice = NULL;
    }
    printf("all write_from_slice ends\n");

    /* 释放内存 */
    if(fin != NULL) fclose(fin);
    fin = NULL;
    if(fout != NULL) fclose(fout);
    fout = NULL;
    if(slice != NULL) free(slice);
    slice = NULL;
    if(selected != NULL) free(selected);
    selected = NULL;
    if(isWriten != NULL) free(isWriten);
    isWriten = NULL;

    return ret;
}


