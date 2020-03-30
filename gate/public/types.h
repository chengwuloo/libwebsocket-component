
#ifndef LSMTYPES_HEADER__
#define LSMTYPES_HEADER__

#include <sys/types.h>
#include <string.h>

#define TRUE			(1)
#define FALSE			(0)

#ifndef NULL
#define NULL			(0)
#endif//NULL

#define S_OK			(0)
#define S_FALSE			(-1)

#define IN			// nothing.
#define OUT			// nothing.

#define SOCKET_ERROR		(-1)

#define SPLIT_SLASH		TEXT("/")

// define the special asset item.
#define ASSERT(a)

// Minimum and maximum macros
#define __max(a,b)  (((a) > (b)) ? (a) : (b))
#define __min(a,b)  (((a) < (b)) ? (a) : (b))

// define the special memory move function value item.
#define CopyMemory(dst,src,n)		memmove(dst,src,n)

// count the special array size of the special type item content value.
#define CountStringBuffer(str)  ((UINT)((lstrlen(str)+1)*sizeof(TCHAR)))
#define CountArray(Array) 		(sizeof(Array)/sizeof(Array[0]))

// free the special interface.
#define SafeRelease(pObject) { if (pObject!=NULL) { pObject->Release(); pObject=NULL; } }

// delete the special pointer.
#define SafeDelete(pData) { try { delete pData; } catch (...) { assert(false); } pData=NULL; } 

// close the special handler.
#define SafeCloseHandle(hHandle) { if (hHandle!=NULL) { close(hHandle); hHandle=NULL; } }

// delete the special array.
#define SafeDeleteArray(pData) { try { delete [] pData; } catch (...) { assert(false); } pData=NULL; } 


// declare the interface type.
#define interface		struct

typedef char			int8;
typedef short			int16;
typedef int    			int32;
typedef unsigned char	uint8;
typedef unsigned char   UCHAR;
typedef unsigned char   byte;
typedef unsigned char   BYTE;
typedef unsigned short  word;
typedef unsigned short  WORD;
typedef unsigned short	uint16;
typedef unsigned int    UINT;
typedef unsigned int  	uint32;
typedef int				SOCKET;

typedef float			FLOAT;
typedef double			DOUBLE;

typedef char			CHAR;
typedef char			TCHAR;
typedef char*			LPSTR;
typedef const char*     LPCSTR;
typedef char*			LPTSTR;
typedef const char*		LPCTSTR;
typedef uint8*			LPBYTE;
typedef WORD 			*PWORD,*LPWORD;
typedef long long		LONGLONG;
typedef unsigned long   ULONG64;
typedef long			HANDLE;
typedef int* 			PLONG;
typedef int				INT;
typedef int				LONG;
typedef int				BOOL;
typedef unsigned int*   PULONG;
typedef unsigned int    ULONG;
typedef unsigned int	DWORD;
typedef unsigned int*	PDWORD,*LPDWORD;
typedef void*			HINSTANCE;

typedef int64_t			__int64;
typedef int				COLORREF;

typedef char			WCHAR;
typedef char* 			LPWSTR;
typedef const char* 	LPCWSTR;
typedef void*			LPVOID;
typedef void			VOID;

typedef unsigned int    WPARAM;
typedef unsigned int    LPARAM;
typedef unsigned int   LRESULT;
typedef unsigned int      HWND;

// declare the others
#ifndef TEXT
#define TEXT(str)		str
#define   _T
#define MAX_PATH		300
#endif//TEXT

// define ATL convert.
#define USES_CONVERSION
#define A2CT
#define T2CA
#define A2T
#define CA2T
#define CT2A

// define the winapi.
#ifdef _WIN32
#else
#define WINAPI
#define CALLBACK
#endif//_WIN32

// define int max value item data.
#define LONG_MAX		(0x7FFFFFFF)

// define ZeroMemory function item now.
#define ZeroMemory(a,b)	memset(a,0,b)

// win32 string function value now.
#define lstrlen			strlen
#define lstrlenA		strlen
#define lstrlenW		strlen
#define lstrcpy			strcpy
#define lstrcpyn		strncpy
#define lstrcat			strcat
#define lstrcmpi		strcasecmp
#define lstrcmp			strcmp
#define _ttoi(a)		atoi(a)
#define _ttol(a)		strtol(a,0,10)
#define _tstol(a)		strtol(a,0,10)
#define _tstof(a)		strtof(a,NULL)
#define wsprintfA		sprintf
#define wsprintf		sprintf
#define _stprintf		sprintf
#define _vstprintf		vsprintf

#ifdef _WIN32
#define	_snprintf_t		_snprintf_s
#define	_sscanf_t		sscanf_s
#define	_64bitFormat	%I64u	
#else
#define _snprintf		snprintf
#define _sntprintf		snprintf
#define	_snprintf_t		snprintf
#define	_sscanf_t		sscanf
#define	_64bitFormat	%llu	
#define _vsntprintf		vsnprintf
#endif

// win32 file operation special
#define _tfopen			fopen

// declare load/free library item.
#define HINSTANCE		void*
#define LoadLibrary(a)	dlopen(a,RTLD_NOW)
#define GetProcAddress	dlsym
#define FreeLibrary		dlclose

// define the dll export now.
#define __declspec(dllexport)

// for windows sockets.
#define WSAGetLastError() 	errno

// zero the special memory operator content.
#define ZeroMemory(a,b)		memset(a,0,b)


#endif//LSMTYPES_HEADER__



