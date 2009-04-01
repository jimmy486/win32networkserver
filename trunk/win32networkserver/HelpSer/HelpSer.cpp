// HelpSer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "HelpSer.h"
#include <vector>
#include <assert.h>
#include <WinSock2.h>
#include <process.h>
#include <string>

#include "HeadFile.h"
#include "NetWork.h"
#include "TcpSerEx.h"
#include "UdpSerEx.h"
#include "RunLog.h"
#include "unhandledexception.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// The one and only application object

CWinApp theApp;

using namespace std;
using namespace HelpMng;		//开发人员需引用HelpMng命名空间

TcpSerEx * g_TcpSer = NULL;					//tcp网络服务模块
UdpSerEx *g_UdpSer = NULL;					//udp网络服务模块
BOOL g_bRun = FALSE;

//当某个连接断开的通知函数
void WINAPI OnClose(LPVOID pParam, SOCKET hSock, int nOpt);
void WINAPI OnUdpSerClose(LPVOID pParam, SOCKET hSock, int nOpt);
UINT WINAPI ThreadProc(LPVOID pParam);

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	//SetExceptionHandler();

	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		_tprintf(_T("Fatal Error: MFC initialization failed\n"));
		nRetCode = 1;
	}
	else
	{
		char *pTest = NULL;
		strcpy(pTest, "sswwww");
		// TODO: code your application's behavior here.
		//日志操作类
		RunLog::_GetObject()->SetSaveFiles(5);			//只保留最近5天的日志信息
		(RunLog::_GetObject())->SetPrintScreen(FALSE);
#ifdef _DEBUG
		cout << "===================START TEST(SERVER_D)===================" << endl;
		_TRACE("===================START_D TEST===================");
#else
		cout << "===================START TEST(SERVER)===================" << endl;
		_TRACE("===================START TEST===================");
#endif

		TcpSerEx::InitReource();
		UdpSerEx::InitReource();
		//TRACE("\r\nGetProcessHeap = 0x%x", GetProcessHeap());

		g_TcpSer = new TcpSerEx(OnClose, NULL);
		g_UdpSer = new UdpSerEx(OnUdpSerClose, NULL);
		g_bRun = TRUE;
		//ASSERT(g_UdpMng);
		//ASSERT(g_TcpMng);

		int nTcpPort, nUdpPort;
		char szIp[50] = { 0 };
		char szUdpIp[50] = { 0 };
		CString szIniFile(RunLog::GetModulePath());
		szIniFile += "ser_info.ini";
		nTcpPort = GetPrivateProfileInt("tcp_info", "port", 0, szIniFile);
		nUdpPort = GetPrivateProfileInt("udp_info", "port", 0, szIniFile);

		GetPrivateProfileString("tcp_info", "ip", "127.0.0.1", szIp, sizeof(szIp) -1, szIniFile);
		GetPrivateProfileString("udp_info", "ip", "127.0.0.1", szUdpIp, sizeof(szUdpIp) -1, szIniFile);

		try
		{
			//vector<string> ip_list;
			//GetLocalIps(&ip_list);
			printf("\r\n将在 %s:%ld 上启动TCP服务, 在 %s : %ld 上启动UDP服务\r\n", szIp, nTcpPort, szUdpIp, nUdpPort);

			//启动TCP和UDP服务
			if (FALSE == g_TcpSer->Init(szIp, nTcpPort))
			{
				cout << "TCP服务启动失败" << endl;
				throw ((long)__LINE__);
			}

			if (FALSE == g_UdpSer->Init(szUdpIp, nUdpPort))
			{
				cout << "UDP 服务启动失败" << endl;
				THROW_LINE;
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

		g_TcpSer->Close();
		g_UdpSer->Close();
		delete g_TcpSer;
		delete g_UdpSer;

		//开发者要在程序结束时释放资源		
		TcpSerEx::ReleaseReource();
		UdpSerEx::ReleaseReource();
		RunLog::_Destroy();
	}

	return nRetCode;
}

void WINAPI OnClose(LPVOID pParam, SOCKET sock, int nOpt)
{
	//IP_ADDR peer_addr;
	//string szPeerAddr;
	//int nPort = 0;
	//int nAddrLen = sizeof(peer_addr);
	//getsockname(sock, (sockaddr*)&peer_addr, &nAddrLen);

	//GetAddress(peer_addr, &szPeerAddr, nPort);
	//g_TcpMng->GetSockAddr(sock, szPeerAddr, nPort);
	//GetPeerAddr(peer_addr, szPeerAddr, nPort);
	cout << "\r\n客户端 SOCKET =  " <<  sock << "断开连接" << endl;
	//_TRACE("客户端 %s:%ld 断开连接", szPeerAddr.c_str(), nPort);
}

void WINAPI OnUdpSerClose(LPVOID pParam, SOCKET hSock, int nOpt)
{

}

UINT WINAPI ThreadProc(LPVOID pParam)
{
	DWORD nQueLen = 0;
	DWORD nUdpLen = 0;
	string szClientIp;
	int nClientPort;

	//TRACE("\r\n%s : %ld g_TcpMng = 0x%x", __FILE__, __LINE__, g_TcpMng);

	while (g_bRun)
	{
		//从TCP接收队列中取出收到的数据
		//TRACE("\r\n%s : %ld 0", __FILE__, __LINE__);
		TCP_RCV_DATA_EX* pRcvData = (TCP_RCV_DATA_EX *)(g_TcpSer->GetRecvData(&nQueLen));
		UDP_RCV_DATA_EX *pUdpRcvData = (UDP_RCV_DATA_EX *)(g_UdpSer->GetRecvData(&nUdpLen));
		//TRACE("\r\n%s : %ld pRcvData = 0x%x", __FILE__, __LINE__, pRcvData);

		if (pRcvData)
		{
			cout << "\r\n收到来自客户端的TCP数据 : ";
			cout << "\r\nLEN = " << pRcvData->m_nLen << " QUE_LEN = " << nQueLen << " DATA = " << (pRcvData->m_pData + sizeof(PACKET_HEAD));

			//将数据原样返回给客户端
			g_TcpSer->SendData( pRcvData->m_pData, pRcvData->m_nLen, pRcvData->m_hSocket);
		}
		if (pUdpRcvData)
		{
			GetAddress(pUdpRcvData->m_PeerAddr, &szClientIp, nClientPort);
			printf("\r\n收到来自客户端 %s : %ld 的UDP数据 : ", szClientIp.c_str(), nClientPort);
			printf("\r\nLEN = %ld QUE_LEN = %ld DATA = %s", pUdpRcvData->m_nLen, nUdpLen, pUdpRcvData->m_pData + sizeof(PACKET_HEAD));

			g_UdpSer->SendData(pUdpRcvData->m_PeerAddr, pUdpRcvData->m_pData, pUdpRcvData->m_nLen);
		}

		if (NULL == pUdpRcvData && NULL == pRcvData)
		{
			Sleep(10);
		}

		delete pUdpRcvData;
		delete pRcvData;
	}
	return 0;
}


