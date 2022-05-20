/**************************************************************************
## 文件名:ug_tools.h
## Copyright (c) 2019-2030 金证科技股份公司研发中心
## 版 本:0.0.0.1
## 创建人:田雪
## 日期/时间:2021-04-22, 16:26
## 描 述:


---------------------------更新记录-----------------------------
## 修改人:田雪
## 日期/时间:
## 描 述:
**********************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>

#pragma warning(disable:4996)
#include <winsock2.h>
#include <windows.h>
#include <IPHlpApi.h>
#include <sys/timeb.h>
#include <intrin.h>

#pragma comment(lib, "IPHlpApi.lib")


#define ug_base_time_ms      1577808000ll //2020.1.1,00:00:00
#define UG_MACADDR_BUF_SIZE   8
#define UG_MACADDR_SIZE       6

typedef unsigned char  u_char;

typedef __int16   			int16_t;
typedef unsigned __int16	uint16_t;
typedef __int32   			int32_t;
typedef unsigned __int32	uint32_t;
typedef __int64   			int64_t;
typedef unsigned __int64	uint64_t;


typedef struct ug_uid_s  ug_uid_t;

#pragma pack(push, 4)
struct ug_uid_s
{
	time_t          ptime;   //前一个41位毫秒,自2020.1.1, 00:00:00
	uint16_t        counter; //在当前时间下的递增的序列号
	unsigned char   bt;      //回拨位,2bits
	unsigned char   uid[16]; //uid
	ug_uid_t* next;
};

#define ug_memory_barrier()    	 _ReadWriteBarrier()
#define ug_cpu_pause()       	  SwitchToThread()

typedef int64_t 					ug_atomic_int_t;
typedef uint64_t					ug_atomic_uint_t;
typedef volatile ug_atomic_uint_t	ug_atomic_t;

#define ug_atomic_cmp_set(lock, old, set)                                    \
((ug_atomic_uint_t) InterlockedCompareExchange64((LONG64 volatile *) lock, set, old) == old)

typedef uint32_t   pthread_key_t;

time_t ug_getreltime();
int ug_getmac();
int ug_getcpus();
int ug_thread_key_create(pthread_key_t* key_ptr, void (*destr_func)(void*));
int ug_thread_key_delete(pthread_key_t key);
int ug_thread_set_specific(pthread_key_t key, const void* pointer);
void* ug_thread_get_specific(pthread_key_t key);
void ug_spinlock(ug_atomic_t* lock, ug_atomic_int_t value, ug_atomic_uint_t spin);

#define ug_trylock(lock)  (*(lock) == 0 && ug_atomic_cmp_set(lock, 0, 1))
#define ug_unlock(lock)    *(lock) = 0


#define ug_min(x, y)  (x > y) ? (y) : (x)

