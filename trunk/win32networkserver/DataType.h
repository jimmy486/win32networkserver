#ifndef _DATA_TYPE_H
#define _DATA_TYPE_H

typedef unsigned long			ULONG;
typedef int						BOOL;
typedef char					CHAR;
typedef short					SHORT;
typedef long					LONG;
typedef const void*				LPCVOID;
typedef void*					LPVOID;
typedef int						INT;
typedef unsigned int			UINT;
typedef unsigned long			DWORD;
typedef unsigned char			BYTE;
typedef unsigned short			WORD;
typedef float					FLOAT;
typedef void *HANDLE;

#ifndef VOID
#define VOID void
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif


#ifndef WINVER				// 允许使用特定于 Windows XP 或更高版本的功能。
#define WINVER 0x0501		// 将此值更改为相应的值，以适用于 Windows 的其他版本。
#endif

#ifndef _WIN32_WINNT		// 允许使用特定于 Windows XP 或更高版本的功能。
#define _WIN32_WINNT 0x0501	// 将此值更改为相应的值，以适用于 Windows 的其他版本。
#endif						

#ifndef _WIN32_WINDOWS		// 允许使用特定于 Windows 98 或更高版本的功能。
#define _WIN32_WINDOWS 0x0410 // 将此值更改为适当的值，以指定将 Windows Me 或更高版本作为目标。
#endif

#ifndef _WIN32_IE			// 允许使用特定于 IE 6.0 或更高版本的功能。
#define _WIN32_IE 0x0600	// 将此值更改为相应的值，以适用于 IE 的其他版本。
#endif

#ifndef THROW_LINE
#define THROW_LINE (throw ((long)(__LINE__)))
#endif		//#ifndef THROW_LINE

#endif		//#ifndef _DATA_TYPE_H