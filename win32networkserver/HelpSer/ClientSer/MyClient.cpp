#include "stdafx.h"
#include "MyClient.h"
#include "unhandledexception.h"

using namespace std;			

__my_set_exception_handler g_Exception;		//捕获程序运行过程中异常信息

MyClient::MyClient(const SOCKET& s, DWORD nId)
	: m_nId(nId)
{
	m_pNet = new ClientNet(s, TRUE, this, ReadProc, ErrorProc);
	ASSERT(m_pNet);
}

MyClient::~MyClient()
{
	if (m_pNet)
	{
		delete m_pNet;
		m_pNet = NULL;
	}
}

int MyClient::ReadProc(LPVOID pParam , const char* szData , int nLen )
{
	cout << "收到来自客户端的TCP数据包 DATA = " << szData << endl;

	//将数据包返回给客户端
	MyClient* const pClient = (MyClient*)pParam;
	ASSERT(pClient);

	if (FALSE == pClient->m_pNet->Send(szData, nLen))
	{
		printf("[%ld] 向客户端返回数据包失败\r\n", pClient->m_nId);
	}

	return 0;
}

void MyClient::ErrorProc(LPVOID pParam, int nOpt)
{
	MyClient* const pClient = (MyClient*)pParam;
	ASSERT(pClient);

	printf("[%ld] 的网络错误 ERR_CODE = %ld\r\n", pClient->m_nId, nOpt);

	//删除该客户端
	delete pClient;
}
