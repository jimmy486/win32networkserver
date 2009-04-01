// checknat_imclient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "checknat_imclient.h"
#include <process.h>

#include <string>
#include "ClientNet.h"
#include "natcheck.h"

// The one and only application object

CWinApp theApp;

using namespace std;
using namespace HelpMng;			//需要引用命名空间
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ClientNet* g_pTcpNet = NULL;       //客户端网络TCP传输模块
//This is Listen
ClientNet* g_pTcpListen = NULL;
//This is Listen
ClientNet* g_pTcpSym = NULL;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool g_bNCClose = false;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
char g_szClientNatIP[16]; 
char g_szClientNatPort[6];
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int g_NatType = CHECK_NAT_FAILE;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//TCP监听回调
void CALLBACK AcceptProc(LPVOID pParam, const SOCKET& sockClient,const IP_ADDR& AddrClient);

//错误处理外部主动连接回调从服务器函数
void CALLBACK AcceptErrorProc(LPVOID pParam, int nOpt);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//TCP模式的数据接收主服务器函数
int CALLBACK ReadProc(LPVOID pParam, const char* szData, int nLen);

//连接主服务器成功后的回调函数
void CALLBACK ConnectProc(LPVOID pParam);

//错误处理主服务器回调函数
void CALLBACK ErrorProc(LPVOID pParam, int nOpt);
////////////////////////////////////////////////////////////////////////////////////

//TCP模式的数据接收从服务器函数
int CALLBACK NCSymReadProc(LPVOID pParam, const char* szData, int nLen);
//连接成功后的回调从服务器函数
void CALLBACK NCSymConnectProc(LPVOID pParam);
//错误处理从服务器回调函数
void CALLBACK NCSymErrorProc(LPVOID pParam, int nOpt);
//////////////////////////////////////////////////////////////////////////////////////////////


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
		ClientNet::InitReource();

		g_pTcpNet = new ClientNet(TRUE);
		assert(g_pTcpNet);
		
		g_pTcpListen = new ClientNet(TRUE);
		assert(g_pTcpListen);
		
		g_pTcpSym = new ClientNet(TRUE);
		assert(g_pTcpSym);

		//与主服务器通信
		g_pTcpNet->SetTCPCallback(NULL, NULL, ConnectProc, ReadProc, ErrorProc);
		
		//与从服务器通信
		g_pTcpSym->SetTCPCallback(NULL, NULL, NCSymConnectProc, NCSymReadProc, NCSymErrorProc);
		
		//接受外部主动通信
		g_pTcpListen->SetTCPCallback(NULL, AcceptProc ,NULL, NULL, AcceptErrorProc);
	
		//连接主服务器
		if (FALSE == g_pTcpNet->Connect(NC_MAINSER_IP, NC_MAINSER_IP_PORT, true, "192.168.100.69", 10000))
		{
			cout << "连接到服务器 " << NC_MAINSER_IP << ":" << NC_MAINSER_IP_PORT << "失败" << endl;
		
		}		

		// TODO: code your application's behavior here.
	}
	cout << "输入q退出程序" << endl;
	while (!g_bNCClose)
	{

	}
	cout << "this is end" << endl;
/*	while (getchar() != 'q')
	{
	}*/


	//结束与主服务器通信
	if (g_pTcpNet != NULL)
	{
		delete g_pTcpNet;
		g_pTcpNet = NULL;
	}
	if (g_pTcpSym != NULL)
	{
		delete g_pTcpSym;
		g_pTcpSym = NULL;

	}
	//删除对从服务器的监听
	if (g_pTcpListen != NULL)
	{
		delete g_pTcpListen;
		g_pTcpListen = NULL;

	}

	ClientNet::ReleaseReource();
	return nRetCode;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//TCP模式的数据接收主服务器函数
int CALLBACK ReadProc(LPVOID pParam, const char* szData, int nLen)
{
	static int nCount = 1;
	char szBuf[BUF_SIZE] = { 0 };
	int iResult;

	string szIp;
	int nPort;

 
	//获得与主服务器通信的地址
	g_pTcpNet->GetLocalAddr(szIp, nPort);

	cout << "受到来自主服务器的TCP数据包 DATA = " << szData << endl;
	sprintf(szBuf, "Client TCP Data %ld", nCount);

	//删除对从服务器的监听
	if (g_pTcpListen != NULL)
	{
		delete g_pTcpListen;
		g_pTcpListen = NULL;

	}

	//分析数据
	iResult = NCHandleRecvMsg(szData, g_szClientNatIP, g_szClientNatPort);
	if (iResult == REQUEST_CHECKNAT_SYMMETRIC) {
		cout << "开始检测对称型,连接从服务器" << endl;
		cout << "Bind IP: " << szIp.c_str() << "port" << nPort << endl;
		if (FALSE == g_pTcpSym->Connect(NC_ASSISTANSER_IP, NC_ASSISTANTSER_PORT, TRUE, szIp.c_str(), nPort))
		{
			cout << "连接到从服务器 " << NC_ASSISTANSER_IP << ":" << NC_ASSISTANTSER_PORT << "失败" << endl;
			if (g_pTcpSym != NULL)
			{
				delete g_pTcpSym;
				g_pTcpSym = NULL;

			}
		}
		cout << "连接到从服务器 " << NC_ASSISTANSER_IP << ":" << NC_ASSISTANTSER_PORT << "成功" << endl;
		
		
	}
	else if (iResult == INEXIST_NAT_NOT_PASSIVITY__VISIT) {
		cout << "This is result: INEXIST_NAT_NOT_PASSIVITY__VISIT" << endl;
		g_bNCClose = false;
	}
	else {
		cout << "This is result : ERROR" << endl;
		g_bNCClose = false;
	}
	
	nCount ++;
	return 0;
}

//连接主服务器成功后的回调函数
void CALLBACK ConnectProc(LPVOID pParam)
{
	cout << "成功连接到服务器" << endl;
	//发送NATCHECK第一份TCP数据包 
	char sendbuf[BUF_SIZE];
	char *pszPort = NULL;
	string szIp;
	int nPort;
    int iMsgLen = 0;

	//获得主服务器的源地址
	g_pTcpNet->GetLocalAddr(szIp, nPort);
	CovertUIntToString(nPort, &pszPort);

	//发送请求外部主动通信
	if (NCHandleSendMsg(sendbuf, REQUSET_NC_ASSISTANTSEV_ACTIVELINK, szIp.c_str(), pszPort, iMsgLen)) {
		
		g_pTcpNet->Send(sendbuf, iMsgLen);
		
		cout << "Client Send msg:= "<< sendbuf << " to NCMainSev is ok" << endl;
	
	}
    DelString(&pszPort);

	//监听外部通讯
	if (g_pTcpListen->Listen(10000, true, szIp.c_str())) {
		cout << "start Listen is ok" << endl;
	}
	else {
		cout << "start Listen is failed" << endl;
		
		delete g_pTcpListen;
		g_pTcpListen = NULL;
	}
}

//错误处理主服务器回调函数
void CALLBACK ErrorProc(LPVOID pParam, int nOpt)
{
	cout << "检测到网络错误; 错误码 = " << nOpt << endl;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//TCP监听外部主动连接回调
void CALLBACK AcceptProc(LPVOID pParam, const SOCKET& sockClient,const IP_ADDR& AddrClient)
{
	
	char *pszIP = NULL;
	unsigned int uiPort = 0;
    
	//获得对方地址
	if (P2PGetLocalIPAndPort(AddrClient, &pszIP, uiPort)) {
		cout << "Accept " << pszIP << "Port " << uiPort << endl;
		if (strcmp(pszIP, NC_ASSISTANSER_IP) == 0) 
		{
			cout << "从服务器主动连接成功" << endl;
			
			g_NatType = PASSIVITY_VISIT;
			cout << "the client Nat is PASSIVITY_VISIT" << endl;
			g_bNCClose = true;
		}
		else 
		{
			cout << "非法外部主动连接" << endl;
			closesocket(sockClient);

		}
	}
    DelString(&pszIP);
	

}
void CALLBACK AcceptErrorProc(LPVOID pParam, int nOpt)
{
	cout << "监听socket出现错误"  << nOpt << endl;

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//从服务器对称检测
int CALLBACK NCSymReadProc(LPVOID pParam, const char* szData, int nLen)
{
	static int nCount = 1;
	char szBuf[BUF_SIZE] = { 0 };
	int iResult = CHECK_NAT_FAILE;

	cout << "受到来自从服务器的TCP数据包 DATA = " << szData << endl;
	sprintf(szBuf, "Client TCP Data %ld", nCount);

	iResult = NCHandleRecvMsg(szData, NULL, NULL);
	if (iResult == CONES_NAT)
	{
		cout << "This is result CONES_NAT" << endl;

	}
	else if (iResult == SYMMETRIC_NAT) 
	{
		cout << "This is result SYMMETRIC_NAT" << endl;
	}
	else
	{
		cout << "NCSym is error" << endl;
	}

	//删除与从服务器的通信
	if (g_pTcpSym != NULL)
	{
		delete g_pTcpSym;
		g_pTcpSym = NULL;

	}
	g_bNCClose = false;
	
	g_NatType = iResult;

	nCount ++;
	return 0;
}

//连接从服务器对称检测成功后的回调函数
void CALLBACK NCSymConnectProc(LPVOID pParam)
{
	cout << "成功连接到从服务器" << endl;
	//发送NATCHECK第一份TCP数据包 
	char sendbuf[BUF_SIZE];
	int iMsgLen = 0;


	//开始检测sym
	if (NCHandleSendMsg(sendbuf, REQUEST_CHECKNAT_SYMMETRIC, g_szClientNatIP, g_szClientNatPort, iMsgLen)) {
		
		g_pTcpSym->Send(sendbuf, iMsgLen+1);
		cout << iMsgLen << endl;
		cout << strlen(sendbuf) << endl;
		cout << "The client Send Msg = " << sendbuf << endl;
	}

	
}
void CALLBACK NCSymErrorProc(LPVOID pParam, int nOpt)
{
	cout << "NCSymsocket出现错误" << nOpt << endl;
}