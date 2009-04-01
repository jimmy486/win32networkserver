// UdpServer.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "UdpServer.h"
#include <vector>
#include <assert.h>
#include <WinSock2.h>
#include <process.h>
#include <string>
#include "HeadFile.h"
#include "UdpSer.h"
#include "RunLog.h"
#include "unhandledexception.h"
#include "HelpMng.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

CWinApp theApp;

using namespace std;
using namespace HelpMng;

//__my_set_exception_handler g_Exception;		//捕获程序运行过程中异常信息
UdpSer* g_UdpSer = NULL;					//tcp网络服务模块
BOOL g_bRun = FALSE;

UINT WINAPI ThreadProc(LPVOID pParam);

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	// 初始化 MFC 并在失败时显示错误
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: 更改错误代码以符合您的需要
		_tprintf(_T("致命错误: MFC 初始化失败\n"));
		nRetCode = 1;
	}
	else
	{
		// TODO: 在此处为应用程序的行为编写代码。
		//日志操作类
		RunLog::_GetObject()->SetSaveFiles(5);			//只保留最近5天的日志信息
		(RunLog::_GetObject())->SetPrintScreen(FALSE);

		string strDecode;
		char cTemp[5];int asc[4];
		strDecode += (char)(int)(asc[0] << 2 &#124; asc[1] << 2 >> 6)
		//strDecode+=(char)(int)(asc[0] << 2 &#124; asc[1] << 2 >> 6);

#ifdef _DEBUG
		cout << "===================START TEST(SERVER_D)===================" << endl;
		_TRACE("===================START_D TEST===================");
#else
		cout << "===================START TEST(SERVER)===================" << endl;
		_TRACE("===================START TEST===================");
#endif

		UdpSer::InitReource();
		g_UdpSer = new UdpSer();
		g_bRun = TRUE;

		int nTcpPort;
		char szIp[50] = { 0 };
		CString szIniFile(RunLog::GetModulePath());
		szIniFile += "ser_info.ini";

		nTcpPort = GetPrivateProfileInt("ser_info", "port", 0, szIniFile);
		GetPrivateProfileString("ser_info", "ip", "127.0.0.1", szIp, sizeof(szIp) -1, szIniFile);

		try
		{
			//vector<string> ip_list;
			//GetLocalIps(&ip_list);
			printf("\r\n将在 %s:%ld 上启动服务器\r\n", szIp, nTcpPort);

			//启动TCP和UDP服务
			if (FALSE == g_UdpSer->StartServer(szIp, nTcpPort))
			{
				cout << "TCP服务启动失败" << endl;
				throw ((long)__LINE__);
			}

			//启动后台线程处理数据
			_beginthreadex(NULL, 0, ThreadProc, NULL, 0, NULL);

			cout << "输入q退出程序" << endl;
			getchar();
			getchar();

			g_bRun = FALSE;

			_TRACE("===================END TEST===================");
		}
		catch (long iErrLine)
		{
			cout << iErrLine << "行程序异常，需退出程序" <<endl;
		}

		g_UdpSer->CloseServer();
		delete g_UdpSer;

		//开发者要在程序结束时释放资源
		RunLog::_Destroy();
		UdpSer::ReleaseReource();
	}

	return nRetCode;
}

UINT WINAPI ThreadProc(LPVOID pParam)
{
	DWORD nQueLen = 0;

	//TRACE("\r\n%s : %ld g_TcpMng = 0x%x", __FILE__, __LINE__, g_TcpMng);

	while (g_bRun)
	{
		UDP_RCV_DATA* pRcvData = g_UdpSer->GetRcvData(&nQueLen);
		//TRACE("\r\n%s : %ld pRcvData = 0x%x", __FILE__, __LINE__, pRcvData);

		if (pRcvData)
		{
			cout << "收到来自客户端的UDP数据 : " << endl;
			cout << "LEN = " << pRcvData->m_nLen << " QUE_LEN = " << nQueLen << " DATA = " << pRcvData->m_pData << endl;

			//将数据原样返回给客户端
			g_UdpSer->SendData(pRcvData->m_PeerAddr, pRcvData->m_pData, pRcvData->m_nLen);

			//使用结束后需进行内存释放
			delete pRcvData;
			pRcvData = NULL;
		}
		else
		{
			Sleep(10);
		}
	}
	return 0;
}
