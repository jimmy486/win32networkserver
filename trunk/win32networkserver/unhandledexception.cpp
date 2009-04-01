#include "UnHandledException.h"
#include <atltime.h>
#include <windows.h>
#include <atlstr.h>
#include <vector>
#include <assert.h>
#include <imagehlp.h>
using namespace std;
#pragma comment (lib, "imagehlp.lib")
#pragma comment (lib, "version.lib")

#pragma warning (disable:4312)
#pragma warning (disable:4311)

namespace HelpMng
{
	extern inline void __stdcall GetCallStack(CONTEXT& ctStx, CString& strStack, DWORD dwLevel = 50);
	extern inline void __stdcall GetCallStack(CString& strStack, DWORD dwLevel = 50);


	// The API_VERSION_NUMBER define is 5 with the NT4 IMAGEHLP.H.  It is
	//  7 with the November Platform SDK version.  This seems to be the only
	//  reliable way to see which header is being used.
#if ( API_VERSION_NUMBER < 7 )
	typedef struct _IMAGEHLP_LINE
	{
		DWORD SizeOfStruct;           // set to sizeof(IMAGEHLP_LINE)
		DWORD Key;                    // internal
		DWORD LineNumber;             // line number in file
		PCHAR FileName;               // full filename
		DWORD Address;                // first instruction of line
	} IMAGEHLP_LINE, *PIMAGEHLP_LINE;
#endif  // API_VERSION_NUMBER < 7

#ifndef SYMOPT_LOAD_LINES
#define SYMOPT_LOAD_LINES	0x00000010
#endif  // SYMOPT_LOAD_LINES

#ifdef __cplusplus
	extern "C" 
	{
#endif

		LONG WINAPI MyExceptionFilter(_EXCEPTION_POINTERS *ExceptionInfo );

#ifdef __cplusplus
	}
#endif		//#ifdef __cplusplus

	static BOOL g_bSymIsInit = FALSE;	
	//LONG WINAPI MyExceptionFilter(_EXCEPTION_POINTERS *ExceptionInfo );

	static DWORD __stdcall GetModBase ( HANDLE hProcess , DWORD dwAddr )
	{
		// Check in the symbol engine first.
		IMAGEHLP_MODULE stIHM ;

		// This is what the MFC stack trace routines forgot to do so their
		//  code will not get the info out of the symbol engine.
		stIHM.SizeOfStruct = sizeof ( IMAGEHLP_MODULE ) ;

		if (SymGetModuleInfo (hProcess, dwAddr , &stIHM ) )
		{
			return ( stIHM.BaseOfImage ) ;
		}
		else
		{
			// Let's go fishing.
			MEMORY_BASIC_INFORMATION stMBI ;

			if ( 0 != VirtualQueryEx(hProcess, (LPCVOID)dwAddr, &stMBI, sizeof(stMBI)))
			{
				// Try and load it.
				DWORD dwNameLen = 0 ;
				TCHAR szFile[ MAX_PATH ] ;
				memset(szFile, 0, sizeof(szFile));
				HANDLE hFile = NULL ;

				dwNameLen = GetModuleFileName((HINSTANCE)stMBI.AllocationBase, szFile, MAX_PATH);
				if ( 0 != dwNameLen )
				{
					hFile = CreateFile(szFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0) ;
					SymLoadModule(hProcess, hFile,(dwNameLen ? szFile : NULL),NULL,(DWORD)stMBI.AllocationBase,0);
					CloseHandle(hFile);
				}
				return ( (DWORD)stMBI.AllocationBase ) ;
			}
		}
		return ( 0 ) ;
	}

#define STACK_LEN 4000
	static void __stdcall ConvertAddress (HANDLE hProcess, CString& strAppend, DWORD dwAddr, DWORD dwParm1, DWORD dwParm2,DWORD dwParm3,DWORD dwParm4)
	{
		char szTemp [ STACK_LEN + sizeof ( IMAGEHLP_SYMBOL ) ] ;

		PIMAGEHLP_SYMBOL pIHS = (PIMAGEHLP_SYMBOL)&szTemp ;

		IMAGEHLP_MODULE stIHM ;

		ZeroMemory ( pIHS , STACK_LEN + sizeof ( IMAGEHLP_SYMBOL ) ) ;
		ZeroMemory ( &stIHM , sizeof ( IMAGEHLP_MODULE ) ) ;

		pIHS->SizeOfStruct = sizeof ( IMAGEHLP_SYMBOL ) ;
		pIHS->Address = dwAddr ;
		pIHS->MaxNameLength = STACK_LEN ;

		stIHM.SizeOfStruct = sizeof ( IMAGEHLP_MODULE ) ;

		// Always stick the address in first.
		CString strTemp;

		// Get the module name.
		if (SymGetModuleInfo (hProcess, dwAddr , &stIHM ) )
		{
			// Strip off the path.
			LPTSTR szName = _tcsrchr ( stIHM.ImageName , _T ( '\\' ) ) ;
			if ( NULL != szName )
			{
				szName++ ;
			}
			else
			{
				szName = stIHM.ImageName ;
			}
			strTemp.Format(_T("0x%08X[%X] %s: ") , stIHM.BaseOfImage, dwAddr - stIHM.BaseOfImage, szName);
			strAppend += strTemp;
		}
		else
		{
			strTemp.Format(_T ( "0x%08X " ) , dwAddr);
			strAppend += strTemp;

			strAppend += _T ( "<unknown module>: " );
		}

		// Get the function.
		DWORD dwDisp ;
		if (SymGetSymFromAddr (hProcess, dwAddr , &dwDisp , pIHS ) )
		{
			strAppend += pIHS->Name;
			strTemp.Format("(%d,%d,%d,%d)", dwParm1, dwParm2, dwParm3, dwParm4);
			strAppend += strTemp;

			//#ifdef _DEBUG
#if (_WIN32_WINNT >= 0x0500)
			{
				// If I got a symbol, give the source and line a whirl.
				IMAGEHLP_LINE stIHL ;
				ZeroMemory ( &stIHL , sizeof ( IMAGEHLP_LINE)) ;
				stIHL.SizeOfStruct = sizeof ( IMAGEHLP_LINE ) ;

				if (SymGetLineFromAddr(hProcess, dwAddr,&dwDisp,&stIHL))
				{
					// Put this on the next line and indented a bit.
					strTemp.Format(_T("\t\t%s, Line %d"), stIHL.FileName, stIHL.LineNumber);
					strAppend += strTemp;
				}
			}
#endif //_WIN32_WINNT >= 0x0500
			//#endif //_DEBUG
		}
		else
		{
			strTemp.Format("(%d,%d,%d,%d)", dwParm1, dwParm2, dwParm3, dwParm4);
			strAppend += strTemp;
		}

		strAppend += _T("\r\n");
	}

	extern inline void __stdcall GetCallStack(CString& strStack, DWORD dwLevel/*=10*/)
	{
		if (dwLevel < 2)
			dwLevel = 2;

		CONTEXT ctx;
		memset(&ctx, 0, sizeof(ctx));
		ctx.ContextFlags = CONTEXT_FULL ;
		if (GetThreadContext(GetCurrentThread(), &ctx))
			GetCallStack(ctx, strStack, dwLevel);
	}
	extern inline void __stdcall GetCallStack(CONTEXT& stCtx,CString& strStack, DWORD dwLevel/*=10*/)
	{
		if (dwLevel < 2)
			dwLevel = 2;

		HANDLE hProcess = GetCurrentProcess();

		// If the symbol engine is not initialized, do it now.
		if ( FALSE == g_bSymIsInit )
		{
			SymSetOptions(SYMOPT_UNDNAME |SYMOPT_LOAD_LINES) ;

			if (SymInitialize(hProcess, NULL, FALSE))
				g_bSymIsInit = TRUE ;
		}

		//	CDWordArray vAddrs ;
		vector<DWORD> vAddrs;
		STACKFRAME stFrame ;
		DWORD      dwMachine ;

		ZeroMemory ( &stFrame , sizeof ( STACKFRAME ) ) ;
		dwMachine                = IMAGE_FILE_MACHINE_I386 ;
		stFrame.AddrPC.Offset    = stCtx.Eip    ;
		stFrame.AddrPC.Mode = AddrModeFlat ;
		stFrame.AddrFrame.Offset = stCtx.Ebp    ;
		stFrame.AddrFrame.Mode   = AddrModeFlat ;

		for ( DWORD i = 0 ; i <= dwLevel ; i++ )
		{
			if (StackWalk(dwMachine,hProcess,hProcess,&stFrame,&stCtx,NULL,SymFunctionTableAccess,GetModBase, NULL))
			{
				// Also check that the address is not zero.  Sometimes StackWalk returns TRUE with a frame of zero.
				if (0 != stFrame.AddrPC.Offset)
				{
					vAddrs.push_back(stFrame.AddrPC.Offset);
					vAddrs.push_back(stFrame.Params[0]);
					vAddrs.push_back(stFrame.Params[1]);
					vAddrs.push_back(stFrame.Params[2]);
					vAddrs.push_back(stFrame.Params[3]);
				}
			}
		}

		UINT nSize = (UINT)vAddrs.size();
		assert(nSize % 5 == 0);
		for (UINT n = 0; n < nSize; n += 5)
		{
			assert(n + 4 < nSize);
			CString strTemp;
			ConvertAddress(hProcess, strTemp, vAddrs[n], vAddrs[n+1],vAddrs[n+2],vAddrs[n+3], vAddrs[n+4]);
			strStack += strTemp;
		}
	}


	LONG WINAPI MyExceptionFilter(_EXCEPTION_POINTERS *ExceptionInfo )
	{
		//	AFX_MANAGE_STATE(AfxGetStaticModuleState());

		CString strTemp;

		EXCEPTION_RECORD * pERec = ExceptionInfo->ExceptionRecord;
		switch(pERec->ExceptionCode)
		{
		case EXCEPTION_ACCESS_VIOLATION:
			strTemp += "EXCEPTION_ACCESS_VIOLATION";
			break;
		case EXCEPTION_DATATYPE_MISALIGNMENT:
			strTemp += "EXCEPTION_DATATYPE_MISALIGNMENT";
			break;
		case EXCEPTION_BREAKPOINT:
			strTemp += "EXCEPTION_BREAKPOINT";
			break;
		case EXCEPTION_SINGLE_STEP:
			strTemp += "EXCEPTION_SINGLE_STEP";
			break;
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
			strTemp += "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
			break;
		case EXCEPTION_FLT_DENORMAL_OPERAND:
			strTemp += "EXCEPTION_FLT_DENORMAL_OPERAND";
			break;
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:
			strTemp += "EXCEPTION_FLT_DIVIDE_BY_ZERO";
			break;
		case EXCEPTION_FLT_INEXACT_RESULT:
			strTemp += "EXCEPTION_FLT_INEXACT_RESULT";
			break;
		case EXCEPTION_FLT_INVALID_OPERATION:
			strTemp += "EXCEPTION_FLT_INVALID_OPERATION";
			break;
		case EXCEPTION_FLT_OVERFLOW:
			strTemp += "EXCEPTION_FLT_OVERFLOW";
			break;
		case EXCEPTION_FLT_STACK_CHECK:
			strTemp += "EXCEPTION_FLT_STACK_CHECK";
			break;
		case EXCEPTION_FLT_UNDERFLOW:
			strTemp += "EXCEPTION_FLT_UNDERFLOW";
			break;
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
			strTemp += "EXCEPTION_INT_DIVIDE_BY_ZERO";
			break;
		case EXCEPTION_INT_OVERFLOW:
			strTemp += "EXCEPTION_INT_OVERFLOW";
			break;
		case EXCEPTION_PRIV_INSTRUCTION:
			strTemp += "EXCEPTION_PRIV_INSTRUCTION";
			break;
		case EXCEPTION_IN_PAGE_ERROR:
			strTemp += "EXCEPTION_IN_PAGE_ERROR";
			break;
		case EXCEPTION_ILLEGAL_INSTRUCTION:
			strTemp += "EXCEPTION_ILLEGAL_INSTRUCTION";
			break;
		case EXCEPTION_NONCONTINUABLE_EXCEPTION:
			strTemp += "EXCEPTION_NONCONTINUABLE_EXCEPTION";
			break;
		case EXCEPTION_STACK_OVERFLOW:
			strTemp += "EXCEPTION_STACK_OVERFLOW";
			break;
		case EXCEPTION_INVALID_DISPOSITION:
			strTemp += "EXCEPTION_INVALID_DISPOSITION";
			break;
		case EXCEPTION_GUARD_PAGE:
			strTemp += "EXCEPTION_GUARD_PAGE";
			break;
		case EXCEPTION_INVALID_HANDLE:
			strTemp += "EXCEPTION_INVALID_HANDLE";
			break;
		}

		//Add By Hyson, 2005.01.27 19:13
		//For print version info
		//#ifdef _DEBUG
		//	CString strPath = ::GetRootPath() + "CoreD.dll";
		//#else
		//	CString strPath = ::GetRootPath() + "Core.dll";
		//#endif

		//End Add

		//出错类型:\r\n%s\r\n出错地址:\r\n 0x%08X\r\n
		CString m_strError;

		m_strError.Format("\r\n\n************************ START ************************\r\n出错类型： %s\r\n出错地址： 0x%08X \r\n",strTemp, pERec->ExceptionAddress);
		m_strError = m_strError;

		if(pERec->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
			pERec->NumberParameters >1 &&
			pERec->ExceptionInformation )
		{
			CString str;
			if(pERec->ExceptionInformation[0]==0)
				str = "读取";
			else
				str = "写入";
			//出错原因:\r\n对 0x%08X 地址进行 %s 操作
			strTemp.Format("出错原因： 对 0x%08X 地址进行%s操作",pERec->ExceptionInformation[1],str);
			m_strError += strTemp;
		}	
		strTemp.Format("\r\n出错时间： %s", CTime::GetCurrentTime().Format("%Y-%m-%d %H:%M:%S"));
		m_strError += strTemp;

		m_strError += "\r\n\r\n出错过程：\r\n";
		strTemp.Empty();
		GetCallStack(*(ExceptionInfo->ContextRecord), strTemp);
		m_strError += strTemp;

		//\r\n模块及其加载地址列表: \r\n===================================
		m_strError += "\r\n\r\n模块及其加载地址列表: \r\n-----------------------------------";

		PBYTE pb = NULL;
		MEMORY_BASIC_INFORMATION mbi;
		int nLen;
		char szModName[MAX_PATH];

		while (VirtualQuery(pb, &mbi, sizeof(mbi)) == sizeof(mbi))
		{
			if (mbi.State == MEM_FREE)
				mbi.AllocationBase = mbi.BaseAddress;

			if ( (mbi.AllocationBase != mbi.BaseAddress) ||
				(mbi.AllocationBase == NULL))
			{
				// Do not add the module name to the list
				// if any of the following is true:
				// 1. If this block is NOT the beginning of a region
				// 2. If the address is NULL
				nLen = 0;
			}
			else
			{
				nLen = GetModuleFileNameA((HINSTANCE) mbi.AllocationBase,
					szModName, MAX_PATH);
			}
			if (nLen > 0)
			{
				strTemp.Format("\r\n[ 0x%08X ] %s", mbi.AllocationBase, szModName);
				m_strError += strTemp;
			}
			pb += mbi.RegionSize;
		}
		m_strError += "\r\n___________________________________";


		//MessageBox(NULL, m_strError, "出错耶!!", MB_OK);

		//CExceptionDlg Dlg;
		//Dlg.m_ExceptionInfo = ExceptionInfo;
		//Dlg.DoModal();

		FILE * fp = fopen("_UnHandled_Exception.log", "a+"); 
		if(fp) 
		{ 
			fputs(m_strError.GetBuffer(0), fp); 
			fputs("\r\n", fp); 
			fclose(fp); 
		} 

		return EXCEPTION_EXECUTE_HANDLER;
	}

	ExceptCatch::ExceptCatch()
	{
		::SetUnhandledExceptionFilter(MyExceptionFilter);	
	}
}