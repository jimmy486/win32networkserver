// MutilClients.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "MutilClients.h"
#include <string>
#include "ClientNet.h"
#include "MyClient.h"
#include "RunLog.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// The one and only application object

CWinApp theApp;

using namespace std;

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
		//初始化相关资源
		HelpMng::ClientNet::InitReource();
#ifdef _DEBUG
		cout << "===================START TEST(MUTIL CLIENT_D)===================" << endl;
#else
		cout << "===================START TEST(MUTIL CLIENT)===================" << endl;
#endif
		MyClient* pMutilClient = NULL;
		int nClientNum = 0, nPort, nStartNo;
		BOOL bTcpType = TRUE;
		char szSerIp[50];

		try
		{
			CString szIniFile(HelpMng::RunLog::GetModulePath());
			szIniFile += "client_info.ini";

			nClientNum = GetPrivateProfileInt("tcp_info", "client_num", 1000, szIniFile);
			if (nClientNum < 1 || nClientNum > 2000)
			{
				cout << "客户端个数非法" << endl;
				throw ((long)__LINE__);
			}

			pMutilClient = new MyClient[nClientNum];
			if (NULL == pMutilClient)
			{
				throw ((long)__LINE__);
			}

			nStartNo = GetPrivateProfileInt("tcp_info", "base_num", 1, szIniFile);
			bTcpType = GetPrivateProfileInt("tcp_info", "type", 1, szIniFile);
			GetPrivateProfileString("tcp_info", "ser_ip", "127.0.0.1", szSerIp, sizeof(szSerIp) -1, szIniFile);
			nPort = GetPrivateProfileInt("tcp_info", "ser_port", 5222, szIniFile);

			//初始化客户端
			for (int index = 0; index < nClientNum; index++)
			{
				if (FALSE == pMutilClient[index].Init(index +nStartNo, szSerIp, nPort, bTcpType))
				{
					printf("初始化 [%ld] 客户端失败\r\n", index+1);
				}
			}
            
			getchar();
			getchar();
		}
		catch (const long& lErrLine)
		{
			cout << lErrLine << " 行程序异常，需退出程序" <<endl;
		}

		if (pMutilClient)
		{
			delete []pMutilClient;
		}
		//程序结束释放相关资源
		HelpMng::ClientNet::ReleaseReource();
	}

	return nRetCode;
}
