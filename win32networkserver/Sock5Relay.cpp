#include "Sock5Relay.h"
#include <assert.h>

#pragma pack(1)

struct SOCK5REQ1
{
	char Ver;			//sock 代理版本
	char nMethods;		//认证方式
	char Methods[255];	//方法序列
};

struct SOCK5REQ2
{
	char ver;			//sock代理版本
	char cmd;			//socket命令
	char rsv;			//保留
	char atyp;			//地址类型
	ULONG ulAddr;		//特定地址
	WORD wPort;			//特定端口
};


//UDP代理的头部信息
struct UDP_SEND_HEAD
{
	WORD m_nRsv;			//保留
	CHAR m_bSeg;			//是否分片
	CHAR m_atpy;			//地址类型
	ULONG m_Addr;			//目的地址
	WORD m_nPort;			//目的端口
};

#pragma pack()

namespace HelpMng
{

	Sock5Relay::Sock5Relay(		
		const string& szUser
		, const string& szPwd
		, const CHAR* szDstIp
		, INT nDstPort
		, LPONREAD pReadFun			//代理成功以后的数据读取回调函数
		, LPONERROR pErrorFun		//代理的错误处理函数
		, LPRELAY_RESULT pResultFun	//代理结果的回调函数
		, LPVOID pParam				//传给回调函数的参数
		)
		: m_szUser(szUser)
		, m_szPwd(szPwd)
		, m_nDstPort(nDstPort)
		, m_nNetType(SOCK_STREAM)
		, m_pReadProc(pReadFun)
		, m_pErrorProc(pErrorFun)
		, m_pResultProc(pResultFun)
		, m_pParam(pParam)
		, m_pRelayNet(NULL)
		, m_pUdpNet(NULL)
		, m_nState(STATE_INIT)
	{

		assert(szDstIp);
		strcpy(m_szDstIP, szDstIp);

		m_pRelayNet = new ClientNet(TRUE);
		assert(m_pRelayNet);

		//设置相关回调函数
		m_pRelayNet->SetTCPCallback(this, NULL, ConnectProc, ReadProc, ErrorProc);

		if (szUser.empty())
		{
			m_bAuth = FALSE;
		}
		else
		{
			m_bAuth = TRUE;
		}
	}

	Sock5Relay::Sock5Relay(
		const string& szUser
		, const string& szPwd
		, INT nLocalPort
		, LPONREADFROM pReadFromFun
		, LPONERROR pErrorFun
		, LPRELAY_RESULT pResultFun
		, LPVOID pParam
		)
		: m_szUser(szUser)
		, m_szPwd(szPwd)
		, m_nLocalPort(nLocalPort)
		, m_nNetType(SOCK_DGRAM)
		, m_pRelayNet(NULL)
		, m_pUdpNet(NULL)
		, m_pReadFromProc(pReadFromFun)
		, m_pErrorProc(pErrorFun)
		, m_pResultProc(pResultFun)
		, m_pParam(pParam)
		, m_nState(STATE_INIT)
	{	

		m_pRelayNet = new ClientNet(TRUE);
		m_pUdpNet = new ClientNet(TRUE);

		
		assert(m_pRelayNet);
		assert(m_pUdpNet);

		if (szUser.empty())
		{
			m_bAuth = FALSE;
		}
		else
		{
			m_bAuth = TRUE;
		}
	}

	Sock5Relay::~Sock5Relay(void)
	{
		if (m_pRelayNet)
		{
			delete m_pRelayNet;
			m_pRelayNet = NULL;
		}

		if (m_pUdpNet)
		{
			delete m_pUdpNet;
			m_pUdpNet = NULL;
		}
	}
	
	BOOL Sock5Relay::Start(const char* szRelayIp, int nSerPort )
	{
		assert(pOnResult);
		m_pOnResult = pOnResult;
		m_pParam = pParam;

		//为网络接口设置回调函数
		switch(m_nNetType)
		{			
		case SOCK_STREAM:		//tcp代理方式
			m_pNet->SetTCPCallback(this, NULL, OnConnect, OnRead, OnError);
			break;

		case SOCK_DGRAM:		//udp代理方式
			break;

		default:
			assert(0);
			break;
		}
	}

	INT Sock5Relay::OnRead(LPVOID pParam, const CHAR* szData, INT nDataLen)
	{
		INT nRet = 0;

		switch (m_nState)
		{
		case STATE_RELAY_STEP_1:		//与代理服务器进行验证的第一步
			if (nDataLen < 2)
			{
				InterlockedExchange(&m_nState, STATE_RELAY_FAILE);
			}
			//S -> C : 服务器应返回 版本号 | 服务器选定的方法
			//第一步验证通过进行第二步
			//C -> S: 协议版本 | Socks命令 |　保留字节　| 地址类型 | 特定地址 | 特定端口
			if (0x05 == szData[0] && 0x00 == szData[1])
			{
				InterlockedExchange(&m_nState, STATE_RELAY_STEP_2);
				CHAR szBuf[256];
				SOCK5REQ2* pReq = (SOCK5REQ2*)szBuf;
				pReq->ver = 0x05;
				pReq->rsv = 0;
				pReq->atyp = 0x01;

				//TCP方式代理
				if (SOCK_STREAM == m_nNetType)
				{
					pReq->cmd = 0x01;
					pReq->ulAddr = inet_addr(m_szDstIP);
					pReq->wPort = htons(m_nDstPort);
				}
				//UDP方式代理
				else
				{
					pReq->cmd = 0x03;
					pReq->ulAddr = 0;
					pReq->wPort = htons(m_nLocalPort);
				}

				if (FALSE == CRelay::Send(szBuf, sizeof(SOCK5REQ2)))
				{
					InterlockedExchange(&m_nState, STATE_RELAY_FAILE);
				}
			} 
			else if (0x05 == szData[0] && 0x02 == szData[1])
			{
				//S -> C : 服务器应返回 版本号 | 服务器选定的方法
				//C -> S: 0x01 | 用户名长度（1字节）| 用户名（长度根据用户名长度域指定） | 口令长度（1字节） | 口令（长度由口令长度域指定）
				InterlockedExchange(&m_nState, STATE_RELAY_STEP_1_AUTH);
				INT nAllLen = 3;
				INT nTemp;
#define LEN 100
				CHAR szBuf[512] = { 0 };

				szBuf[0] = 0x01;
				nTemp = (INT)(m_szUser.length() > LEN ? LEN : m_szUser.length());
				szBuf[1] = (CHAR)nTemp;
				strcpy(szBuf+2, m_szUser.c_str());
				nAllLen += nTemp;
				szBuf[nAllLen -1] = (CHAR)(m_szPwd.length() > LEN ? LEN : m_szPwd.length());
				nTemp = szBuf[nAllLen -1];
				strcpy(szBuf +nAllLen, m_szPwd.c_str());
				nAllLen += nTemp;

				if (FALSE == CRelay::Send(szBuf, nAllLen))
				{
					InterlockedExchange(&m_nState, STATE_RELAY_FAILE);
				}

			}
			else
			{
				InterlockedExchange(&m_nState, STATE_RELAY_FAILE);
			}
			break;

		case STATE_RELAY_STEP_1_AUTH:		//用户密码验证的方式
			//S -> C: 0x01 | 验证结果标志
			//C -> S: 协议版本 | Socks命令 |　保留字节　| 地址类型 | 特定地址 | 特定端口
			if (nDataLen < 2)
			{
				InterlockedExchange(&m_nState, STATE_RELAY_FAILE);
			}
			if (0x05 == szData[0] && 0x00 == szData[1])
			{
				InterlockedExchange(&m_nState, STATE_RELAY_STEP_2);
				CHAR szBuf[256];
				SOCK5REQ2* pReq = (SOCK5REQ2*)szBuf;
				pReq->ver = 0x05;
				pReq->rsv = 0;
				pReq->atyp = 0x01;

				//TCP方式代理
				if (SOCK_STREAM == m_nNetType)
				{
					pReq->cmd = 0x01;
					pReq->ulAddr = inet_addr(m_szDstIP);
					pReq->wPort = htons(m_nDstPort);
				}
				//UDP方式代理
				else
				{
					pReq->cmd = 0x03;
					pReq->ulAddr = 0;
					pReq->wPort = htons(m_nLocalPort);
				}

				if (FALSE == CRelay::Send(szBuf, sizeof(SOCK5REQ2)))
				{
					InterlockedExchange(&m_nState, STATE_RELAY_FAILE);
				}
			}
			else
			{
				InterlockedExchange(&m_nState, STATE_RELAY_FAILE);
			}
			break;

		case STATE_RELAY_STEP_2:		//与代理服务器的第二步验证
			//S -> C:  版本 | 代理的应答 |　保留1字节　| 地址类型 | 代理服务器地址 | 绑定的代理端口
			if (nDataLen < sizeof(SOCK5REQ2))
			{
				InterlockedExchange(&m_nState, STATE_RELAY_FAILE);
			}
			if (0x05 == szData[0] && 0 == szData[1])
			{
				InterlockedExchange(&m_nState, STATE_RELAY_SUCCESS);
				SOCK5REQ2* pReq = (SOCK5REQ2*)szData;

				if (SOCK_DGRAM == m_nNetType)
				{
					m_SerAddr.sin_port = pReq->wPort;
					m_SerAddr.sin_addr.s_addr = pReq->ulAddr;

					//为m_UdpSock投递接收操作
					PostRecv(m_pUdpRcvContext, m_UdpSock);
				}
			}
			else
			{
				InterlockedExchange(&m_nState, STATE_RELAY_FAILE);
			}
			break;

		default:
			break;

		}
		return nRet;
	}


	void Sock5Relay::OnConnect(INT iErrorCode)
	{
		//连接失败
		if (0 != iErrorCode)
		{
			//连接代理服务器失败
			if (STATE_CONNECT_SER == m_nState)
			{
				InterlockedExchange(&m_nState, STATE_RELAY_FAILE);
			}
		}
		//连接成功
		else if (STATE_CONNECT_SER == m_nState)
		{		
			//成功连接到代理服务器, 向服务器发送认证信息
			//版本号（1字节） | 可供选择的认证方法(1字节) | 方法序列（1-255个字节长度）

			//投递读操作
			if (FALSE == PostRecv(m_pRcvContext, m_hSock))
			{
				InterlockedExchange(&m_nState, STATE_RELAY_FAILE);
				return;
			}

			CHAR szBuf[512];
			SOCK5REQ1* pReq1 = (SOCK5REQ1*)szBuf;
			INT nLen = 0;
			if (FALSE == m_bAuth)
			{
				pReq1->Ver = 0x05;
				pReq1->nMethods = 0x01;
				pReq1->Methods[0] = 0x00;
				nLen = 3;				
			}
			else
			{
				pReq1->Ver = 0x05;
				pReq1->nMethods = 0x01;
				pReq1->Methods[0] = 0x02;
				nLen = 3;
			}			

			InterlockedExchange(&m_nState, STATE_RELAY_STEP_1);

			if (FALSE == CRelay::Send(szBuf, nLen))
			{
				InterlockedExchange(&m_nState, STATE_RELAY_FAILE);
			}		
		}
	}

	BOOL Sock5Relay::SendTo(const IP_ADDR& PeerAddr, const CHAR* szData, INT nLen)
	{
		//数据非法
		if (NULL == szData || (nLen + sizeof(UDP_SEND_HEAD)) > RELAY_CONTEXT::BUF_SIZE 
			|| SOCK_DGRAM != m_nNetType || STATE_RELAY_SUCCESS != m_nState)
		{
			return FALSE;
		}

		BOOL bResult = TRUE;
		INT iErrCode = 0;

		//网络上没有发送操作直接进行投递操作
		if ( 0 == m_pUdpSendContext->m_nPostCount)
		{
			WSABUF SendBuf;
			DWORD dwBytes;
			InterlockedExchange(&(m_pUdpSendContext->m_nPostCount), 1);
			m_pUdpSendContext->m_hSock = m_UdpSock;
			m_pUdpSendContext->m_lAllData = nLen + sizeof(UDP_SEND_HEAD);
			m_pUdpSendContext->m_nDataLen = 0;
			m_pUdpSendContext->m_nOperation = OP_WRITE;
			m_pUdpSendContext->m_PeerIP = m_SerAddr;
			memcpy(m_pUdpSendContext->m_pBuf +sizeof(UDP_SEND_HEAD), szData, nLen);	
			UDP_SEND_HEAD* pHead = (UDP_SEND_HEAD*)m_pUdpSendContext->m_pBuf;
			pHead->m_Addr = PeerAddr.sin_addr.s_addr;
			pHead->m_atpy = 1;
			pHead->m_bSeg = 0;
			pHead->m_nPort = PeerAddr.sin_port;
			pHead->m_nRsv = 0;
			SendBuf.buf = m_pUdpSendContext->m_pBuf;
			SendBuf.len = m_pUdpSendContext->m_lAllData;

			iErrCode = WSASendTo(m_UdpSock, &SendBuf, 1, &dwBytes, 0
				,(sockaddr*)&(m_pUdpSendContext->m_PeerIP), sizeof(m_pUdpSendContext->m_PeerIP), &(m_pUdpSendContext->m_ol), NULL);
			if (SOCKET_ERROR == iErrCode && ERROR_IO_PENDING != WSAGetLastError())
			{
				closesocket(m_UdpSock);
				InterlockedExchange(&(m_pUdpSendContext->m_nPostCount), 0);
				bResult = FALSE;
			}
		}
		//网络IO正在进行发送操作, 将数据放入队列中
		else
		{
			EnterCriticalSection(&m_SendDataLock);

			RELAY_SEND_DATA* pSendData = new RELAY_SEND_DATA(nLen + sizeof(UDP_SEND_HEAD));
			pSendData->m_PeerIP = m_SerAddr;
			memcpy(pSendData->m_pData +sizeof(UDP_SEND_HEAD), szData, nLen);
			UDP_SEND_HEAD* pHead = (UDP_SEND_HEAD*)(pSendData->m_pData);
			pHead->m_Addr = PeerAddr.sin_addr.s_addr;
			pHead->m_atpy = 1;
			pHead->m_bSeg = 0;
			pHead->m_nPort = PeerAddr.sin_port;
			pHead->m_nRsv = 0;

			if (RELAY_SEND_DATA::IsAddressValid(pSendData) && pSendData->IsDataValid())
			{
				m_SendDataQue.push_back(pSendData);
			}
			else
			{
				delete pSendData;
				bResult = FALSE;
			}

			LeaveCriticalSection(&m_SendDataLock);
		}

		return bResult;
	}

	const CHAR* Sock5Relay::ParseUdpData(const CHAR* szBuf, INT nBufLen, IP_ADDR& PeerAddr, INT& nDataLen)
	{
		//数据非法
		if (nBufLen < sizeof(UDP_SEND_HEAD))
		{
			return NULL;
		}

		UDP_SEND_HEAD* pHead = (UDP_SEND_HEAD*)szBuf;
		PeerAddr.sin_addr.s_addr = pHead->m_Addr;
		PeerAddr.sin_port = pHead->m_nPort;

		nDataLen = nBufLen - sizeof(UDP_SEND_HEAD);
		return (szBuf +sizeof(UDP_SEND_HEAD));
	}

}