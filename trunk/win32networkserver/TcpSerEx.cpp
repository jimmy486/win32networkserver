#include <process.h>
#include <assert.h>
#include <time.h>

#include "TcpSerEx.h"
#include "RunLog.h"
#include <atltrace.h>

//#define _XML_NET_
#define _DEBUG_SQ

namespace HelpMng
{
	//静态成员变量初始化
	vector<TCP_CONTEXT_EX* > TCP_CONTEXT_EX::s_IDLQue;	
	CRITICAL_SECTION TCP_CONTEXT_EX::s_IDLQueLock;	
	HANDLE TCP_CONTEXT_EX::s_hHeap = NULL;	

	vector<ACCEPT_CONTEXT_EX* > ACCEPT_CONTEXT_EX::s_IDLQue;
	CRITICAL_SECTION ACCEPT_CONTEXT_EX::s_IDLQueLock;
	HANDLE ACCEPT_CONTEXT_EX::s_hHeap = NULL;

	HANDLE TCP_RCV_DATA_EX::s_DataHeap = NULL;
	HANDLE TCP_RCV_DATA_EX::s_hHeap = NULL;
	vector<TCP_RCV_DATA_EX* > TCP_RCV_DATA_EX::s_IDLQue;
	CRITICAL_SECTION TCP_RCV_DATA_EX::s_IDLQueLock;

	LPFN_ACCEPTEX TcpSerEx::s_pfAcceptEx = NULL;
	LPFN_GETACCEPTEXSOCKADDRS TcpSerEx::s_pfGetAddrs = NULL;
	BOOL TcpSerEx::s_bInit = FALSE;

	void* TCP_CONTEXT_EX::operator new(size_t nSize)
	{
		void* pContext = NULL;

		try
		{
			if (NULL == s_hHeap)
			{
				THROW_LINE;;
			}

			//申请堆内存, 先从空闲的队列中找若空闲队列为空则从堆内存上申请
			//bool bQueEmpty = true;

			EnterCriticalSection(&s_IDLQueLock);
			vector<TCP_CONTEXT_EX* >::iterator iter = s_IDLQue.begin();

			if (iter != s_IDLQue.end())
			{
				pContext = *iter;
				s_IDLQue.erase(iter);
				//bQueEmpty = false;
			}
			else
			{
				pContext = HeapAlloc(s_hHeap, HEAP_ZERO_MEMORY | HEAP_NO_SERIALIZE, nSize);
			}
			LeaveCriticalSection(&s_IDLQueLock);

			if (NULL == pContext)
			{
				THROW_LINE;;
			}
		}
		catch (const long& iErrCode)
		{
			pContext = NULL;
			_TRACE("Exp : %s--%ld", __FILE__, iErrCode);
		}

		return pContext;
	}

	void TCP_CONTEXT_EX::operator delete(void* p)
	{
		if (p)		
		{
			//若空闲队列长度小于MAX_IDL_DATA, 则将其放入空闲队列中; 否则释放

			EnterCriticalSection(&s_IDLQueLock);
			const DWORD QUE_SIZE = (DWORD)(s_IDLQue.size());
			TCP_CONTEXT_EX* const pContext = (TCP_CONTEXT_EX*)p;

			if (QUE_SIZE <= MAX_IDL_DATA)
			{
				s_IDLQue.push_back(pContext);
			}
			else
			{
				HeapFree(s_hHeap, HEAP_NO_SERIALIZE, p);
			}
			LeaveCriticalSection(&s_IDLQueLock);		
		}

		return;
	}

	void* ACCEPT_CONTEXT_EX:: operator new(size_t nSize)
	{
		void* pContext = NULL;

		try
		{	
			if (NULL == s_hHeap)
			{
				THROW_LINE;
			}

			//申请堆内存, 先从空闲的队列中找若空闲队列为空则从堆内存上申请
			//bool bQueEmpty = true;

			EnterCriticalSection(&s_IDLQueLock);
			vector<ACCEPT_CONTEXT_EX* >::iterator iter = s_IDLQue.begin();

			if (iter != s_IDLQue.end())
			{
				pContext = *iter;
				s_IDLQue.erase(iter);
			}
			else
			{
				pContext = HeapAlloc(s_hHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, nSize);
			}
			LeaveCriticalSection(&s_IDLQueLock);

			if (NULL == pContext)
			{
				THROW_LINE;
			}
		}
		catch (const long& iErrCode)
		{
			pContext = NULL;
			_TRACE("Exp : %s--%ld", __FILE__, iErrCode);
		}

		return pContext;
	}

	void ACCEPT_CONTEXT_EX:: operator delete(void* p)
	{
		if (p)
		{
			//若空闲队列长度小于10000, 则将其放入空闲队列中; 否则释放

			EnterCriticalSection(&s_IDLQueLock);
			const DWORD QUE_SIZE = (DWORD)(s_IDLQue.size());
			const DWORD MAX_IDL = 500;
			ACCEPT_CONTEXT_EX* const pContext = (ACCEPT_CONTEXT_EX*)p;

			if (QUE_SIZE <= MAX_IDL)
			{
				s_IDLQue.push_back(pContext);
			}
			else
			{
				HeapFree(s_hHeap, HEAP_NO_SERIALIZE, p);
			}
			LeaveCriticalSection(&s_IDLQueLock);
		}
		return;
	}


	//TCP_RCV_DATA_EX 相关实现
	TCP_RCV_DATA_EX::TCP_RCV_DATA_EX(SOCKET hSocket, const CHAR* pBuf, INT nLen)
		: m_hSocket(hSocket)
		, m_nLen(nLen)
	{
		try
		{
			assert(pBuf);
			if (NULL == s_DataHeap)
			{
				THROW_LINE;
			}

			m_pData = (CHAR*)HeapAlloc(s_DataHeap, HEAP_ZERO_MEMORY, nLen);
			//assert(m_pData);
			if (m_pData)
			{
				memcpy(m_pData, pBuf, nLen);
			}

		}
		catch (const long& iErrCode)
		{
			m_pData = NULL;
			_TRACE("Exp : %s--%ld", __FILE__, iErrCode);
		}
	}

	TCP_RCV_DATA_EX::~TCP_RCV_DATA_EX()
	{
		if ((NULL != m_pData ) && (NULL != s_DataHeap))
		{
			HeapFree(s_DataHeap, 0, m_pData);
			m_pData = NULL;
		}
	}

	void* TCP_RCV_DATA_EX::operator new(size_t nSize)
	{
		void* pRcvData = NULL;

		try
		{
			if (NULL == s_hHeap || NULL == s_DataHeap)
			{
				THROW_LINE;
			}

			EnterCriticalSection(&s_IDLQueLock);
			vector<TCP_RCV_DATA_EX* >::iterator iter = s_IDLQue.begin();
			if (s_IDLQue.end() != iter)
			{
				pRcvData = *iter;
				s_IDLQue.erase(iter);
			}
			else
			{
				pRcvData = HeapAlloc(s_hHeap, HEAP_ZERO_MEMORY | HEAP_NO_SERIALIZE, nSize);
			}
			LeaveCriticalSection(&s_IDLQueLock);

			if (NULL == pRcvData)
			{
				THROW_LINE;
			}
		}
		catch (const long& iErrCode)
		{
			pRcvData = NULL;
			_TRACE("Exp : %s--%ld", __FILE__, iErrCode);
		}
		return pRcvData;
	}

	void TCP_RCV_DATA_EX::operator delete(void* p)
	{
		if (p)
		{
			EnterCriticalSection(&s_IDLQueLock);
			const DWORD QUE_SIZE = (DWORD)(s_IDLQue.size());
			TCP_RCV_DATA_EX* const pContext = (TCP_RCV_DATA_EX*)p;

			if (QUE_SIZE <= MAX_IDL_DATA)
			{
				s_IDLQue.push_back(pContext);
			}
			else
			{
				HeapFree(s_hHeap, HEAP_NO_SERIALIZE, p);
			}
			LeaveCriticalSection(&s_IDLQueLock);
		}
		return;
	}

	void TcpSerEx::InitReource()
	{
		if (FALSE == s_bInit)
		{
			s_bInit = TRUE;	

			//TCP_CONTEXT_EX
			TCP_CONTEXT_EX::s_hHeap = HeapCreate(0, TCP_CONTEXT_EX::S_PAGE_SIZE, TCP_CONTEXT_EX::E_TCP_HEAP_SIZE);
			InitializeCriticalSection(&(TCP_CONTEXT_EX::s_IDLQueLock));
			TCP_CONTEXT_EX::s_IDLQue.reserve(TCP_CONTEXT_EX::MAX_IDL_DATA * sizeof(void*));
			//_TRACE("%s:%ld TCPMNG_CONTEXT::s_hHeap = 0x%x sizeof(TCPMNG_CONTEXT*) = %ld"
			//	, __FILE__, __LINE__, TCPMNG_CONTEXT::s_hHeap, sizeof(TCPMNG_CONTEXT*));

			//CONNECT_CONTEXT
			//CONNECT_CONTEXT::s_hHeap = HeapCreate(0, CONNECT_CONTEXT::S_PAGE_SIZE, CONNECT_CONTEXT::HEAP_SIZE);
			//InitializeCriticalSection(&(CONNECT_CONTEXT::s_IDLQueLock));

			//ACCEPT_CONTEXT_EX
			ACCEPT_CONTEXT_EX::s_hHeap = HeapCreate(0, ACCEPT_CONTEXT_EX::S_PAGE_SIZE, 500 * 1024);
			InitializeCriticalSection(&(ACCEPT_CONTEXT_EX::s_IDLQueLock));
			ACCEPT_CONTEXT_EX::s_IDLQue.reserve(500 * sizeof(void*));
			//_TRACE("%s:%ld nACCEPT_CONTEXT::s_hHeap = 0x%x", __FILE__, __LINE__, ACCEPT_CONTEXT_EX::s_hHeap);

			//TCP_RCV_DATA_EX
			TCP_RCV_DATA_EX::s_hHeap = HeapCreate(0, 0, TCP_RCV_DATA_EX::HEAP_SIZE);
			TCP_RCV_DATA_EX::s_DataHeap = HeapCreate(0, 0, TCP_RCV_DATA_EX::DATA_HEAP_SIZE);
			InitializeCriticalSection(&(TCP_RCV_DATA_EX::s_IDLQueLock));
			TCP_RCV_DATA_EX::s_IDLQue.reserve(TCP_RCV_DATA_EX::MAX_IDL_DATA * sizeof(void*));
		}
	}

	void TcpSerEx::ReleaseReource()
	{
		if (s_bInit)
		{
			s_bInit = FALSE;

			//TCP_CONTEXT_EX
			HeapDestroy(TCP_CONTEXT_EX::s_hHeap);
			TCP_CONTEXT_EX::s_hHeap = NULL;

			EnterCriticalSection(&(TCP_CONTEXT_EX::s_IDLQueLock));
			TCP_CONTEXT_EX::s_IDLQue.clear();
			LeaveCriticalSection(&(TCP_CONTEXT_EX::s_IDLQueLock));
			DeleteCriticalSection(&(TCP_CONTEXT_EX::s_IDLQueLock));

			//ACCEPT_CONTEXT_EX
			HeapDestroy(ACCEPT_CONTEXT_EX::s_hHeap);
			ACCEPT_CONTEXT_EX::s_hHeap = NULL;

			EnterCriticalSection(&(ACCEPT_CONTEXT_EX::s_IDLQueLock));
			ACCEPT_CONTEXT_EX::s_IDLQue.clear();
			LeaveCriticalSection(&(ACCEPT_CONTEXT_EX::s_IDLQueLock));
			DeleteCriticalSection(&(ACCEPT_CONTEXT_EX::s_IDLQueLock));

			//TCP_RCV_DATA_EX
			HeapDestroy(TCP_RCV_DATA_EX::s_hHeap);
			TCP_RCV_DATA_EX::s_hHeap = NULL;

			HeapDestroy(TCP_RCV_DATA_EX::s_DataHeap);
			TCP_RCV_DATA_EX::s_DataHeap = NULL;

			EnterCriticalSection(&(TCP_RCV_DATA_EX::s_IDLQueLock));
			TCP_RCV_DATA_EX::s_IDLQue.clear();
			LeaveCriticalSection(&(TCP_RCV_DATA_EX::s_IDLQueLock));
			DeleteCriticalSection(&(TCP_RCV_DATA_EX::s_IDLQueLock));				
		}
	}

	TcpSerEx::TcpSerEx(LPCLOSE_ROUTINE pCloseFun , void *pParam)
		: m_pCloseFun(pCloseFun)
		, m_pCloseParam(pParam)
		, m_bThreadRun(TRUE)
		, m_bSerRun(FALSE)
		, m_nAcceptCount(0)
		, m_nReadCount(0)
	{
		m_RcvDataQue.reserve(10000 * sizeof(void *));
		m_SocketQue.reserve(50000 * sizeof(SOCKET));

		InitializeCriticalSection(&m_RcvQueLock);
		InitializeCriticalSection(&m_SockQueLock);

		//创建监听事件
		for (int nIndex = 0; nIndex < LISTEN_EVENTS; nIndex ++)
		{
			m_ListenEvents[nIndex] = CreateEvent(NULL, FALSE, FALSE, NULL);
		}

		m_Threads[0] = (HANDLE)_beginthreadex(NULL, 0, ListenThread, this, 0, NULL);
		m_Threads[1] = (HANDLE)_beginthreadex(NULL, 0, AideThread, this, 0, NULL);
	}

	TcpSerEx::~TcpSerEx()
	{
		//通知后台线程退出
		InterlockedExchange((long volatile *)&m_bThreadRun, FALSE);

		WaitForMultipleObjects(THREAD_NUM, m_Threads, TRUE, 30 * 1000);
		//_TRACE("~TcpSerEx() : TIME = %u", GetTickCount());

		for (DWORD nIndex = 0; nIndex < THREAD_NUM; nIndex ++)
		{
			CloseHandle(m_Threads[nIndex]);
		}

		//确保所有的accept操作完成
		while (InterlockedExchangeAdd(&m_nAcceptCount, 0) || InterlockedExchangeAdd(&m_nReadCount, 0))
		{
			Sleep(100);
		}

		EnterCriticalSection(&m_SockQueLock);
		m_SocketQue.clear();
		LeaveCriticalSection(&m_SockQueLock);

		EnterCriticalSection(&m_RcvQueLock);
		for (vector<TCP_RCV_DATA_EX *>::iterator item_iter = m_RcvDataQue.begin(); m_RcvDataQue.end() != item_iter;)
		{
			TCP_RCV_DATA_EX *pRecvData = *item_iter;
			delete pRecvData;
			m_RcvDataQue.erase(item_iter);
		}
		//m_RcvDataQue.clear();
		LeaveCriticalSection(&m_RcvQueLock);

		CloseHandle(m_ListenEvents[0]);
		CloseHandle(m_ListenEvents[1]);
		DeleteCriticalSection(&m_RcvQueLock);
		DeleteCriticalSection(&m_SockQueLock);	
	}

	BOOL TcpSerEx::Init( const char *szIp , int nPort  )
	{
		BOOL bSucc = TRUE;
		int nRet = 0;
		DWORD dwBytes = 0;
		ULONG ul = 1;
		int nOpt = 1;

		try 
		{
			//若服务器正在运行, 则不允许启动新的服务
			if (m_bSerRun || m_nAcceptCount)
			{
				THROW_LINE;;
			}	

			//创建监听套接字
			m_hSock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
			if (INVALID_SOCKET == m_hSock)
			{
				THROW_LINE;;
			}

			//加载AcceptEx函数			
			GUID guidProc = WSAID_ACCEPTEX;
			if (NULL == s_pfAcceptEx)
			{
				nRet = WSAIoctl(m_hSock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidProc, sizeof(guidProc)
					, &s_pfAcceptEx, sizeof(s_pfAcceptEx), &dwBytes, NULL, NULL);
			}
			if (NULL == s_pfAcceptEx || SOCKET_ERROR == nRet)
			{
				THROW_LINE;;
			}

			//加载GetAcceptExSockaddrs函数
			GUID guidGetAddr = WSAID_GETACCEPTEXSOCKADDRS;
			dwBytes = 0;
			if (NULL == s_pfGetAddrs)
			{
				nRet = WSAIoctl(m_hSock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidGetAddr, sizeof(guidGetAddr)
					, &s_pfGetAddrs, sizeof(s_pfGetAddrs), &dwBytes, NULL, NULL);
			}
			if (NULL == s_pfGetAddrs)
			{
				THROW_LINE;;
			}

			//启动监听服务，并投递ACCEPT_NUM个accept操作			
			ioctlsocket(m_hSock, FIONBIO, &ul);

			//设置为地址重用，优点在于服务器关闭后可以立即启用			
			setsockopt(m_hSock, SOL_SOCKET, SO_REUSEADDR, (char*)&nOpt, sizeof(nOpt));

			sockaddr_in LocalAddr;
			LocalAddr.sin_family = AF_INET;
			LocalAddr.sin_port = htons(nPort);
			if (szIp)
			{
				LocalAddr.sin_addr.s_addr = inet_addr(szIp);
			}			
			else
			{
				LocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);
			}

			//绑定到监听socket上
			nRet = bind(m_hSock, (sockaddr*)&LocalAddr, sizeof(LocalAddr));
			if (SOCKET_ERROR == nRet)
			{
				THROW_LINE;;
			}

			nRet = listen(m_hSock, 200);
			if (SOCKET_ERROR == nRet)
			{
				THROW_LINE;;
			}

			//将监听套接字绑定到完成端口上
			CreateIoCompletionPort((HANDLE)m_hSock, s_hCompletion, (ULONG_PTR)this, 0);
			WSAEventSelect(m_hSock, m_ListenEvents[0], FD_ACCEPT);

			//先投递MAX_ACCEPT个accept操作			
			for (int nIndex = 0; nIndex < MAX_ACCEPT; )
			{
				SOCKET hClient = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
				if (INVALID_SOCKET == hClient)
				{
					continue;
				}

				ul = 1;
				ioctlsocket(hClient, FIONBIO, &ul);

				ACCEPT_CONTEXT_EX* pAccContext = new ACCEPT_CONTEXT_EX();
				if (NULL == pAccContext)
				{
					THROW_LINE;;
				}

				pAccContext->m_hSock = m_hSock;
				pAccContext->m_hRemoteSock = hClient;
				pAccContext->m_nOperation = OP_ACCEPT;

				nRet = s_pfAcceptEx(m_hSock, hClient, pAccContext->m_pBuf, 0
					, sizeof(sockaddr_in) +16, sizeof(sockaddr_in) +16, &dwBytes, &(pAccContext->m_ol));

				if (FALSE == nRet && ERROR_IO_PENDING != WSAGetLastError())
				{
					closesocket(hClient);
					delete pAccContext;
					pAccContext = NULL;
					THROW_LINE;;
				}
				else
				{
					InterlockedExchangeAdd(&m_nAcceptCount, 1);
				}

				nIndex++;
			}

			m_bSerRun = TRUE;
		}
		catch (const long &lErrLine)
		{
			bSucc = FALSE;
			//m_bSerRun = FALSE;
			_TRACE("Exp : %s -- %ld", __FILE__, lErrLine);
		}
		return bSucc;
	}

	void TcpSerEx::Close()
	{
		//关闭所有的socket
		closesocket(m_hSock);

		EnterCriticalSection(&m_SockQueLock);

		for (vector<SOCKET>::iterator iter_sock = m_SocketQue.begin(); m_SocketQue.end() != iter_sock; iter_sock++)
		{
			closesocket(*iter_sock);
		}

		LeaveCriticalSection(&m_SockQueLock);

		//清除接收队列
		EnterCriticalSection(&m_RcvQueLock);
		for (vector<TCP_RCV_DATA_EX *>::iterator item_iter = m_RcvDataQue.begin(); m_RcvDataQue.end() != item_iter;)
		{
			TCP_RCV_DATA_EX *pRecvData = *item_iter;
			delete pRecvData;
			m_RcvDataQue.erase(item_iter);
		}
		//m_RcvDataQue.clear();
		LeaveCriticalSection(&m_RcvQueLock);

		m_bSerRun = FALSE;
	}

	BOOL TcpSerEx::SendData(const char * szData, int nDataLen, SOCKET hSock /* = INVALID_SOCKET */)
	{
#ifdef _XML_NET_
		//数据长度非法
		if (((DWORD)nDataLen > TCP_CONTEXT_EX::S_PAGE_SIZE) || (NULL == szData))
		{
			return FALSE;
		}
#else
		//数据长度非法
		if ((nDataLen > (int)(TCP_CONTEXT_EX::S_PAGE_SIZE)) || (NULL == szData) || (nDataLen < sizeof(PACKET_HEAD)))
		{
			return FALSE;
		}
#endif	//#ifdef _XML_NET_

		BOOL bResult = TRUE;
		DWORD dwBytes = 0;
		WSABUF SendBuf;
		TCP_CONTEXT_EX *pSendContext = new TCP_CONTEXT_EX();
		if (pSendContext && pSendContext->m_pBuf)
		{
			pSendContext->m_hSock = hSock;
			pSendContext->m_nDataLen = nDataLen;			//测试
			pSendContext->m_nOperation = OP_WRITE;
			memcpy(pSendContext->m_pBuf, szData, nDataLen);

			SendBuf.buf = pSendContext->m_pBuf;
			SendBuf.len = nDataLen;

			assert(szData);		
			INT iErr = WSASend(pSendContext->m_hSock, &SendBuf, 1, &dwBytes, 0, &(pSendContext->m_ol), NULL);

			if (SOCKET_ERROR == iErr && ERROR_IO_PENDING != WSAGetLastError())
			{
				delete pSendContext;
				pSendContext = NULL;
				_TRACE("%s : %ld LAST_ERROR = %ld", __FILE__, __LINE__, WSAGetLastError());
				//SetSocketClosed(sock);			
				bResult = FALSE;
			}
		}
		else
		{
			delete pSendContext;
			bResult = FALSE;
		}

		return bResult;
	}

	void *TcpSerEx::GetRecvData(DWORD* const pQueLen)
	{
		TCP_RCV_DATA_EX* pRcvData = NULL;

		EnterCriticalSection(&m_RcvQueLock);
		vector<TCP_RCV_DATA_EX*>::iterator iter = m_RcvDataQue.begin();
		if (m_RcvDataQue.end() != iter)
		{
			pRcvData = *iter;
			m_RcvDataQue.erase(iter);
		}

		if (NULL != pQueLen)
		{
			*pQueLen = (DWORD)(m_RcvDataQue.size());
		}
		LeaveCriticalSection(&m_RcvQueLock);

		return pRcvData;
	}

	void TcpSerEx::IOCompletionProc(BOOL bSuccess, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		NET_CONTEXT * pContext = CONTAINING_RECORD(lpOverlapped, NET_CONTEXT, m_ol);

		if (pContext)
		{
			switch (pContext->m_nOperation)
			{
			case OP_ACCEPT:
				AcceptCompletionProc(bSuccess, dwNumberOfBytesTransfered, lpOverlapped);
				break;
			case OP_READ:
				RecvCompletionProc(bSuccess, dwNumberOfBytesTransfered, lpOverlapped);
				break;
			case OP_WRITE:
				SendCompletionProc(bSuccess, dwNumberOfBytesTransfered, lpOverlapped);
				break;
			}
		}
	}

	void TcpSerEx::AcceptCompletionProc(BOOL bSuccess, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		ACCEPT_CONTEXT_EX *pContext = CONTAINING_RECORD(lpOverlapped, ACCEPT_CONTEXT_EX, m_ol);
		INT nZero = 0;
		DWORD nPro = 0;
		IP_ADDR* pClientAddr = NULL;
		IP_ADDR* pLocalAddr = NULL;
		INT nClientLen = 0;
		INT nLocalLen = 0;
		int iErrCode;
		DWORD nFlag = 0;			
		DWORD nBytes = 0;
		WSABUF RcvBuf;

		if (bSuccess)
		{
			setsockopt(pContext->m_hRemoteSock, SOL_SOCKET, SO_SNDBUF, (char*)&nZero, sizeof(nZero));
			setsockopt(pContext->m_hRemoteSock, SOL_SOCKET, SO_RCVBUF, (CHAR*)&nZero, sizeof(nZero));
			setsockopt(pContext->m_hRemoteSock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&(pContext->m_hSock), sizeof(pContext->m_hSock));
			nPro = (DWORD)time(NULL);
			setsockopt(pContext->m_hRemoteSock, SOL_SOCKET, SO_GROUP_PRIORITY, (char *)&nPro, sizeof(nPro));

			s_pfGetAddrs(pContext->m_pBuf, 0, sizeof(sockaddr_in) +16, sizeof(sockaddr_in) +16
				, (LPSOCKADDR*)&pLocalAddr, &nLocalLen, (LPSOCKADDR*)&pClientAddr, &nClientLen);

			//为新到来的连接投递读操作
			TCP_CONTEXT_EX *pRcvContext = new TCP_CONTEXT_EX;
			if (pRcvContext && pRcvContext->m_pBuf)
			{
				pRcvContext->m_hSock = pContext->m_hRemoteSock;
				pRcvContext->m_nOperation = OP_READ;
				CreateIoCompletionPort((HANDLE)(pRcvContext->m_hSock), s_hCompletion, (ULONG_PTR)this, 0);

				RcvBuf.buf = pRcvContext->m_pBuf;
				RcvBuf.len = TCP_CONTEXT_EX::S_PAGE_SIZE;

				iErrCode = WSARecv(pRcvContext->m_hSock, &RcvBuf, 1, &nBytes, &nFlag, &(pRcvContext->m_ol), NULL);

				//投递失败
				if (SOCKET_ERROR == iErrCode && WSA_IO_PENDING != WSAGetLastError())
				{
					closesocket(pRcvContext->m_hSock);
					delete pRcvContext;
					pRcvContext = NULL;
					_TRACE("%s : %ld  SOCKET = 0x%x LAST_ERROR = %ld", __FILE__, __LINE__, pContext->m_hRemoteSock, WSAGetLastError());
				}
				else
				{
					InterlockedExchangeAdd(&m_nReadCount, 1);

					EnterCriticalSection(&m_SockQueLock);
					m_SocketQue.push_back(pRcvContext->m_hSock);

#ifdef _DEBUG_SQ
					DWORD nClients = (DWORD)(m_SocketQue.size());
					ATLTRACE("\r\n %s -- %ld 当前已有 %ld 个客户端连接到此服务器", __FILE__, __LINE__, nClients);
					_TRACE(" %s -- %ld 当前已有 %ld 个客户端连接到此服务器", __FILE__, __LINE__, nClients);
#endif
					LeaveCriticalSection(&m_SockQueLock);
				}
			}
			else
			{
				delete pRcvContext;
			}

			SetEvent(m_ListenEvents[1]);
		}
		else
		{
			closesocket(pContext->m_hRemoteSock);
			_TRACE(" %s -- %ld accept 操作失败", __FILE__, __LINE__);
		}

		InterlockedExchangeAdd(&m_nAcceptCount, -1);
		//ATLTRACE("\r\n 还有 %ld 个未处理的连接", m_nAcceptCount);
		delete pContext;
		pContext = NULL;
	}

	void TcpSerEx::RecvCompletionProc(BOOL bSuccess, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		TCP_CONTEXT_EX * pRcvContext = CONTAINING_RECORD(lpOverlapped, TCP_CONTEXT_EX, m_ol);
		DWORD dwFlag = 0;
		DWORD dwBytes = 0;
		WSABUF RcvBuf;
		int nErrCode = 0;
		DWORD nPro = 0;
		int nOffSet = 0;

		try
		{
			if ( FALSE == bSuccess || 0 == dwNumberOfBytesTransfered)
			{
				closesocket(pRcvContext->m_hSock);
				THROW_LINE;
			}

			nPro = (DWORD)time(NULL);
			setsockopt(pRcvContext->m_hSock, SOL_SOCKET, SO_GROUP_PRIORITY, (char *)&nPro, sizeof(nPro));

#ifdef _XML_NET_	//处理XML流
			TCP_RCV_DATA_EX* pRcvData = new TCP_RCV_DATA_EX(
				pRcvContext->m_hSock
				, pRcvContext->m_pBuf
				, dwNumberOfBytesTransfered
				);

			if (pRcvData && pRcvData->m_pData)
			{					
				EnterCriticalSection(&m_RcvQueLock);
				m_RcvDataQue.push_back(pRcvData);
				LeaveCriticalSection(&m_RcvQueLock);
			}

			pRcvContext->m_nDataLen = 0;
			RcvBuf.buf = pRcvContext->m_pBuf;
			RcvBuf.len = TCP_CONTEXT_EX::S_PAGE_SIZE;

#else				//处理二进制数据流

			//当前已接收的数据包总长
			pRcvContext->m_nDataLen += dwNumberOfBytesTransfered;		

			while (true)
			{
				if (pRcvContext->m_nDataLen >= sizeof(PACKET_HEAD))
				{
					PACKET_HEAD *pHeadInfo = (PACKET_HEAD *)(pRcvContext->m_pBuf + nOffSet);
					//数据长度合法才进行处理
					if (pHeadInfo->nCurrentLen + sizeof(PACKET_HEAD) <= TCP_CONTEXT_EX::S_PAGE_SIZE )
					{
						//已接收完一个或多个数据包
						if (pHeadInfo->nCurrentLen + sizeof(PACKET_HEAD) <= pRcvContext->m_nDataLen)
						{
							TCP_RCV_DATA_EX *pRcvData = new TCP_RCV_DATA_EX(
								pRcvContext->m_hSock
								, pRcvContext->m_pBuf + nOffSet
								, pHeadInfo->nCurrentLen + sizeof(PACKET_HEAD)
								);

							if (pRcvData && pRcvData->m_pData)
							{
								EnterCriticalSection(&m_RcvQueLock);
								m_RcvDataQue.push_back(pRcvData);
								LeaveCriticalSection(&m_RcvQueLock);
							}

							//为处理下一个数据包做准备
							nOffSet += pHeadInfo->nCurrentLen + sizeof(PACKET_HEAD);
							pRcvContext->m_nDataLen -= nOffSet;
						}
						//数据没有接收完, 需要接收剩余的数据
						else
						{
							if (nOffSet && pRcvContext->m_nDataLen)
							{
								memmove(pRcvContext->m_pBuf, pRcvContext->m_pBuf +nOffSet, pRcvContext->m_nDataLen);
							}
							RcvBuf.buf = pRcvContext->m_pBuf + pRcvContext->m_nDataLen;
							RcvBuf.len = TCP_CONTEXT_EX::S_PAGE_SIZE - pRcvContext->m_nDataLen;
							break;
						}
					}
					//数据非法, 则不处理数据直接进行下一次读投递
					else
					{
						pRcvContext->m_nDataLen = 0;
						RcvBuf.buf = pRcvContext->m_pBuf;
						RcvBuf.len = TCP_CONTEXT_EX::S_PAGE_SIZE;
						break;
					}
				}
				//数据包的长度过短需继续接收
				else
				{
					if (nOffSet && pRcvContext->m_nDataLen)
					{
						memmove(pRcvContext->m_pBuf, pRcvContext->m_pBuf +nOffSet, pRcvContext->m_nDataLen);
					}
					RcvBuf.buf = pRcvContext->m_pBuf + pRcvContext->m_nDataLen;
					RcvBuf.len = TCP_CONTEXT_EX::S_PAGE_SIZE - pRcvContext->m_nDataLen;
					break;			
				}
			}
#endif	//#ifdef _XML_NET_

			//继续投递读操作
			nErrCode = WSARecv(pRcvContext->m_hSock, &RcvBuf, 1, &dwBytes, &dwFlag, &(pRcvContext->m_ol), NULL);
			//Recv操作错误
			if (SOCKET_ERROR == nErrCode && WSA_IO_PENDING != WSAGetLastError())
			{
				closesocket(pRcvContext->m_hSock);
				THROW_LINE;
			}			
		}
		catch (const long &lErrLine)
		{
			_TRACE("Exp : %s -- %ld SOCKET = 0x%x ERR_CODE = 0x%x", __FILE__, lErrLine, pRcvContext->m_hSock, WSAGetLastError());
			InterlockedExchangeAdd(&m_nReadCount, -1);
			delete pRcvContext;			
		}
	}

	void TcpSerEx::SendCompletionProc(BOOL bSuccess, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		TCP_CONTEXT_EX* pSendContext = CONTAINING_RECORD(lpOverlapped, TCP_CONTEXT_EX, m_ol);

#ifdef _DEBUG_SQ
		if (dwNumberOfBytesTransfered != pSendContext->m_nDataLen)
		{
			_TRACE("Exp : %s -- %ld 数据未发送完成 DATA_LEN = %ld SEND_LEN = %ld", __FILE__, __LINE__, pSendContext->m_nDataLen, dwNumberOfBytesTransfered);
			assert(0);
		}
#endif				//#ifdef _DEBUG_SQ

		delete pSendContext;
		pSendContext = NULL;
	}

	UINT WINAPI TcpSerEx::ListenThread(LPVOID lpParam)
	{
		TcpSerEx *pThis = (TcpSerEx *)lpParam;
		try
		{
			int nRet = 0;
			DWORD	nEvents = 0;
			DWORD dwBytes = 0;	
			int nAccept = 0;

			while (TRUE)
			{				
				nEvents = WSAWaitForMultipleEvents(LISTEN_EVENTS, pThis->m_ListenEvents, FALSE, 1000, FALSE);			

				//等待失败, 退出线程
				if (WSA_WAIT_FAILED == nEvents)
				{
					THROW_LINE;
				}
				else if (WSA_WAIT_TIMEOUT == nEvents)
				{
					if (FALSE == InterlockedExchangeAdd(&(pThis->m_bThreadRun), 0))
					{
						THROW_LINE;
					}
				}
				else
				{
					nEvents = nEvents - WAIT_OBJECT_0;
					if (0 == nEvents)
					{
						nAccept = MAX_ACCEPT;
					}
					else if (1 == nEvents)
					{
						nAccept = 1;
					}

					//最多只能投递200个accept操作
					if (InterlockedExchangeAdd(&(pThis->m_nAcceptCount), 0) > 200)
					{
						nAccept = 0;
					}

					for (int nIndex = 0; nIndex < nAccept; nIndex++)
					{
						SOCKET hClient = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
						if (INVALID_SOCKET == hClient)
						{
							continue;
						}

						ULONG ul = 1;
						ioctlsocket(hClient, FIONBIO, &ul);

						ACCEPT_CONTEXT_EX* pAccContext = new ACCEPT_CONTEXT_EX();
						if (pAccContext && pAccContext->m_pBuf)
						{
							pAccContext->m_hSock = pThis->m_hSock;
							pAccContext->m_hRemoteSock = hClient;
							pAccContext->m_nOperation = OP_ACCEPT;

							nRet = s_pfAcceptEx(pThis->m_hSock, hClient, pAccContext->m_pBuf, 0
								, sizeof(sockaddr_in) +16, sizeof(sockaddr_in) +16, &dwBytes, &(pAccContext->m_ol));

							if (FALSE == nRet && ERROR_IO_PENDING != WSAGetLastError())
							{
								_TRACE("%s -- %ld 投递 accept 操作失败", __FILE__, __LINE__);
								closesocket(hClient);
								delete pAccContext;
								pAccContext = NULL;
							}	
							else
							{
								InterlockedExchangeAdd(&(pThis->m_nAcceptCount), 1);
								_TRACE("%s -- %ld 已投递了%ld 个accept操作", __FILE__, __LINE__, pThis->m_nAcceptCount);
							}
						}
						else
						{
							delete pAccContext;
						}						
					}
				}
			}
		}
		catch ( const long &lErrLine)
		{
			_TRACE("Exp : %s -- %ld", __FILE__, lErrLine);
		}
		return 0;
	}

	UINT WINAPI TcpSerEx::AideThread(LPVOID lpParam)
	{
		TcpSerEx *pThis = (TcpSerEx *)lpParam;
		try
		{
			const int SOCK_CHECKS = 10000;
			const int TIME_OUT = 10 * 60;				//超时时长
			//int nSockTime = 0;
			DWORD nPro = 0;
			int nTimeLen = 0;	
			bool bQueEmpty = true;
			vector<SOCKET>::iterator sock_itre = pThis->m_SocketQue.begin();
			DWORD nNewTime = 0;

			while (TRUE)
			{
				for (int index = 0; index < SOCK_CHECKS; index++)
				{
					nPro = 0;
					//nSockTime = 0x0000ffff;
					// 检查socket队列
					EnterCriticalSection(&(pThis->m_SockQueLock));

					bQueEmpty = pThis->m_SocketQue.empty();

					if (pThis->m_SocketQue.end() != sock_itre)
					{
						nTimeLen = sizeof(nPro);
						getsockopt(*sock_itre, SOL_SOCKET, SO_GROUP_PRIORITY, (char *)&nPro, &nTimeLen);
						//if (_SOCK_RECV != nPro)
						//{
						//	nTimeLen = sizeof(nSockTime);
						//	getsockopt(*sock_itre, SOL_SOCKET, SO_CONNECT_TIME, (char *)&nSockTime, &nTimeLen);

						nNewTime = (DWORD)time(NULL);
						if (nNewTime - nPro > TIME_OUT)
						{
							closesocket(*sock_itre);

							pThis->m_pCloseFun(pThis->m_pCloseParam, *sock_itre, OP_READ);
							pThis->m_SocketQue.erase(sock_itre);

							_TRACE("%s -- %ld SOCKET = 0x%x 出现错误, WAS_ERR = 0x%x, QUE_LEN = %u, TIME = %ld", __FILE__, __LINE__, *sock_itre, WSAGetLastError(), pThis->m_SocketQue.size(), nNewTime - nPro );
						}
						else
						{
							sock_itre++;
						}
						/*						}
						else
						{
						sock_itre ++;						
						}	*/		
					}
					else
					{
						sock_itre = pThis->m_SocketQue.begin();
						LeaveCriticalSection(&(pThis->m_SockQueLock));
						break;
					}

					LeaveCriticalSection(&(pThis->m_SockQueLock));
				}

				if ((FALSE == InterlockedExchangeAdd(&(pThis->m_bThreadRun), 0)) && (true == bQueEmpty))
				{
					THROW_LINE;
				}

				Sleep(100);
			}
		}
		catch (const long &lErrLine)
		{
			_TRACE("Exp : %s -- %ld ", __FILE__, lErrLine);
		}
		return 0;
	}

	void ReleaseTcpSerEx()
	{
		TcpSerEx::ReleaseReource();
	}
}