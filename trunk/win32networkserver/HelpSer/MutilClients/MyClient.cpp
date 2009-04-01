#include "stdafx.h"
#include "MyClient.h"
#include "unhandledexception.h"

using namespace std;			

__my_set_exception_handler g_Exception;		//捕获程序运行过程中异常信息

MyClient::~MyClient()
{
	if (m_pNet)
	{
		delete m_pNet;
		m_pNet = NULL;
	}
}

BOOL MyClient::Init(DWORD nId , const char* szSerIp , int nPort , BOOL bType)
{
	BOOL bRet = TRUE;
	try
	{
		m_nId = nId;
		m_nCount = 0;
		m_pNet = new ClientNet(TRUE);
		ASSERT(m_pNet);

		if (NULL == m_pNet)
		{
			throw ((long)(__LINE__));
		}

		//设置相关回调函数, TCP类型的客户端
		if (bType)
		{
			m_pNet->SetTCPCallback(this, NULL, ConnectPrco, ReadProc, ErrorProc);

			//连接TCP服务器
			if (FALSE == m_pNet->Connect(szSerIp, nPort, FALSE, NULL, 0))
			{
				cout << "连接到服务器失败" <<endl;
				throw ((long)__LINE__);
			}
		}
		else
		{
			m_pNet->SetUDPCallback(this, ReadFromProc, ErrorProc);

			//初始化UDP客户端
			if (FALSE == m_pNet->Bind(0, FALSE, NULL))
			{
				cout << "绑定本地UDP端口失败" << endl;
				throw ((long)__LINE__);
			}

			//向UDP服务器发送第一份数据
			const INT BUF_SIZE = 512;
			char szBuf[BUF_SIZE] = { 0 };
			sprintf(szBuf, "[%ld]<UDP> MyClient Data 0", m_nId);
			if (FALSE == m_pNet->SendTo(IP_ADDR(szSerIp, nPort), szBuf, (int)(strlen(szBuf) +1)))
			{
				cout << '[' << m_nId << ']' << " 向服务器发送第1份UDP数据包失败" << endl;
				throw ((long)__LINE__);
			}
		}

	}
	catch (const long& iErrLine)
	{
		bRet = FALSE;
		TRACE("\r\nExcept : %s--%ld", __FILE__, iErrLine);
	}
	
	return bRet;
}

void MyClient::ConnectPrco(LPVOID pParam)
{
	//成功连接到服务器向服务器发送第一份TCP数据包
	MyClient* const pClient = (MyClient*)pParam;
	ASSERT(pClient);
	
	const INT BUF_SIZE = 2048;
	char szBuf[BUF_SIZE] = { 0 };
		
	sprintf(szBuf, "[%ld]<TCP> MyClient Data 0", pClient->m_nId);

	if (FALSE == pClient->m_pNet->Send(szBuf, (int)(sizeof(szBuf) -1)))
	{
		printf("[%ld] 向服务器第1份TCP数据包失败\r\n", pClient->m_nId);
	}	
}

int MyClient::ReadProc(LPVOID pParam , const char* szData , int nLen )
{
	cout << "收到来自服务器的TCP数据包 DATA = " << szData << endl;

	TRACE("\r\n strlen(szData) = %ld", strlen(szData));
	//向服务器发送下一份数据包
	MyClient* const pClient = (MyClient*)pParam;
	ASSERT(pClient);
	pClient->m_nCount++;

	const INT BUF_SIZE = 2048;
	char szBuf[BUF_SIZE] = { 0 };

	sprintf(szBuf, "[%ld]<TCP> MyClient Data %ld", pClient->m_nId, pClient->m_nCount);
	Sleep(200);
	if (FALSE == pClient->m_pNet->Send(szBuf, (int)(sizeof(szBuf) -1)))
	{
		printf("[%ld] 向服务器第%ld份TCP数据包失败\r\n", pClient->m_nId, pClient->m_nCount);
	}

	return 0;
}

void MyClient::ReadFromProc(LPVOID pParam , const IP_ADDR& peer_addr , const char* szData , int nLen )
{
	cout << "收到来自服务器的UDP数据包 DATA = " << szData <<endl;

	//向服务器发送下一份数据包
	MyClient* const pClient = (MyClient*)pParam;
	ASSERT(pClient);
	pClient->m_nCount++;

	const INT BUF_SIZE = 512;
	char szBuf[BUF_SIZE] = { 0 };
	sprintf(szBuf, "[%ld]<UDP> MyClient Data %ld", pClient->m_nId, pClient->m_nCount);
	Sleep(50);
	if (FALSE == pClient->m_pNet->SendTo(peer_addr, szBuf, (int)(strlen(szBuf) +1)))
	{
		printf("[%ld] 向服务器第%ld份UDP数据包失败\r\n", pClient->m_nId, pClient->m_nCount);
	}
	return;
}

void MyClient::ErrorProc(LPVOID pParam, int nOpt)
{
	MyClient* const pClient = (MyClient*)pParam;
	ASSERT(pClient);

	printf("[%ld] 的网络错误 ERR_CODE = %ld\r\n", pClient->m_nId, nOpt);
}
