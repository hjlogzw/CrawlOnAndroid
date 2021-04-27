#ifndef CAC_H
#define CAC_H

/* 错误码定义 */
#define CAC_SUCCESS            0
#define CAC_INVALID_PARAMETERS -1
#define CAC_FILE_ACCESS_FAIL   -2
#define CAC_MEMORY_ALLOC_FAIL  -3
#define CAC_FILE_TOO_SMALL     -4
#define CAC_DIR_CREATE_FAIL    -5

#ifdef __cplusplus
extern "C" {
#endif
    /*
    功能：	计算文件的摘要值
    参数：	@path[in]    文件路径
            @digest[out] 指向摘要值的指针
    返回值：CAC_SUCCESS          函数执行成功
            CAC_FILE_ACCESS_FAIL 文件访问失败
    */
    int calc_digest_of_file(const char *path, char *digest);

    /*
    功能：	获取文件大小
    参数：	@path[in]   文件路径
            @pSize[out] 存放文件大小
    返回值：CAC_SUCCESS            函数执行成功
            CAC_INVALID_PARAMETERS 函数参数无效
    */
    int get_file_size(const char *path, unsigned int *pSize);

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
        unsigned int max);

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
            CAC_DIR_CREATE_FAIL    目录创建失败
    */
    int file2slices(const char *inFilePath, const char *outDirPath,
        unsigned int slicesNum, float reductionRatio);

    /*
    功能：	从完整分片数据中提取数据段的数据，写入文件
    参数：	@slice[in]         完整分片数据
            @fout[in]          写文件指针
            @pIsWriten[in/out] 标识某个数据段是否已经被提取过
    返回值：CAC_SUCCESS            函数执行成功
            CAC_INVALID_PARAMETERS 函数参数无效
    */
    int write_from_slice(char *slice, FILE *fout, bool **pIsWriten);

    /*
    功能：	从完整分片数据中提取分片总数
    参数：	@slice[in] 完整分片数据
    返回值：分片总数
    */
    unsigned int get_count_of_slice(char *slice);

    /*
    功能：	从完整分片数据中提取原始文件的摘要值
    参数：	@slice[in]   完整分片数据
            @digest[out] 存放提取出的摘要值
    返回值：无返回值
    */
    void get_digest_of_slice(char *slice, char *digest);


#ifdef __cplusplus
}
#endif // C


#endif
