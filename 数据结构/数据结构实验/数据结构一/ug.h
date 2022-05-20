
#ifdef   __cplusplus
extern  "C" {
#endif

#ifndef  __UG_UG_H
#define  __UG_UG_H

#define UG_THREAD_NUM 128

#ifdef _WIN32
#include <windows.h>
#ifdef UGUG_EXPORTS
#define UG_API __declspec(dllexport)
#else
#define UG_API __declspec(dllimport)
#endif
#define UGSTDCALL WINAPI
#else
#define UG_API extern
#define UGSTDCALL
#endif

    //����ֵ:NULLʧ�ܣ�����Ϊunsigned char [16]��UUID
    UG_API unsigned char* UGSTDCALL  ug_getuid();

#endif
#ifdef   __cplusplus
}
#endif



