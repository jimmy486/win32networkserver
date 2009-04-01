#include <process.h>
#include <assert.h>

#include "NetWork.h"
#include "RunLog.h"

#pragma warning (disable:4312)

namespace HelpMng
{
	//初始化静态成员变量
	BOOL NetWork::s_bInit = FALSE;
	HANDLE NetWork::s_hCompletion = NULL;
	HANDLE *NetWork::s_pThreads = NULL;

	void NetWork::InitReource()
	{
		if (FALSE == s_bInit)
		{
			s_bInit = TRUE;			

			//创建完成端口
			s_hCompletion = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

			//创建完成端口上工作的后台线程
			SYSTEM_INFO sys_info;
			GetSystemInfo(&sys_info);

			const DWORD MAX_THREAD = sys_info.dwNumberOfProcessors * 2 +2;
			s_pThreads = new HANDLE[MAX_THREAD];
			assert(s_pThreads);

			for (DWORD nIndex = 0; nIndex < MAX_THREAD; nIndex++)
			{
				s_pThreads[nIndex] = (HANDLE)_beginthreadex(NULL, 0, WorkThread, NULL, 0, NULL);
			}
		}
	}

	void NetWork::ReleaseReource()
	{
		if (s_bInit)
		{
			s_bInit = FALSE;

			SYSTEM_INFO sys_info;
			GetSystemInfo(&sys_info);
			const DWORD MAX_THREAD = sys_info.dwNumberOfProcessors * 2 +2;

			for ( DWORD nIndex = 0; nIndex < MAX_THREAD + 10; nIndex++ )
			{
				//停止后台线程的运行
				PostQueuedCompletionStatus(s_hCompletion, 0xffffffff, NULL, NULL);
			}

			WaitForMultipleObjects(MAX_THREAD, s_pThreads, TRUE, 30 * 1000);
			//_TRACE("ReleaseReource() TIME = %u", GetTickCount());

			for ( DWORD nIndex = 0; nIndex < MAX_THREAD; nIndex++ )
			{
				CloseHandle(s_pThreads[nIndex]);
			}

			delete []s_pThreads;

			//关闭完成端口
			CloseHandle(s_hCompletion);
		}
	}

	BOOL NetWork::SendData(const IP_ADDR& PeerAddr, const char * szData, int nLen)
	{
		return TRUE;
	}

	BOOL NetWork::SendData(const char * szData, int nDataLen, SOCKET hSock /* = INVALID_SOCKET */)
	{
		return TRUE;
	}

	void *NetWork::GetRecvData(DWORD* const pQueLen)
	{
		return NULL;
	}

	BOOL NetWork::Connect(const char *szIp, int nPort, const char *szData, int nLen)
	{
		return TRUE;
	}

	UINT WINAPI NetWork::WorkThread(LPVOID lpParam)
	{
		DWORD dwTrans = 0, dwKey = 0;
		LPOVERLAPPED pOl = NULL;
		BOOL bOk = FALSE;

		while (TRUE)
		{
			bOk = GetQueuedCompletionStatus(s_hCompletion, &dwTrans, &dwKey, (LPOVERLAPPED *)&pOl, WSA_INFINITE);
			if (0xffffffff == dwTrans)
			{
				break;
			}
			assert(dwKey);

			NetWork *pNetMng = (NetWork *)dwKey;

			if (pNetMng)
			{				
				pNetMng->IOCompletionProc(bOk, dwTrans, pOl);
			}
		}
		//_TRACE("WorkThread() TIME = %u", GetTickCount());

		return 0;
	}

	void ReleaseNetWork()
	{
		NetWork::ReleaseReource();
	}
}