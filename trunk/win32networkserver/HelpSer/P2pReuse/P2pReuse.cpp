// P2pReuse.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string>
#include "P2pReuse.h"
#include "ClientNet.h"
//#ifdef _DEBUG
//#define new DEBUG_NEW
//#endif


// The one and only application object

CWinApp theApp;

using namespace std;
using namespace HelpMng;

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
		//GUID guid;
		//CoCreateGuid(&guid);


		//ClientNet::InitReource();
		//ClientNet* pUdp1 = new ClientNet(TRUE);
		//ClientNet* pUdp2 = new ClientNet(TRUE);
		//ASSERT(pUdp1);
		//ASSERT(pUdp2);

		//pUdp1->SetUDPCallback(NULL, ReadFromProc1, ErrProc1);
		//pUdp2->SetUDPCallback(NULL, ReadFromProc2, ErrProc2);

		//int nRet = 0;
		//nRet = pUdp1->Bind(4600, TRUE, "192.168.100.71");
		//nRet = pUdp2->Bind(4600, TRUE, "192.168.100.71");

		//nRet = pUdp1->SendTo(IP_ADDR("192.168.100.71", 5500), "test1", 6);
		//nRet = pUdp2->SendTo(IP_ADDR("192.168.100.71", 5500), "test2", 6);
		//WIN32_FIND_DATA find_data;
		//cout << atoi("f") << endl;
		//CString szFilePath = "D:/Vusual Studio Project/Projects/HelpMng/*.h";
		//HANDLE hFind = FindFirstFile(szFilePath, &find_data);
		//if (INVALID_HANDLE_VALUE != hFind)
		//{
		//	do
		//	{
		//		cout << "cFileName : " << find_data.cFileName << " \tcAlternateFileName : "<< find_data.cAlternateFileName << endl;
		//	}while(FindNextFile(hFind, &find_data));
		//	FindClose(hFind);
		//}

		//DWORD test[10][2];
		//cout << "sizeof(DWORD[][]) = " << sizeof(test)/sizeof(DWORD) << endl;
		WSADATA wsData;
		WSAStartup(MAKEWORD(2, 2), &wsData);

		char szBuf[1024] = "我的测试程序的德尔";
		SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
		ASSERT(INVALID_SOCKET != sock);
		IP_ADDR addr("172.24.17.20", 5222);
		connect(sock, (sockaddr *)&addr, sizeof(IP_ADDR));
		//send(sock, "test", 5, 0);

		//Sleep(5000);

		//send(sock, "test1", 6, 0);
		
		getchar();

		WSACleanup();
	}

	return nRetCode;
}
