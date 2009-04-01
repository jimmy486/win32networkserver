// ClientSer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "ClientSer.h"
#include <string>
#include "MyClient.h"
#include "HeadFile.h"
//#ifdef _DEBUG
//#define new DEBUG_NEW
//#endif


// The one and only application object

CWinApp theApp;

using namespace std;
using namespace HelpMng;

ClientNet* g_pListenNet = NULL;
ClientNet* g_pConnNet = NULL;
ClientNet* g_pConnNet2 = NULL;

volatile long g_lCount = 0;

void CALLBACK AcceptProc(LPVOID pParam, const SOCKET& sockClient, const IP_ADDR& AddrClient);
void CALLBACK ErrorProc(LPVOID pParam, int nOpt);

int CALLBACK ReadProc(LPVOID pParam, const char* szData, int nLen);
void CALLBACK ConnectProc(LPVOID pParam);
void CALLBACK ConnErrProc(LPVOID pParam, int nOpt);

void CALLBACK ConnProc1(LPVOID pParam);
void CALLBACK ConnErrProc1(LPVOID pParam, int nOpt);

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		_tprintf(_T("Fatal Error: MFC initialization failed\n"));
		nRetCode = 1;
	}
	else
	{
		// TODO: code your application's behavior here.
#ifdef _DEBUG
		cout << "===================START TEST(CLIENT SER_D)===================" << endl;
#else
		cout << "===================START TEST(CLIENT SER)===================" << endl;
#endif

		ClientNet::InitReource();
		int nPort = 0;
		cout << "请输入监听端口号 : ";
		cin >> nPort;

        g_pListenNet = new ClientNet(TRUE);
		g_pConnNet = new ClientNet(TRUE);
		g_pConnNet2 = new ClientNet(TRUE);
		ASSERT(g_pListenNet);
		ASSERT(g_pConnNet);
		ASSERT(g_pConnNet2);

		g_pListenNet->SetTCPCallback(NULL, AcceptProc, NULL, NULL, ErrorProc);
		g_pConnNet->SetTCPCallback(NULL, NULL, ConnectProc, ReadProc, ConnErrProc);
		g_pConnNet2->SetTCPCallback(NULL, NULL, ConnProc1, NULL, ConnErrProc1);

		if (FALSE == g_pConnNet2->Connect("192.168.100.71", 8901, TRUE, "192.168.100.71", 6908))
		{
			cout << "启动监听失败1" << endl;
		}

		string szLocalIp;

		g_pListenNet->GetLocalAddr(szLocalIp, nPort);

		if (FALSE == g_pListenNet->Listen(6908, TRUE, "192.168.100.71"))
		{
			cout << "启动监听失败1" << endl;
		}

		if (FALSE == g_pConnNet->Connect("192.168.100.71", 8900, TRUE, "192.168.100.71", 6908))
		{
			cout << "启动连接失败" << endl;
		}

		cout << "按q键退出" <<endl;
		while (getchar() != 'q')
		{
		}
		delete g_pListenNet;
		delete g_pConnNet;
		delete g_pConnNet2;
		ClientNet::ReleaseReource();
	}

	return nRetCode;
}

void CALLBACK AcceptProc(LPVOID pParam, const SOCKET& sockClient, const IP_ADDR& AddrClient)
{
	string szIp;
	int nPort;

	GetAddress(AddrClient, szIp, nPort);
	cout << "客户端 " << szIp << ':' << nPort << "连接到本服务器" << endl;
	InterlockedExchangeAdd(&g_lCount, 1);
	MyClient* pClient = new MyClient(sockClient, g_lCount);
	ASSERT(pClient);
}
void CALLBACK ErrorProc(LPVOID pParam, int nOpt)
{
	cout << "监听socket出现错误" << endl;
}

int CALLBACK ReadProc(LPVOID pParam, const char* szData, int nLen)
{
	return 0;
}

void CALLBACK ConnectProc(LPVOID pParam)
{
	cout << "\r\nsocket2连接到服务器成功" << endl;
}

void CALLBACK ConnErrProc(LPVOID pParam, int nOpt)
{
	cout << "连接socket2出现错误 ERR = " << nOpt << endl;
}

void CALLBACK ConnErrProc1(LPVOID pParam, int nOpt)
{
	cout << "连接socket1出现错误 ERR = " << nOpt << endl;
}

void CALLBACK ConnProc1(LPVOID pParam)
{
	cout << "\r\nsocket1连接到服务器上 " << endl;
}
