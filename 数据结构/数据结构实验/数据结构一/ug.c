#define UGUG_EXPORTS
#include "ug_tools.h"
#include "ug.h"


static ug_uid_t* ug_uids;
static ug_uid_t* ug_freeuids;
static ug_uid_t* ug_tmpuids;

unsigned char   	      ug_macaddr[UG_MACADDR_BUF_SIZE];
static pthread_key_t      ug_key;
static ug_atomic_t        ug_lock = 0;

extern int ug_ncpu;

static void ug_init();
static void ug_exit();

#ifdef _GNUC_
void _attribute_((constructor))ug_init();
void _attribute_((destructor)) ug_exit();
#else
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		ug_init();
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		ug_exit();
		break;
	}
	return TRUE;
}

#endif


//程序初始化时调用，只能调用一次
//获得网卡的mac地址
//返回值:0:成功，非0：错误码
void ug_init()
{
	int 		rtn;
	ug_uid_t* c, * next;
	int         i;

	rtn = ug_getmac();
	if (rtn)
	{
		return;
	}

	//创建全局数组链表变量并初始化，ug_uids = calloc(...)
	/*
		添加语句
	*/
	ug_uids = (ug_uid_t*)calloc(128, sizeof(ug_uid_t));
	if (!ug_uids)
	{
		return;
	}
	i = 128;
	next = NULL;
	c = ug_uids;

	do
	{
		i--;
		c[i].ptime = 0;
		c[i].counter = (uint16_t)-1;
		c[i].bt = 0;
		memset(c[i].uid, 0, sizeof(c[i].uid));
		c[i].next = next;
		next = &c[i];
	} while (i);

	ug_freeuids = ug_uids;

	ug_ncpu = ug_getcpus();

	//创建一个key: ug_key, 使用ug_thread_key_create
	ug_key = ug_thread_key_create(&ug_key, NULL);
	if (rtn)
	{
		return;
	}
	return;
}

//程序退出时调用一次，且只调用一次
void ug_exit()
{
	ug_thread_key_delete(ug_key);

	ug_uid_t* p, * next;

	for (p = ug_tmpuids; p;)
	{
		next = p->next;
		free(p);
		p = next;
	}

	if (ug_uids)
	{
		free(ug_uids);
	}
}

//设置mac地址给p->uid
static inline void ug_uid_set_mac(ug_uid_t* p)
{
	/*
	编程实现：

	*/
	for (int i = 0; i < UG_MACADDR_BUF_SIZE; i++)
	{
		p->uid[i] = ug_macaddr[i];
	}
}

//设置线程ID，调用GetCurrentThreadId函数设置到p->uid的相应位置，最多16位
//使用单目运算符
static inline void ug_uid_set_tid(ug_uid_t* p)
{
	/*编程实现：

	*/
#ifdef _GNUC_
	* ((uint16_t*)&p->uid[6]) = (uint16_t)pthread_self();
#else
	* ((uint16_t*)&p->uid[6]) = (uint16_t)(GetCurrentThreadId() & 15);
#endif
}

//设置递增的序列号，设置到p->uid的相应位置
//使用单目运算符
static inline void ug_uid_set_counter(ug_uid_t* p)
{

	/*编程实现：

	*/
	*(uint16_t*)(&p->uid[8]) = p->counter;
}

//设置序列号、时间、回拨位，到p->uid的相应位置;
//使用单目运算符
static inline void ug_uid_set_value(ug_uid_t* p)
{
	/*编程实现：

	*/
	uint64_t* p64 = (uint64_t*)&p->uid[8];
	*(uint16_t*)p64 = p->counter;
	*p64 |= (p->ptime << 16);
	*p64 |= (((uint64_t)p->bt) << 62);
}

//设置时间到p->uid的相应位置;
//使用单目运算符
static inline void ug_uid_set_time(unsigned char* uid, time_t cur)
{
	/*编程实现：

	*/
	uint64_t* p = (uint64_t*)&uid[8];
	*p &= 0xFFFF;
	*p |= (cur << 16);
}


/*
获得uid,16字节
返回值:NULL失败，否则为unsigned char [16]
*/
UG_API unsigned char* UGSTDCALL  ug_getuid()
{
	ug_uid_t* p;
	time_t        cur;
	int           rst;
	unsigned char uid[16] = "0";


	p = (ug_uid_t*)ug_thread_get_specific(ug_key);
	if (!p)
	{
		ug_spinlock(&ug_lock, 1, 2048);
		/* 编程实现：
		//从全局数组链表ug_freeuids取头，并将指针放入当前线程的key索引指向的内存

		*/
		p = ug_freeuids;
		ug_freeuids = ug_freeuids->next;
		if (!p)
		{
			ug_unlock(&ug_lock);
			return -1;
		}
		p->next = ug_uids;
		ug_uids = p;
		ug_memory_barrier();
		ug_unlock(&ug_lock);

		cur = p->ptime = ug_getreltime();
		//		printf("p->time:%I64X\n", p->ptime);
		ug_uid_set_mac(p);
		ug_uid_set_tid(p);
		ug_uid_set_time(p->uid, p->ptime);

		rst = ug_thread_set_specific(ug_key, p);
		if (rst)
		{
			return 0;
		}
	}
	else
	{
		cur = ug_getreltime();
	}
	if (cur == p->ptime)
	{
		if ((uint16_t)0xFFFF == p->counter)
		{
			p->counter = 0;
		}
		else
		{
			p->counter++;
		}
		ug_uid_set_counter(p);
	}
	else if (cur > p->ptime)
	{
		p->ptime = cur;
		p->counter = 0;
		ug_uid_set_value(p);
	}
	else
	{
		p->ptime = cur;
		p->counter = 0;
		p->bt = (++p->bt) & 3;
		ug_uid_set_value(p);
	}
	/*
		1、比较上次时间和当前时间
		2、设置递增计数器p->counter
		3、设置时间（比较上次时间和当前时间，若时间回拨则需修改回拨位p->bt）p->ptime

		编程实现：
	*/
	*(uint64_t*)&uid[0] = *(uint64_t*)&p->uid[0];
	*(uint64_t*)&uid[8] = *(uint64_t*)&p->uid[8];
	return p->uid;
}