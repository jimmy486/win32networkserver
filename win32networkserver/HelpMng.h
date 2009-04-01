#ifndef _HELP_MNG_H_
#define _HELP_MNG_H_

#ifdef WIN32
#ifdef _DLL_EXPORTS_
#define DLLENTRY __declspec(dllexport)
#include <windows.h>
#else
#define DLLENTRY __declspec(dllimport)
#endif		//#ifdef _DLL_EXPORTS_
#else
#define DLLENTRY  
#endif		//#ifdef WIN32

namespace HelpMng
{
#ifdef __cplusplus
	extern "C" 
	{
#endif

		void DLLENTRY ReleaseAll();
#ifdef __cplusplus
	}
#endif
}

#endif			//#ifndef _HELP_MNG_H_