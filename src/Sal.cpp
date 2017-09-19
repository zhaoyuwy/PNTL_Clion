//
// Created by zy on 17-9-15.
//


#include <stdlib.h> // for malloc
#include <assert.h> // for assert
#include <semaphore.h>
#include <errno.h>
#include <pthread.h>

#include <unistd.h>

#include <iostream>

#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

#include "Sal.h"
#include "Log.h"

// Common


void sal_usleep(UINT32 usec)
{
    INT32 iRet = 0;
    // usleep()可能会因为进程收到信号(比如alarm)而提前返回EINTR, 后续优化
    iRet = usleep(usec);
    if (iRet)
        SAL_WARNING("Sleep: usleep failed[%d]: %s [%d]", iRet, strerror(errno), errno);
}

void sal_sleep(UINT32 sec)
{
    INT32 iRet = 0;
    // sleep()可能会因为进程收到信号(比如alarm)而提前返回EINTR, 后续优化
    iRet = sleep(sec);
    if (iRet)
        SAL_WARNING("Sleep: sleep failed[%d]: %s [%d]", iRet, strerror(errno), errno);
}


// 输入主机序ip二进制地址, 输出ip地址字符串, 字符串保存在每thread一份的静态内存中, 无需释放.
char *sal_inet_ntoa(UINT32 ip)
{
    struct in_addr in;
    in.s_addr = htonl(ip);
    return inet_ntoa(in);
}

// 输入ip地址字符串, 输出主机序ip二进制地址
UINT32 sal_inet_aton(const char * cp)
{
    struct in_addr in;
    inet_aton(cp, &in);

    return ntohl(in.s_addr);
}

void *
sal_memset(void *dst_void, INT32 val, size_t len)
{
    unsigned char *dst = (unsigned char *)dst_void;

    while (len--)
    {
        *dst++ = (unsigned char) val;
    }

    return dst_void;
}

INT32
sal_strlen(const char *s)
{
    const char *s_orig = s;

    while (*s != 0)
    {
        s++;
    }

    return (s - s_orig);
}

char *
sal_strncpy(char *dst, const char *src, size_t length)
{
    INT32 i = 0;
    char *dst_orig = dst;

    while ((*dst++ = *src++) != 0 && (i <= length))
    {
        ++i;
    }

    return dst_orig;
}

INT32
sal_strcmp(const char *s1, const char *s2)
{
    do
    {
        if (*s1 < *s2)
        {
            return -1;
        }
        else if (*s1 > *s2)
        {
            return 1;
        }
        s1++;
    }
    while (*s2++);

    return 0;
}

// mutex, semaphore
static
INT32
_sal_compute_timeout(struct timespec *ts, INT32 usec)
{
    INT32 sec;
    UINT32 nsecs;

    // CLOCK_REALTIME会受ntp干扰, 未来替换
    // clock_gettime(CLOCK_MONOTONIC, ts);
    clock_gettime(CLOCK_REALTIME, ts);

    /* Add in the delay */
    ts->tv_sec += usec / SECOND_USEC;

    /* compute new nsecs */
    nsecs = ts->tv_nsec + (usec % SECOND_USEC) * 1000;

    /* detect and handle rollover */
    if (nsecs < ts->tv_nsec)
    {
        ts->tv_sec += 1;
        nsecs -= SECOND_NSEC;
    }
    ts->tv_nsec = nsecs;

    /* Normalize if needed */
    sec = ts->tv_nsec / SECOND_NSEC;
    if (sec)
    {
        ts->tv_sec += sec;
        ts->tv_nsec = ts->tv_nsec % SECOND_NSEC;
    }

    /* indicate that we successfully got the time */
    return 1;
}


/*
 * recursive_mutex_t
 *
 *   This is an abstract type built on the POSIX mutex that allows a
 *   mutex to be taken recursively by the same thread without deadlock.
 *
 *   The Linux version of pthreads supports recursive mutexes
 *   (a non-portable extension to posix). In this case, we
 *   use the Linux support instead of our own.
 */

typedef struct recursive_mutex_s
{
    pthread_mutex_t mutex;
    const char      *desc;
} recursive_mutex_t;

sal_mutex_t
sal_mutex_create(const char *desc)
{
    recursive_mutex_t   *rm;
    pthread_mutexattr_t attr;

    if ((rm = (recursive_mutex_t *)malloc(sizeof (recursive_mutex_t))) == NULL)
    {
        return NULL;
    }

    sal_memset(rm,0,sizeof(recursive_mutex_t));

    rm->desc = desc;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);  //PI锁支持优先级继承和反转.

    pthread_mutex_init(&rm->mutex, &attr);
    return (sal_mutex_t) rm;
}

void
sal_mutex_destroy(sal_mutex_t m)
{
    recursive_mutex_t   *rm = (recursive_mutex_t *) m;

    assert(rm);
    pthread_mutex_destroy(&rm->mutex);

    free(rm);
}

INT32
sal_mutex_take(sal_mutex_t m, INT32 usec)
{
    recursive_mutex_t   *rm = (recursive_mutex_t *) m;
    INT32 err = 0;

    struct timespec ts;

    assert(rm);

    errno = 0;

    if (usec == sal_mutex_FOREVER)
    {
        do
        {
            err = pthread_mutex_lock(&rm->mutex);
        }
        while (err != 0 && errno == EINTR);
    }
    else if (_sal_compute_timeout(&ts, usec))
    {
        /* Treat EAGAIN as a fatal error on Linux */
        err = pthread_mutex_timedlock(&rm->mutex, &ts);
    }

    if (err)
    {
        SAL_WARNING("SAL: take mutex failed[%d]: %s [%d]", err, strerror(errno), errno);
        return -1;
    }

    return 0;
}

INT32
sal_mutex_give(sal_mutex_t m)
{
    recursive_mutex_t   *rm = (recursive_mutex_t *) m;
    INT32 err;

    assert(rm);
    err = pthread_mutex_unlock(&rm->mutex);

    return err ? -1 : 0;
}


// semaphore

/*
 * Wrapper class to hold additional info
 * along with the semaphore.
 */
typedef struct
{
    sem_t       s;
    const char        *desc;
    INT32         binary;
} wrapped_sem_t;

sal_sem_t
sal_sem_create(const char *desc, INT32 binary, INT32 initial_count)
{
    wrapped_sem_t *s = NULL;

    if ((s = (wrapped_sem_t *)malloc(sizeof (wrapped_sem_t))) == NULL)
    {
        return NULL;
    }

    /*
     * This is needed by some libraries with a bug requiring to zero sem_t before calling sem_init(),
     * even though this it is not required by the function description.
     * Threads using sem_timedwait() to maintain polling interval use 100% CPU if we not set the memory to zero SDK-77724
     */
    sal_memset(s, 0, sizeof(wrapped_sem_t));

    sem_init(&s->s, 0, initial_count);
    s->desc = desc;
    s->binary = binary;

    return (sal_sem_t) s;
}

void
sal_sem_destroy(sal_sem_t b)
{
    wrapped_sem_t *s = (wrapped_sem_t *) b;

    assert(s);

    sem_destroy(&s->s);

    free(s);
}

INT32
sal_sem_take(sal_sem_t b, INT32 usec)
{
    wrapped_sem_t   *s = (wrapped_sem_t *) b;
    INT32 err = 0;

    struct timespec ts;

    if (usec == sal_sem_FOREVER)
    {
        do
        {
            err = sem_wait(&s->s);
        }
        while (err != 0 && errno == EINTR);
    }
    else if (_sal_compute_timeout(&ts, usec))
    {
        while (1)
        {
            if (!sem_timedwait(&s->s, &ts))
            {
                err = 0;
                break;
            }
            if (errno != EAGAIN && errno != EINTR)
            {
                err = errno;
                break;
            }
        }
    }

    return err ? -1 : 0;
}

INT32
sal_sem_give(sal_sem_t b)
{
    wrapped_sem_t       *s = (wrapped_sem_t *) b;
    INT32                 err = 0;
    INT32                 sem_val = 0;

    /* Binary sem only post if sem_val == 0 */
    if (s->binary)
    {
        /* Post sem on getvalue failure */
        sem_getvalue(&s->s, &sem_val);
        if (sem_val == 0)
        {
            err = sem_post(&s->s);
        }
    }
    else
    {
        err = sem_post(&s->s);
    }
    return err ? -1 : 0;
}


// 替换pstrSrting中的所有pstrSrtingOld字符.
void sal_string_replace(string * pstrSrting, const string * pstrSrtingOld, const string * pstrSrtingNew)
{
    for (string::size_type pos(0); pos != string::npos; pos += pstrSrtingOld->length())
    {
        pos = pstrSrting->find((*pstrSrtingOld), pos);
        if ( pos != string::npos)
            pstrSrting->replace(pos, pstrSrtingOld->length(), (*pstrSrtingNew));
        else
            break;
    }
}
