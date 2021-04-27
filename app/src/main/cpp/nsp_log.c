#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include "unistd.h"
#include <errno.h>
#include "nsp_log.h"

#define MAX_STRING_LEN 2048

static int getCurTime(char *strTime)
{
    char outstr[64];
    time_t t;
    struct tm *tmp;
    size_t timeLen = 0;

    t = time(NULL);
    tmp = localtime(&t);
    timeLen = strftime(outstr, sizeof(outstr), "%Y-%m-%d %H:%M:%S", tmp);

    memcpy(strTime, outstr, strlen(outstr));
    strTime[strlen(outstr)] = '\0';

    return timeLen;
}

#ifdef NSP_LOG_DEBUG
#ifdef NSP_LOG_FILE
__attribute((visibility("hidden"))) int nsp_log(const char *func_name,
    int line, const char *fmt, ...)
{
    FILE *fp = NULL;
    char str[MAX_STRING_LEN];
    int strLen = 0;
    int tmpStrLen = 0;
    char tmpStr[64];
    va_list args;

    fp = fopen(NSP_LOG_FILE_PATH, "a");
    if (fp == NULL)
        return -1;

    va_start(args, fmt);

    memset(tmpStr, 0, 64);
    getCurTime(tmpStr);
    memset(str, 0, MAX_STRING_LEN);
    tmpStrLen = sprintf(str, "[%s] ", tmpStr);
    strLen += tmpStrLen;

    tmpStrLen = sprintf(str + strLen, "[func:%s] ", func_name);
    strLen += tmpStrLen;

    tmpStrLen = sprintf(str + strLen, "[line:%d] ", line);
    strLen += tmpStrLen;

    vsprintf(str + strLen, fmt, args);

    va_end(args);

    fprintf(fp, "%s\n", str);

    fclose(fp);
    return 0;
}
#endif 	//NSP_LOG_FILE

#ifdef NSP_LOG_TERM
__attribute((visibility("hidden"))) int nsp_log(const char *func_name, int line,
    const char *fmt, ...)
{
    va_list args;
    char tmpStr[64];

    memset(tmpStr, 0, 64);
    getCurTime(tmpStr);

    va_start(args, fmt);
    printf("[%s]", tmpStr);
    printf("[func:%s][line:%d]", func_name, line);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");

    return 0;
}
#endif 	//NSP_LOG_TERM
#endif 	//NSP_LOG_DEBUG

#ifdef NSP_LOG_DEBUG
#ifdef NSP_LOG_FILE
__attribute((visibility("hidden"))) int nsp_print(char *info,
    unsigned char *data, int data_len)
{
    FILE *fp = NULL;
    int i;

    fp = fopen(NSP_LOG_FILE_PATH, "a");
    if (fp == NULL)
        return -1;

    fprintf(fp, "\n%s:", info);
    for (i = 0; i < data_len; i++)
    {
        if (i % 16 == 0)
            fprintf(fp, "\n\t");
        fprintf(fp, "%02x ", data[i]);
    }
    fprintf(fp, "\n\n");

    fclose(fp);
    return 0;
}
#endif 	//NSP_LOG_FILE

#ifdef NSP_LOG_TERM
__attribute((visibility("hidden"))) int nsp_print(char *info,
    unsigned char *data, int data_len)
{
    int i;

    printf("\n%s:", info);
    for (i = 0; i < data_len; i++)
    {
        if (i % 16 == 0)
            printf("\n\t");
        printf("%02x ", data[i]);
    }
    printf("\n");

    return 0;
}
#endif 	//NSP_LOG_TERM
#endif 	//NSP_LOG_DEBUG

#ifdef NSP_LOG_ERR
#ifdef NSP_LOG_FILE
__attribute((visibility("hidden"))) int nsp_err(const char *func_name,
    int line, const char *err_info, int err_code)
{
    char str_time[64];
    FILE *fp = NULL;

    fp = fopen(NSP_LOG_FILE_PATH, "a");
    if (fp == NULL)
        return -1;

    getCurTime(str_time);
    fprintf(fp, "[%s][func:%s][line:%d][err:%d]%s\n", str_time, func_name,
        line, err_code, err_info);

    fclose(fp);
    return 0;
}
#else
__attribute((visibility("hidden"))) int nsp_err(const char *func_name, int line,
    const char *err_info, int err_code)
{
    char str_time[64];
    getCurTime(str_time);
    printf("[%s][func:%s][line:%d][err:%d]%s\n", str_time, func_name, line,
        err_code, err_info);

    return 0;
}
#endif
#endif 	//NSP_LOG_ERR
