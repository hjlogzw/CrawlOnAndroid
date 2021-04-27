#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <unistd.h>

#include <ctype.h>
#include "nsp_log.h"
#include "cac.h"

/* 常量宏定义 */
#define PATH_MAX_LEN  256    // 路径最大长度
#define DIGEST_LEN    32     // 摘要长度
#define FILE_MIN_SIZE 1024   // 载体文件的最小长度为1KB
#define BLOCK_MAX_LEN 8192   // 读取文件的块大小
#define SLICE_MIN_NUM 10     // 分片数目最小值
#define SLICE_MAX_NUM 200     // 分片数目最大值
#define NEED_SLICE_MIN_NUM 2 // 还原所需要的最小分片数目

#define FREE(p) \
{\
    if((p) != NULL) free((p));\
    (p) = NULL;\
}

#define FCLOSE(fp) \
{\
    if((fp) != NULL) fclose((fp));\
    (fp) = NULL;\
}

#pragma pack(1) // 结构体对齐为1个字节

// 数据段标注结构体
typedef struct DataChunkHdr
{
    unsigned char hdrLen;		// 标注长度
    unsigned int num;			// 数据段总数，不应该超过50
    unsigned int seq;			// 数据段序号
    unsigned int totalLen;		// 文件总长度
    unsigned int offset;		// 文件块偏移
    unsigned int dataLen;		// 文件块长度
} stDataChunkHdr;
#define ST_CHUNKHDR_SIZE sizeof(stDataChunkHdr)

// 向量标注结构体
typedef struct VectorHdr
{
    unsigned char hdrLen;   // 标注长度
    unsigned int num;       // 向量总数
    unsigned int seq;       // 向量序号
    unsigned int chunksNum; // 数据段总数
} stVectorHdr;
#define ST_VECTORHDR_SIZE sizeof(stVectorHdr)

// 向量信息结构体
typedef struct VectorInfo
{
    stVectorHdr hdr;           // 向量标注
    unsigned int chunksSeqs[]; // 数据段序号数组
} stVectorInfo;

// 分片标注结构体
typedef struct SliceHdr
{
    unsigned char hdrLen;        // 标注长度
    unsigned int num;            // 分片总数
    unsigned int seq;            // 分片序号
    unsigned int vectorsNum;     // 向量总数
    unsigned int chunksNum;      // 从每个向量读取的数据段数目
    unsigned int chunksTotalNum; // 数据段总数
    char digest[DIGEST_LEN];     // 原始文件的摘要值
} stSliceHdr;
#define ST_SLICEHDR_SIZE sizeof(stSliceHdr)

// 分片信息结构体
typedef struct SliceInfo
{
    stSliceHdr hdr;              // 分片标注
    unsigned int chunksOffset[]; // 从每个向量读取的数据段在该向量的数据段序号数组中的偏移
} stSliceInfo;

#pragma pack() // 恢复缺省模式

/*
功能：	计算文件的摘要值
参数：	@path[in]    文件路径
        @digest[out] 指向摘要值的指针
返回值：CAC_SUCCESS          函数执行成功
        CAC_FILE_ACCESS_FAIL 文件访问失败
*/
int calc_digest_of_file(const char *path, char *digest)
{
    int i;
        int j;
        char md5str[33];
        char cmd[17 + PATH_MAX_LEN];
        FILE *ptr = NULL;

        sprintf(cmd, "md5sums.exe -u -e %s", path);
        ptr = popen(cmd, "r");
        if(ptr == NULL)
        {
            nsp_err(__FUNCTION__, __LINE__, "cannot read file",
                CAC_FILE_ACCESS_FAIL);
            return CAC_FILE_ACCESS_FAIL;
        }
        fgets(md5str, 33, ptr);
        pclose(ptr);
        ptr = NULL;
        i = 0;
        for(j = 0; j < 16; j++)
        {
            printf("md5str: %c%c\n", md5str[i], md5str[i + 1]);
            digest[j] = (md5str[i] - (isdigit(md5str[i])? '0': 'a' - 10)) << 4;
            digest[j] += md5str[i + 1] - (isdigit(md5str[i + 1])? '0': 'a' - 10);
            i += 2;
        }

    return CAC_SUCCESS;
}

/*
功能：	获取文件大小
参数：	@path[in]   文件路径
        @pSize[out] 存放文件大小
返回值：CAC_SUCCESS            函数执行成功
        CAC_INVALID_PARAMETERS 函数参数无效
*/
int get_file_size(const char *path, unsigned int *pSize)
{
    struct stat info;

    if (path == NULL || pSize == NULL)
    {
        nsp_err(__FUNCTION__, __LINE__, "invalid parameters",
            CAC_INVALID_PARAMETERS);
        return CAC_INVALID_PARAMETERS;
    }

    stat((const char*)path, &info);
    *pSize = info.st_size;

    return CAC_SUCCESS;
}

/*
功能：	生成固定数量的随机数，存放在arr指向的内存中，
        并确保随机数范围不小于arr的容量时，随机数不重复
参数：	@arr[in] 存放随机数的连续内存空间
        @len[in] 随机数数量
        @min[in] 随机数最小值为min + 1
        @max[in] 随机数最大值
返回值：CAC_SUCCESS            函数执行成功
        CAC_INVALID_PARAMETERS 函数参数无效
        CAC_MEMORY_ALLOC_FAIL  内存分配失败
*/
int gen_and_shuffle(unsigned int *arr, unsigned int len, unsigned int min,
    unsigned int max)
{
    unsigned int i;
    unsigned int j;
    unsigned int *tmp = NULL;

    if (arr == NULL || len == 0 || min > max)
    {
        nsp_err(__FUNCTION__, __LINE__, "invalid parameters",
            CAC_INVALID_PARAMETERS);
        return CAC_INVALID_PARAMETERS;
    }

    tmp = (unsigned int *)malloc(sizeof(unsigned int) * (max - min));
    if (tmp == NULL)
    {
        nsp_err(__FUNCTION__, __LINE__, "tmp malloc fail",
            CAC_MEMORY_ALLOC_FAIL);
        return CAC_MEMORY_ALLOC_FAIL;
    }
    for (i = min; i < max; i++) tmp[i - min] = i + 1;

    srand(time(NULL));
    for (j = 0; j < len; j++)
    {
        i = rand() % (max - min);
        arr[j] = tmp[i];
        tmp[i] = tmp[max - min - 1];
        max--;
    }

    FREE(tmp);
    return CAC_SUCCESS;
}

/*
功能：	将连续内存空间seqs在范围[s, e]内的数据翻转
参数：	@seqs[in] 连续内存空间
        @s[in]    翻转范围的起始偏移
        @e[in]    翻转范围的终止偏移
返回值：CAC_SUCCESS            函数执行成功
        CAC_INVALID_PARAMETERS 函数参数无效
        CAC_MEMORY_ALLOC_FAIL  内存分配失败
*/
static int reverse(unsigned int *seqs, unsigned int s, unsigned int e)
{
    unsigned int tmp;

    if (seqs == NULL)
    {
        nsp_err(__FUNCTION__, __LINE__, "invalid parameters",
            CAC_INVALID_PARAMETERS);
        return CAC_INVALID_PARAMETERS;
    }

    while (s < e)
    {
        tmp = seqs[s];
        seqs[s] = seqs[e];
        seqs[e] = tmp;
        s++;
        e--;
    }

    return CAC_SUCCESS;
}

/*
功能：	对连续内存空间seqs做循环左移
参数：	@seqs[in]   连续内存空间
        @len[in]    连续内存空间的大小
        @offset[in] 偏移量
返回值：无返回值
*/
static inline void seqs_offset(unsigned int *seqs, unsigned int len,
    unsigned int offset)
{
    reverse(seqs, 0, offset - 1);
    reverse(seqs, offset, len - 1);
    reverse(seqs, 0, len - 1);
}

/*
功能：	对连续内存空间data的前len项求和
参数：	@data[in]   连续内存空间
参数：	@len[in]    要求和的项数
返回值：求和结果
*/
static inline unsigned int sum(unsigned int *data, unsigned int len)
{
    unsigned int i;
    unsigned int sum;

    sum = 0;
    for (i = 0; i < len; i++) sum += data[i];
    return sum;
}

/*
功能：	构造向量信息列表
参数：	@pVectorInfoList[in] 存放向量信息的列表
        @vectorNum[in]       向量个数
        @chunksTotalNum[in]  数据段总数
返回值：CAC_SUCCESS            函数执行成功
        CAC_INVALID_PARAMETERS 函数参数无效
        CAC_MEMORY_ALLOC_FAIL  内存分配失败
*/
static int gen_vectorInfoList(stVectorInfo *pVectorInfoList,
    unsigned int vectorNum, unsigned int chunksTotalNum)
{
    int ret;
    unsigned int i;
    unsigned int j;
    unsigned int *offset = NULL;
    stVectorInfo *info = NULL;

    // 参数检查
    if (pVectorInfoList == NULL || vectorNum == 0 || chunksTotalNum == 0)
    {
        nsp_err(__FUNCTION__, __LINE__, "invalid parameters",
            CAC_INVALID_PARAMETERS);
        return CAC_INVALID_PARAMETERS;
    }

    /* 构造主本向量信息 */
    info = pVectorInfoList;
    info->hdr.hdrLen = ST_VECTORHDR_SIZE;
    info->hdr.num = vectorNum;
    info->hdr.seq = 0;
    info->hdr.chunksNum = chunksTotalNum;
    for (j = 0; j < chunksTotalNum; j++) info->chunksSeqs[j] = j;
    if (vectorNum == 1)
    {
        info = NULL;
        return CAC_SUCCESS;
    }

    /* 构造随机偏移数组，用于对除了主本向量之外的向量的数据段序号数组进行随机偏移 */
    offset = (unsigned int *)malloc(sizeof(unsigned int) * (vectorNum - 1));
    if (offset == NULL)
    {
        nsp_err(__FUNCTION__, __LINE__, "offset malloc fail",
            CAC_MEMORY_ALLOC_FAIL);
        return CAC_MEMORY_ALLOC_FAIL;
    }
    ret = gen_and_shuffle(offset, vectorNum - 1, 0, chunksTotalNum - 1);
    if (ret != CAC_SUCCESS)
    {
        nsp_err(__FUNCTION__, __LINE__, "gen_and_shuffle call fail",
            ret);
        FREE(offset);
        return ret;
    }

    /* 构造副本向量信息 */
    for (i = 1; i < vectorNum; i++)
    {
        info = (stVectorInfo *)((char *)info + ST_VECTORHDR_SIZE +
            sizeof(unsigned int) * chunksTotalNum);
        info->hdr.hdrLen = ST_VECTORHDR_SIZE;
        info->hdr.num = vectorNum;
        info->hdr.seq = i;
        info->hdr.chunksNum = chunksTotalNum;
        for (j = 0; j < chunksTotalNum; j++)
            info->chunksSeqs[j] = j;
        seqs_offset(info->chunksSeqs, chunksTotalNum, offset[i - 1]);
    }
    info = NULL;
    FREE(offset);

    return CAC_SUCCESS;
}

/*
功能：	构造分片信息列表
参数：	@pSliceInfoList[in] 存放分片信息的列表
        @slicesNum[in]      分片个数
        @vectorsNum[in]     向量个数
        @chunksNum[in]      从每个向量读取的数据段数目
        @chunksTotalNum[in] 数据段总数
        @digest[in]         原始文件的摘要值
返回值：CAC_SUCCESS            函数执行成功
        CAC_INVALID_PARAMETERS 函数参数无效
*/
static int gen_sliceInfoList(stSliceInfo *pSliceInfoList, unsigned int slicesNum,
    unsigned int vectorsNum, unsigned int chunksNum,
    unsigned int chunksTotalNum, char *digest)
{
    unsigned int i;
    unsigned int j;
    stSliceInfo *info = NULL;

    // 参数检查
    if (pSliceInfoList == NULL)
    {
        nsp_err(__FUNCTION__, __LINE__, "invalid parameters", CAC_INVALID_PARAMETERS);
        return CAC_INVALID_PARAMETERS;
    }

    /* 构造分片信息 */
    info = pSliceInfoList;
    for (i = 0; i < slicesNum; i++)
    {
        info->hdr.hdrLen = ST_SLICEHDR_SIZE;
        info->hdr.num = slicesNum;
        info->hdr.seq = i;
        info->hdr.vectorsNum = vectorsNum;
        info->hdr.chunksNum = chunksNum;
        info->hdr.chunksTotalNum = chunksTotalNum;
        memcpy(info->hdr.digest, digest, DIGEST_LEN);
        for (j = 0; j < vectorsNum; j++) info->chunksOffset[j] = i;
        info = (stSliceInfo *)((char *)info + ST_SLICEHDR_SIZE +
            sizeof(unsigned int) * vectorsNum);
    }
    info = NULL;

    return CAC_SUCCESS;
}

/*
功能：	构造一个完整分片
参数：	@pSlice[out]         存放构造好的完整分片数据
        @pSliceSize[out]     存放完整分片数据长度
        @pSliceInfo[in]      一个分片信息结构体指针
        @pVectorInfoList[in] 向量信息列表指针
        @fin[in]             读文件指针
        @size[in]            文件大小
        @chunksTotalNum[in]  数据段总数
        @chunkOffsets[in]    数据段偏移
返回值：CAC_SUCCESS            函数执行成功
        CAC_INVALID_PARAMETERS 函数参数无效
        CAC_MEMORY_ALLOC_FAIL  内存分配失败
*/
static int gen_slice(char **pSlice, unsigned int *pSliceSize,
    stSliceInfo *pSliceInfo, stVectorInfo *pVectorInfoList,
    FILE *fin, unsigned int size, unsigned int chunksTotalNum,
    unsigned int *chunkOffsets)
{
    unsigned int i;
    unsigned int offset;
    unsigned int seq;
    unsigned int chunkLen;
    unsigned int sliceSize;
    char *slice = NULL;
    stDataChunkHdr *pHdr = NULL;
    stVectorInfo *info = NULL;

    // 参数检查
    if (pSlice == NULL || pSliceSize == NULL || pSliceInfo == NULL
        || pVectorInfoList == NULL || fin == NULL || size == 0)
    {
        nsp_err(__FUNCTION__, __LINE__, "invalid parameters",
            CAC_INVALID_PARAMETERS);
        return CAC_INVALID_PARAMETERS;
    }

    /* 初始化分片，填充分片标注 */
    slice = (char *)malloc(ST_SLICEHDR_SIZE);
    if (slice == NULL)
    {
        nsp_err(__FUNCTION__, __LINE__, "slice malloc fail",
            CAC_MEMORY_ALLOC_FAIL);
        return CAC_MEMORY_ALLOC_FAIL;
    }
    memcpy(slice, &(pSliceInfo->hdr), ST_SLICEHDR_SIZE);
    sliceSize = ST_SLICEHDR_SIZE;

    /* 循环访问该分片从每一个向量中选取的数据段序号，以此构造数据段标准并从文件中读取数据 */
    for (i = 0; i < pSliceInfo->hdr.vectorsNum; i++)
    {
        info = (stVectorInfo *)((char *)pVectorInfoList
            + i * (ST_VECTORHDR_SIZE + sizeof(unsigned int) * chunksTotalNum));
        seq = info->chunksSeqs[pSliceInfo->chunksOffset[i]];
        offset = chunkOffsets[seq];
        chunkLen = chunkOffsets[seq + 1] - offset;
        sliceSize += ST_CHUNKHDR_SIZE + chunkLen;
        slice = (char *)realloc(slice, sliceSize);
        if (slice == NULL)
        {
            nsp_err(__FUNCTION__, __LINE__, "slice realloc fail",
                CAC_MEMORY_ALLOC_FAIL);
            return CAC_MEMORY_ALLOC_FAIL;
        }
        pHdr = (stDataChunkHdr *)(slice +
            sliceSize - chunkLen - ST_CHUNKHDR_SIZE);
        pHdr->hdrLen = ST_CHUNKHDR_SIZE;
        pHdr->num = chunksTotalNum;
        pHdr->seq = seq;
        pHdr->totalLen = size;
        pHdr->offset = offset;
        pHdr->dataLen = chunkLen;
        fseek(fin, offset, SEEK_SET);
        fread(slice + sliceSize - chunkLen, sizeof(char), chunkLen, fin);
    }
    *pSlice = slice;
    slice = NULL;
    *pSliceSize = sliceSize;
    info = NULL;
    pHdr = NULL;

    return CAC_SUCCESS;
}

/*
功能：	将文件拆分成若干分片文件，保存在指定目录中
参数：	@inFilePath[in]     文件路径
        @outDirPath[in]     目录路径
        @slicesNum[in]      分片总数
        @reductionRatio[in] 还原比例
返回值：CAC_SUCCESS            函数执行成功
        CAC_INVALID_PARAMETERS 函数参数无效
        CAC_FILE_ACCESS_FAIL   文件访问失败
        CAC_MEMORY_ALLOC_FAIL  内存分配失败
        CAC_FILE_TOO_SMALL     文件大小不合要求
*/
int file2slices(const char *inFilePath, const char *outDirPath,
    unsigned int slicesNum, float reductionRatio)
{
    int ret;
    unsigned int i;
    unsigned int sumData;
    unsigned int fileSize;                // 要读取的文件的大小
    unsigned int chunksTotalNum;          // 数据段总数
    unsigned int vectorsNum;              // 向量总数
    unsigned int sliceSize;               // 完整分片长度
    unsigned int aveChunkSize;            // 数据段平均长度
    unsigned int minChunkSize;            // 数据段最小长度
    unsigned int maxChunkSize;            // 数据段最大长度
    unsigned int lastChunkSize;           // 最后一个数据段的长度
    char slicePath[PATH_MAX_LEN];         // 单个分片文件的存储路径
    char digest[DIGEST_LEN];              // 原始文件的摘要值
    unsigned int *chunkSizes = NULL;      // 前chunksTotalNum -1个数据段的长度指针
    unsigned int *chunkOffsets = NULL;    // 数据段偏移指针
    char *slice = NULL;                   // 指向单个完整分片的指针
    FILE *fin = NULL;                     // 读文件指针
    FILE *fout = NULL;                    // 写文件指针
    stVectorInfo *pVectorInfoList = NULL; // 向量信息列表指针
    stSliceInfo *pSliceInfoList = NULL;   // 分片信息列表指针
    stSliceInfo *pSliceInfo = NULL;       // 分片信息指针

    /* 参数检查 */
    if (inFilePath == NULL || outDirPath == NULL || slicesNum > SLICE_MAX_NUM || (int)floor(reductionRatio * slicesNum) < NEED_SLICE_MIN_NUM)
    {
        nsp_err(__FUNCTION__, __LINE__, "invalid parameters",
            CAC_INVALID_PARAMETERS);
        return CAC_INVALID_PARAMETERS;
    }
    if (access(inFilePath, R_OK) == -1)
    {
        nsp_err(__FUNCTION__, __LINE__, "cannot read file",
            CAC_FILE_ACCESS_FAIL);
        return CAC_FILE_ACCESS_FAIL;
    }

    if (access(outDirPath, R_OK) == -1)
    {
        nsp_err(__FUNCTION__, __LINE__, "cannot read file",
                CAC_FILE_ACCESS_FAIL);
        return CAC_FILE_ACCESS_FAIL;
//        ret = mkdir(outDirPath, S_IXUSR); //S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH
//        if (ret != 0)
//        {
//            nsp_err(__FUNCTION__, __LINE__, "mkdir call fail",
//                CAC_DIR_CREATE_FAIL);
//            return CAC_DIR_CREATE_FAIL;
//        }
    }

    /* 初始化分片总数，并计算数据段总数和向量总数 */
    ret = get_file_size(inFilePath, &fileSize);
    if (ret != CAC_SUCCESS)
    {
        nsp_err(__FUNCTION__, __LINE__, "get_file_size call fail", ret);
        return ret;
    }
    if (fileSize < FILE_MIN_SIZE)
    {
        nsp_err(__FUNCTION__, __LINE__, "filesize is too small",
            CAC_FILE_TOO_SMALL);
        return CAC_FILE_TOO_SMALL;
    }
    /*
    if(slicesNum < SLICE_MIN_NUM || slicesNum > SLICE_MAX_NUM)
    {
        srand(time(NULL));
        slicesNum = rand() % (SLICE_MAX_NUM - SLICE_MIN_NUM + 1) + SLICE_MIN_NUM;
    }
    */
    chunksTotalNum = slicesNum; // 目前的策略是数据段总数与分片总数保持一致，分片仅对每个向量读取一个数据段
    vectorsNum = slicesNum + 1 - (int)floor(reductionRatio * slicesNum); // 所需最少向量数
    nsp_log(__FUNCTION__, __LINE__, "source file size: %d", fileSize);
    nsp_log(__FUNCTION__, __LINE__,
        "chunks count: %d, vectors count: %d, slices count: %d",
        chunksTotalNum, vectorsNum, slicesNum);

    /* 按照数据段个数随机切割原始文件，并记录每个数据段序号和偏移的对应关系 */
    chunkSizes = (unsigned int *)malloc(sizeof(unsigned int)
        * (chunksTotalNum - 1));
    if (chunkSizes == NULL)
    {
        nsp_err(__FUNCTION__, __LINE__, "chunkSizes malloc fail",
            CAC_MEMORY_ALLOC_FAIL);
        return CAC_MEMORY_ALLOC_FAIL;
    }
    aveChunkSize = round(fileSize / chunksTotalNum); // 最小平均数据段长度为20字节
    minChunkSize = floor(0.8 * aveChunkSize) - 1;
    maxChunkSize = ceil(1.2 * aveChunkSize) + 1;
    do
    {
        gen_and_shuffle(chunkSizes, chunksTotalNum - 1, minChunkSize,
            maxChunkSize);
        sumData = sum(chunkSizes, chunksTotalNum - 1);
        lastChunkSize = fileSize - sumData;
    } while (lastChunkSize < minChunkSize);

    chunkOffsets = (unsigned int *)malloc(sizeof(unsigned int)
        * (chunksTotalNum + 1));
    if (chunkOffsets == NULL)
    {
        nsp_err(__FUNCTION__, __LINE__, "chunkOffsets malloc fail",
            CAC_MEMORY_ALLOC_FAIL);
        FREE(chunkSizes);
        return CAC_MEMORY_ALLOC_FAIL;
    }
    chunkOffsets[0] = 0;
    chunkOffsets[chunksTotalNum] = fileSize;
    for (i = 1; i <= chunksTotalNum - 1; i++)
        chunkOffsets[i] = chunkOffsets[i - 1] + chunkSizes[i - 1];
    FREE(chunkSizes);

    /* 打印每个数据段的起始偏移和长度 */
    for (i = 0; i <= chunksTotalNum - 1; i++)
        nsp_log(__FUNCTION__, __LINE__, "chunk seq: %d, offset: %d, length: %d",
            i, chunkOffsets[i], chunkOffsets[i + 1] - chunkOffsets[i]);

    /* 构造向量信息列表 */
    pVectorInfoList = (stVectorInfo *)malloc((ST_VECTORHDR_SIZE +
        sizeof(unsigned int) * chunksTotalNum) * vectorsNum);
    if (pVectorInfoList == NULL)
    {
        nsp_err(__FUNCTION__, __LINE__, "pVectorInfoList malloc fail",
            CAC_MEMORY_ALLOC_FAIL);
        FREE(chunkOffsets);
        return CAC_MEMORY_ALLOC_FAIL;
    }
    ret = gen_vectorInfoList(pVectorInfoList, vectorsNum, chunksTotalNum);
    if (ret != CAC_SUCCESS)
    {
        nsp_err(__FUNCTION__, __LINE__, "gen_vectorInfoList fail", ret);
        FREE(pVectorInfoList);
        FREE(chunkOffsets);
        return ret;
    }
    nsp_log(__FUNCTION__, __LINE__, "gen_vectorInfoList success");

    /* 构造分片信息列表 */
    calc_digest_of_file(inFilePath, digest);
    nsp_print("digest", (unsigned char *)digest, DIGEST_LEN);
    pSliceInfoList = (stSliceInfo *)malloc((ST_SLICEHDR_SIZE +
        sizeof(unsigned int) * vectorsNum) * slicesNum);
    if (pSliceInfoList == NULL)
    {
        nsp_err(__FUNCTION__, __LINE__, "pSliceInfoList malloc fail",
            CAC_MEMORY_ALLOC_FAIL);
        FREE(pVectorInfoList);
        FREE(chunkOffsets);
        return CAC_MEMORY_ALLOC_FAIL;
    }
    ret = gen_sliceInfoList(pSliceInfoList, slicesNum,
        vectorsNum, 1, chunksTotalNum, digest);
    if (ret != CAC_SUCCESS)
    {
        nsp_err(__FUNCTION__, __LINE__, "gen_sliceInfoList fail", ret);
        FREE(pSliceInfoList);
        FREE(pVectorInfoList);
        FREE(chunkOffsets);
        return ret;
    }
    nsp_log(__FUNCTION__, __LINE__, "gen_sliceInfoList success");

    /* 对于每个分片信息，构造对应的完整分片，写入文件 */
    if ((fin = fopen(inFilePath, "rb")) == NULL)
    {
        nsp_err(__FUNCTION__, __LINE__, "cannot read file", CAC_FILE_ACCESS_FAIL);
        FREE(pSliceInfoList);
        FREE(pVectorInfoList);
        FREE(chunkOffsets);
        return CAC_FILE_ACCESS_FAIL;
    }
    pSliceInfo = pSliceInfoList;
    for (i = 0; i < slicesNum; i++)
    {
        ret = gen_slice(&slice, &sliceSize, pSliceInfo, pVectorInfoList,
            fin, fileSize, chunksTotalNum, chunkOffsets);
        if (ret != CAC_SUCCESS)
        {
            nsp_err(__FUNCTION__, __LINE__, "gen_slice fail", CAC_SUCCESS);
            FCLOSE(fin);
            FREE(slice);
            FREE(pSliceInfoList);
            FREE(pVectorInfoList);
            FREE(chunkOffsets);
            return CAC_SUCCESS;
        }
        sprintf(slicePath, "%s/%d.slice", outDirPath, i);
        if ((fout = fopen(slicePath, "wb")) == NULL)
        {
            nsp_err(__FUNCTION__, __LINE__, "cannot write file",
                CAC_FILE_ACCESS_FAIL);
            FCLOSE(fin);
            FREE(slice);
            FREE(pSliceInfoList);
            FREE(pVectorInfoList);
            FREE(chunkOffsets);
            return CAC_FILE_ACCESS_FAIL;
        }
        fwrite(slice, sizeof(char), sliceSize, fout);
        FCLOSE(fout);
        FREE(slice);
        pSliceInfo = (stSliceInfo *)((char *)pSliceInfo + ST_SLICEHDR_SIZE +
            sizeof(unsigned int) * vectorsNum);
        nsp_log(__FUNCTION__, __LINE__, "slice seq: %d  size: %.2lf MB", i,
            sliceSize / 1024 / 1024.0);
    }
    nsp_log(__FUNCTION__, __LINE__, "all gen_slice success");

    /* 释放内存 */
    FCLOSE(fout);
    FCLOSE(fin);
    FREE(slice);
    FREE(pSliceInfoList);
    FREE(pVectorInfoList);
    FREE(chunkOffsets);
    return CAC_SUCCESS;
}

/*
功能：	从完整分片数据中提取数据段的数据，写入文件
参数：	@slice[in]         完整分片数据
        @fout[in]          写文件指针
        @pIsWriten[in/out] 标识某个数据段是否已经被提取过
返回值：CAC_SUCCESS            函数执行成功
        CAC_INVALID_PARAMETERS 函数参数无效
*/
int write_from_slice(char *slice, FILE *fout, bool **pIsWriten)
{
    unsigned int i;
    unsigned int num;
    unsigned int sliceOff;
    stSliceHdr *sliceHdr = NULL;
    stDataChunkHdr *chunkHdr = NULL;

    // 参数检查
    if (slice == NULL || fout == NULL)
    {
        nsp_err(__FUNCTION__, __LINE__, "invalid parameters",
            CAC_INVALID_PARAMETERS);
        return CAC_INVALID_PARAMETERS;
    }

    /* 读取分片标注 */
    sliceHdr = (stSliceHdr *)slice;
    num = sliceHdr->vectorsNum * sliceHdr->chunksNum;
    if (*pIsWriten == NULL)
        *pIsWriten = (bool *)malloc(sizeof(bool) * sliceHdr->chunksTotalNum);
    for (i = 0; i < sliceHdr->chunksTotalNum; i++) (*pIsWriten)[i] = false;

    /* 循环读取数据段并写入文件 */
    sliceOff = sliceHdr->hdrLen;
    for (i = 0; i < num; i++)
    {
        chunkHdr = (stDataChunkHdr *)((char *)slice + sliceOff);
        if ((*pIsWriten)[chunkHdr->seq] == false) // 未访问过的偏移范围内数据才会被写入
        {
            fseek(fout, chunkHdr->offset, SEEK_SET);
            fwrite((char *)chunkHdr + chunkHdr->hdrLen,
                sizeof(char), chunkHdr->dataLen, fout);
            fflush(fout);
            (*pIsWriten)[chunkHdr->seq] = true;
        }
        sliceOff += chunkHdr->hdrLen + chunkHdr->dataLen;
    }

    return CAC_SUCCESS;
}

/*
功能：	从完整分片数据中提取分片总数
参数：	@slice[in] 完整分片数据
返回值：分片总数
*/
inline unsigned int get_count_of_slice(char *slice)
{
    return ((stSliceHdr *)slice)->num;
}

/*
功能：	从完整分片数据中提取原始文件的摘要值
参数：	@slice[in]   完整分片数据
        @digest[out] 存放提取出的摘要值
返回值：无返回值
*/
inline void get_digest_of_slice(char *slice, char *digest)
{
    memcpy(digest, ((stSliceHdr *)slice)->digest, DIGEST_LEN);
}
