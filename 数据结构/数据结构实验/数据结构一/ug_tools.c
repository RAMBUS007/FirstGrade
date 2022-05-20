#include "ug_tools.h"

int ug_ncpu;
extern unsigned char ug_macaddr[];

void ug_spinlock(ug_atomic_t* lock, ug_atomic_int_t value, ug_atomic_uint_t spin)
{

	ug_atomic_uint_t  i, n;

	for (;; )
	{

		if (*lock == 0 && ug_atomic_cmp_set(lock, 0, value))
		{
			return;
		}

		if (ug_ncpu > 1)
		{

			for (n = 1; n < spin; n <<= 1)
			{

				for (i = 0; i < n; i++)
				{
					ug_cpu_pause();
				}

				if (*lock == 0 && ug_atomic_cmp_set(lock, 0, value))
				{
					return;
				}
			}
		}

		ug_cpu_pause();
	}

}
int ug_getmac()
{
	IP_ADAPTER_INFO* pAI;
	char* buffer = NULL;

	DWORD dwNeeded;

	DWORD dwRetVal = GetAdaptersInfo(NULL, &dwNeeded);
	if (dwRetVal == ERROR_SUCCESS || dwRetVal == ERROR_BUFFER_OVERFLOW)
	{
		buffer = (char*)calloc(dwNeeded, 1);
		if (!buffer)
		{
			return -1;
		}

		dwRetVal = GetAdaptersInfo((IP_ADAPTER_INFO*)buffer, &dwNeeded);
		if (dwRetVal == ERROR_SUCCESS)
		{
			pAI = (IP_ADAPTER_INFO*)buffer;
			//physical address
			if (pAI->AddressLength > 0)
			{
				memcpy(ug_macaddr, (char*)pAI->Address, min(UG_MACADDR_BUF_SIZE, pAI->AddressLength));
			}
			else
			{
				free(buffer);
				return -1;
			}
		}
		free(buffer);
		return 0;
	}


	return -1;

}

time_t ug_getreltime()
{

	struct _timeb timebuffer;

	_ftime_s(&timebuffer);

	return timebuffer.time * 1000 + timebuffer.millitm - ug_base_time_ms;

}

int ug_getcpus()
{
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	return sysinfo.dwNumberOfProcessors;
}

int ug_thread_key_create(pthread_key_t* key_ptr, void (*destr_func)(void*))
{
	DWORD dkey = TlsAlloc();
	if (dkey != TLS_OUT_OF_INDEXES)
	{
		*key_ptr = dkey;
		return 0;
	}
	else
	{
		return -1;
	}
}

int ug_thread_key_delete(pthread_key_t key)
{
	if (TlsFree(key))
	{
		return 0;
	}
	else
	{
		return -1;
	}

}

int ug_thread_set_specific(pthread_key_t key, const void* pointer)
{
	if (TlsSetValue(key, (LPVOID)pointer))
	{
		return 0;
	}
	else
	{
		return -1;
	}

}

void* ug_thread_get_specific(pthread_key_t key)
{
	return TlsGetValue(key);

}

