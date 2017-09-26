//
// Created by zy on 17-9-15.
//

#ifndef PNTL_AGENT_SAL_H
#define PNTL_AGENT_SAL_H


#include <string.h>
#include <string>

using namespace std;

#define MILLISECOND_USEC        (1000)
#define SECOND_USEC            (1000000)
#define MINUTE_USEC            (60 * SECOND_USEC)
#define HOUR_USEC            (60 * MINUTE_USEC)

#define SECOND_MSEC            (1000)
#define MINUTE_MSEC            (60 * SECOND_MSEC)

#define SECOND_NSEC     (SECOND_USEC * 1000)

#define SAL_USECS_TIMESTAMP_ROLLOVER    ((UINT32)(0xFFFFFFFF))

// 常用数据类型定义:

typedef char CHAR;

typedef signed char INT8;
typedef signed short INT16;
typedef signed int INT32;
typedef signed long long INT64;

typedef unsigned char UINT8;
typedef unsigned short UINT6;
typedef unsigned int UINT32;
typedef unsigned long long UINT64;



// 调用可能抛出异常的代码, 未被catch的异常会逐级上报, 最终导致进程abort退出.
#define CALL_WITH_EXECEPTION(exec) \
    ({   \
        INT32 __iRet__ = AGENT_OK;\
        try\
        {\
            exec;\
        }\
        catch (exception const & e)\
        {\
            JSON_PARSER_ERROR("Caught exception [%s] in [%s]", e.what(), #exec);\
            __iRet__ = AGENT_E_ERROR;\
        }\
        __iRet__;   \
    })


typedef struct sal_sem_s {
    CHAR sal_opaque_type;
} *sal_sem_t;

typedef struct sal_mutex_s {
    CHAR mutex_opaque_type;
} *sal_mutex_t;

#define sal_sem_FOREVER        (-1)

extern sal_sem_t sal_sem_create(const CHAR *desc, INT32 binary, INT32 initial_count);

extern void sal_sem_destroy(sal_sem_t b);

extern INT32 sal_sem_take(sal_sem_t b, INT32 usec);

extern INT32 sal_sem_give(sal_sem_t b);

#define sal_mutex_FOREVER    (-1)

extern sal_mutex_t sal_mutex_create(const CHAR *desc);

extern void sal_mutex_destroy(sal_mutex_t m);

extern INT32 sal_mutex_take(sal_mutex_t m, INT32 usec);

extern INT32 sal_mutex_give(sal_mutex_t m);


extern void *sal_memset(void *dst_void, INT32 val, size_t len);

extern CHAR *sal_inet_ntoa(UINT32 ip);

extern UINT32 sal_inet_aton(const CHAR *cp);

extern void sal_usleep(UINT32 usec);

extern void sal_sleep(UINT32 sec);

extern INT32 sal_strlen(const CHAR *s);

extern CHAR *sal_strncpy(CHAR *dst, const CHAR *src, size_t length);

extern INT32 sal_strcmp(const CHAR *s1, const CHAR *s2);

extern void sal_string_replace(string *pstrSrting, const string *pstrSrtingOld, const string *pstrSrtingNew);


#endif //PNTL_AGENT_SAL_H
