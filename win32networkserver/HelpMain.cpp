#include "HeadFile.h"
#include <atltrace.h>
#include <atlstr.h>
#include "NetWork.h"
#include "TcpClient.h"
#include "TcpSerEx.h"
#include "UdpSerEx.h"
#include "unhandledexception.h"
#include "RunLog.h"
#include "HelpMng.h"

using namespace HelpMng;

ExceptCatch g_ExpCatch;

BOOL APIENTRY DllMain( HANDLE hModule, 
					  DWORD  ul_reason_for_call, 
					  LPVOID lpReserved
					  )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		WSADATA wsData;
		WSAStartup(MAKEWORD(2, 2), &wsData);

		NET_CONTEXT::InitReource();
		NetWork::InitReource();
		TcpSerEx::InitReource();
		UdpSerEx::InitReource();
		TcpClient::InitReource();
		ATLTRACE("\r\n%s -- %ld\n", __FILE__, __LINE__);
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		WSACleanup();
		ATLTRACE("\r\n%s -- %ld\n", __FILE__, __LINE__);
		break;
	}
	return TRUE;
}

namespace HelpMng
{
	void ReleaseAll()
	{
		static BOOL bRelease = FALSE;

		if (FALSE == bRelease)
		{
			bRelease = TRUE;
			ReleaseTcpSerEx();
			ReleaseUdpSerEx();
			ReleaseTcpClient();
			ReleaseNetWork();	
			NET_CONTEXT::ReleaseReource();
			RunLog::_Destroy();
		}
	}
}