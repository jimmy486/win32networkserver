#include <process.h>
#include <assert.h>

#include "TcpSer.h"
#include "RunLog.h"

#define _XML_NET_

#define _DEBUG_SQ
#ifdef _DEBUG_SQ
#include <atltrace.h>
#endif

namespace HelpMng
{
	//静态成员变量初始化
	vector<TCP_CONTEXT* > TCP_CONTEXT::s_IDLQue;	
	CRITICAL_SECTION TCP_CONTEXT::s_IDLQueLock;	
	HANDLE TCP_CONTEXT::s_hHeap = NULL;	

	vector<ACCEPT_CONTEXT* > ACCEPT_CONTEXT::s_IDLQue;
	CRITICAL_SECTION ACCEPT_CONTEXT::s_IDLQueLock;
	HANDLE ACCEPT_CONTEXT::s_hHeap = NULL;

	HANDLE TCP_RCV_DATA::s_DataHeap = NULL;
	HANDLE TCP_RCV_DATA::s_hHeap = NULL;
	vector<TCP_RCV_DATA* > TCP_RCV_DATA::s_IDLQue;
	CRITICAL_SECTION TCP_RCV_DATA::s_IDLQueLock;

	LPFN_ACCEPTEX TcpServer::s_pfAcceptEx = NULL;
	LPFN_GETACCEPTEXSOCKADDRS TcpServer::s_pfGetAddrs = NULL;

	void* TCP_CONTEXT::operator new(size_t nSize)
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
			vector<TCP_CONTEXT* >::iterator iter = s_IDLQue.begin();

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

	void TCP_CONTEXT::operator delete(void* p)
	{
		if (p)		
		{
			//若空闲队列长度小于MAX_IDL_DATA, 则将其放入空闲队列中; 否则释放

			EnterCriticalSection(&s_IDLQueLock);
			const DWORD QUE_SIZE = (DWORD)(s_IDLQue.size());
			TCP_CONTEXT* const pContext = (TCP_CONTEXT*)p;

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

	void* ACCEPT_CONTEXT:: operator new(size_t nSize)
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
			vector<ACCEPT_CONTEXT* >::iterator iter = s_IDLQue.begin();

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

	void ACCEPT_CONTEXT:: operator delete(void* p)
	{
		if (p)
		{
			//若空闲队列长度小于10000, 则将其放入空闲队列中; 否则释放

			EnterCriticalSection(&s_IDLQueLock);
			const DWORD QUE_SIZE = (DWORD)(s_IDLQue.size());
			const DWORD MAX_IDL = 500;
			ACCEPT_CONTEXT* const pContext = (ACCEPT_CONTEXT*)p;

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


	//TCP_RCV_DATA 相关实现
	TCP_RCV_DATA::TCP_RCV_DATA(SOCKET hSocket, const CHAR* pBuf, INT nLen)
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

	TCP_RCV_DATA::~TCP_RCV_DATA()
	{
		if ((NULL != m_pData ) && (NULL != s_DataHeap))
		{
			HeapFree(s_DataHeap, 0, m_pData);
			m_pData = NULL;
		}
	}

	void* TCP_RCV_DATA::operator new(size_t nSize)
	{
		void* pRcvData = NULL;

		try
		{
			if (NULL == s_hHeap || NULL == s_DataHeap)
			{
				THROW_LINE;
			}

			EnterCriticalSection(&s_IDLQueLock);
			vector<TCP_RCV_DATA* >::iterator iter = s_IDLQue.begin();
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

	void TCP_RCV_DATA::operator delete(void* p)
	{
		if (p)
		{
			EnterCriticalSection(&s_IDLQueLock);
			const DWORD QUE_SIZE = (DWORD)(s_IDLQue.size());
			TCP_RCV_DATA* const pContext = (TCP_RCV_DATA*)p;

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

	void TcpServer::InitReource()
	{
		WSADATA wsData;
		WSAStartup(MAKEWORD(2, 2), &wsData);

		NET_CONTEXT::InitReource();

		//TCP_CONTEXT
		TCP_CONTEXT::s_hHeap = HeapCreate(0, TCP_CONTEXT::S_PAGE_SIZE, TCP_CONTEXT::E_TCP_HEAP_SIZE);
		InitializeCriticalSection(&(TCP_CONTEXT::s_IDLQueLock));
		TCP_CONTEXT::s_IDLQue.reserve(TCP_CONTEXT::MAX_IDL_DATA * sizeof(void*));
		//_TRACE("%s:%ld TCPMNG_CONTEXT::s_hHeap = 0x%x sizeof(TCPMNG_CONTEXT*) = %ld"
		//	, __FILE__, __LINE__, TCPMNG_CONTEXT::s_hHeap, sizeof(TCPMNG_CONTEXT*));

		//CONNECT_CONTEXT
		//CONNECT_CONTEXT::s_hHeap = HeapCreate(0, CONNECT_CONTEXT::S_PAGE_SIZE, CONNECT_CONTEXT::HEAP_SIZE);
		//InitializeCriticalSection(&(CONNECT_CONTEXT::s_IDLQueLock));

		//ACCEPT_CONTEXT
		ACCEPT_CONTEXT::s_hHeap = HeapCreate(0, ACCEPT_CONTEXT::S_PAGE_SIZE, 500 * 1024);
		InitializeCriticalSection(&(ACCEPT_CONTEXT::s_IDLQueLock));
		ACCEPT_CONTEXT::s_IDLQue.reserve(500 * sizeof(void*));
		//_TRACE("%s:%ld nACCEPT_CONTEXT::s_hHeap = 0x%x", __FILE__, __LINE__, ACCEPT_CONTEXT::s_hHeap);

		//TCP_RCV_DATA
		TCP_RCV_DATA::s_hHeap = HeapCreate(0, 0, TCP_RCV_DATA::HEAP_SIZE);
		TCP_RCV_DATA::s_DataHeap = HeapCreate(0, 0, TCP_RCV_DATA::DATA_HEAP_SIZE);
		InitializeCriticalSection(&(TCP_RCV_DATA::s_IDLQueLock));
		TCP_RCV_DATA::s_IDLQue.reserve(TCP_RCV_DATA::MAX_IDL_DATA * sizeof(void*));
	}

	void TcpServer::ReleaseReource()
	{
		//TCP_CONTEXT
		HeapDestroy(TCP_CONTEXT::s_hHeap);
		TCP_CONTEXT::s_hHeap = NULL;

		EnterCriticalSection(&(TCP_CONTEXT::s_IDLQueLock));
		TCP_CONTEXT::s_IDLQue.clear();
		LeaveCriticalSection(&(TCP_CONTEXT::s_IDLQueLock));
		DeleteCriticalSection(&(TCP_CONTEXT::s_IDLQueLock));

		//ACCEPT_CONTEXT
		HeapDestroy(ACCEPT_CONTEXT::s_hHeap);
		ACCEPT_CONTEXT::s_hHeap = NULL;

		EnterCriticalSection(&(ACCEPT_CONTEXT::s_IDLQueLock));
		ACCEPT_CONTEXT::s_IDLQue.clear();
		LeaveCriticalSection(&(ACCEPT_CONTEXT::s_IDLQueLock));
		DeleteCriticalSection(&(ACCEPT_CONTEXT::s_IDLQueLock));

		//TCP_RCV_DATA
		HeapDestroy(TCP_RCV_DATA::s_hHeap);
		TCP_RCV_DATA::s_hHeap = NULL;

		HeapDestroy(TCP_RCV_DATA::s_DataHeap);
		TCP_RCV_DATA::s_DataHeap = NULL;

		EnterCriticalSection(&(TCP_RCV_DATA::s_IDLQueLock));
		TCP_RCV_DATA::s_IDLQue.clear();
		LeaveCriticalSection(&(TCP_RCV_DATA::s_IDLQueLock));

		DeleteCriticalSection(&(TCP_RCV_DATA::s_IDLQueLock));

		NET_CONTEXT::ReleaseReource();

		WSACleanup();
	}

	TcpServer::TcpServer()
		: m_pCloseFun(NULL)
		, m_hSock(INVALID_SOCKET)
		, m_pCloseParam(NULL)
		, m_bThreadRun(TRUE)
		, m_bSerRun(FALSE)
		, m_nAcceptCount(0)
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
		m_hCompletion = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

		//创建监听线程和工作者线程
		//创建工作者线程, 工作线程的数目为CPU * 2 + 2
		SYSTEM_INFO sys_info;
		GetSystemInfo(&sys_info);

		const DWORD MAX_THREAD = sys_info.dwNumberOfProcessors * 2 +2 + 2;
		m_pThreads = new HANDLE[MAX_THREAD];
		assert(m_pThreads);

		//启动监听线程
		m_pThreads[0] = (HANDLE)_beginthreadex(NULL, 0, ListenThread, this, 0, NULL);
		m_pThreads[1] = (HANDLE)_beginthreadex(NULL, 0, AideThread, this, 0, NULL);
 
		for (DWORD nIndex = 2; nIndex < MAX_THREAD; nIndex++)
		{
			m_pThreads[nIndex] = (HANDLE)_beginthreadex(NULL, 0, WorkThread, this, 0, NULL);
		}
	}

	TcpServer::~TcpServer()
	{
		//必须等待所有的后台线程退出后才能退出		
		SYSTEM_INFO sys_info;
		GetSystemInfo(&sys_info);
		const DWORD MAX_THREAD = sys_info.dwNumberOfProcessors * 2 +2 + 2;

		//通知后台线程退出
		InterlockedExchange((long volatile *)&m_bThreadRun, FALSE);

		WaitForMultipleObjects(MAX_THREAD, m_pThreads, TRUE, 30 * 1000);

		for (DWORD nIndex = 0; nIndex < MAX_THREAD; nIndex ++)
		{
			CloseHandle(m_pThreads[nIndex]);
		}

		delete []m_pThreads;
		EnterCriticalSection(&m_SockQueLock);
		m_SocketQue.clear();
		LeaveCriticalSection(&m_SockQueLock);

		EnterCriticalSection(&m_RcvQueLock);
		m_RcvDataQue.clear();
		LeaveCriticalSection(&m_RcvQueLock);

		CloseHandle(m_ListenEvents[0]);
		CloseHandle(m_ListenEvents[1]);
		CloseHandle(m_hCompletion);
		DeleteCriticalSection(&m_RcvQueLock);
		DeleteCriticalSection(&m_SockQueLock);		
	}

	BOOL TcpServer::StartServer( const char *szIp , INT nPort , LPCLOSE_ROUTINE pCloseFun , LPVOID pParam )
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

			m_pCloseFun = pCloseFun;
			m_pCloseParam = pParam;
			m_bSerRun = TRUE;

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
			CreateIoCompletionPort((HANDLE)m_hSock, m_hCompletion, 0, 0);
			WSAEventSelect(m_hSock, m_ListenEvents[0], FD_ACCEPT);

			//先投递50个accept操作			
			for (int nIndex = 0; nIndex < MAX_ACCEPT; )
			{
				SOCKET hClient = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
				if (INVALID_SOCKET == hClient)
				{
					continue;
				}

				ul = 1;
				ioctlsocket(hClient, FIONBIO, &ul);

				ACCEPT_CONTEXT* pAccContext = new ACCEPT_CONTEXT();
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
		}
		catch (const long &lErrLine)
		{
			bSucc = FALSE;
			m_bSerRun = FALSE;
			_TRACE("Exp : %s -- %ld", __FILE__, lErrLine);
		}
		return bSucc;
	}

	UINT WINAPI TcpServer::ListenThread(LPVOID lpParam)
	{
		TcpServer *pThis = (TcpServer *)lpParam;
		try
		{
			int nRet = 0;
			DWORD	nEvents = 0;
			DWORD dwBytes = 0;	
			int nAccept = 0;

			while (TRUE)
			{				
				nEvents = WSAWaitForMultipleEvents(LISTEN_EVENTS, pThis->m_ListenEvents, FALSE, WSA_INFINITE, FALSE);			
			
				//等待失败, 退出线程
				if (WSA_WAIT_FAILED == nEvents)
				{
					THROW_LINE;;
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

					for (int nIndex = 0; nIndex < nAccept; )
					{
						SOCKET hClient = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
						if (INVALID_SOCKET == hClient)
						{
							continue;
						}

						ULONG ul = 1;
						ioctlsocket(hClient, FIONBIO, &ul);

						ACCEPT_CONTEXT* pAccContext = new ACCEPT_CONTEXT();
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
						nIndex++;
					}
				}

				if (FALSE == InterlockedExchangeAdd(&(pThis->m_bThreadRun), 0))
				{
					THROW_LINE;;
				}
			}
		}
		catch ( const long &lErrLine)
		{
			_TRACE("Exp : %s -- %ld", __FILE__, lErrLine);
		}
		return 0;
	}

	void TcpServer::CloseServer()
	{
		//关闭所有的socket
		closesocket(m_hSock);

		EnterCriticalSection(&m_SockQueLock);

		for (vector<SOCKET>::iterator iter_sock = m_SocketQue.begin(); m_SocketQue.end() != iter_sock; iter_sock++)
		{
			closesocket(*iter_sock);
		}

		LeaveCriticalSection(&m_SockQueLock);

		m_bSerRun = FALSE;
	}	

	BOOL TcpServer::SendData(SOCKET hSock, const CHAR* szData, INT nDataLen)
	{
#ifdef _XML_NET_
		//数据长度非法
		if (((DWORD)nDataLen > TCP_CONTEXT::S_PAGE_SIZE) || (NULL == szData))
		{
			return FALSE;
		}
#else
		//数据长度非法
		if ((nDataLen > (int)(TCP_CONTEXT::S_PAGE_SIZE)) || (NULL == szData) || (nDataLen < sizeof(PACKET_HEAD)))
		{
			return FALSE;
		}
#endif	//#ifdef _XML_NET_

		BOOL bResult = TRUE;
		DWORD dwBytes = 0;
		WSABUF SendBuf;
		TCP_CONTEXT *pSendContext = new TCP_CONTEXT();
		if (pSendContext && pSendContext->m_pBuf)
		{
			pSendContext->m_hSock = hSock;
			pSendContext->m_nDataLen = 0;
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

	TCP_RCV_DATA * TcpServer::GetRcvData( DWORD* const pQueLen )
	{
		TCP_RCV_DATA* pRcvData = NULL;

		EnterCriticalSection(&m_RcvQueLock);
		vector<TCP_RCV_DATA*>::iterator iter = m_RcvDataQue.begin();
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

	void TcpServer::PushInRecvQue(TCP_RCV_DATA* const pRcvData)
	{
		EnterCriticalSection(&m_RcvQueLock);
		m_RcvDataQue.push_back(pRcvData);
		LeaveCriticalSection(&m_RcvQueLock);
	}

	UINT WINAPI TcpServer::WorkThread(LPVOID lpParam)
	{
		TcpServer *pThis = (TcpServer *)lpParam;
		DWORD dwTrans = 0, dwKey = 0, dwSockSize = 0;
		LPOVERLAPPED pOl = NULL;
		NET_CONTEXT *pContext = NULL;
		BOOL bRun = TRUE;

		while (TRUE)
		{
			BOOL bOk = GetQueuedCompletionStatus(pThis->m_hCompletion, &dwTrans, &dwKey, (LPOVERLAPPED *)&pOl, WSA_INFINITE);

			pContext = CONTAINING_RECORD(pOl, NET_CONTEXT, m_ol);
			if (pContext)
			{
				switch (pContext->m_nOperation)
				{
				case OP_ACCEPT:
					pThis->AcceptCompletionProc(bOk, dwTrans, pOl);
					break;
				case OP_READ:
					pThis->RecvCompletionProc(bOk, dwTrans, pOl);
					break;
				case OP_WRITE:
					pThis->SendCompletionProc(bOk, dwTrans, pOl);
					break;
				}
			}

			EnterCriticalSection(&(pThis->m_SockQueLock));

			dwSockSize = (DWORD)(pThis->m_SocketQue.size());
			if (FALSE == InterlockedExchangeAdd(&(pThis->m_bThreadRun), 0) && 0 == dwSockSize 
				&& 0 == InterlockedExchangeAdd(&(pThis->m_nAcceptCount), 0))
			{
				bRun = FALSE;
			}

			LeaveCriticalSection(&(pThis->m_SockQueLock));

			if (FALSE == bRun)
			{
				//ATLTRACE("\r\n 工作线程 0x%x 已退出", GetCurrentThread());
				break;
			}
		}

		return 0;
	}

	void TcpServer::AcceptCompletionProc(BOOL bSuccess, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		ACCEPT_CONTEXT *pContext = CONTAINING_RECORD(lpOverlapped, ACCEPT_CONTEXT, m_ol);
		INT nZero = 0;
		int nPro = _SOCK_NO_RECV;
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
			setsockopt(pContext->m_hRemoteSock, SOL_SOCKET, SO_GROUP_PRIORITY, (char *)&nPro, sizeof(nPro));

			s_pfGetAddrs(pContext->m_pBuf, 0, sizeof(sockaddr_in) +16, sizeof(sockaddr_in) +16
				, (LPSOCKADDR*)&pLocalAddr, &nLocalLen, (LPSOCKADDR*)&pClientAddr, &nClientLen);

			//为新到来的连接投递读操作
			TCP_CONTEXT *pRcvContext = new TCP_CONTEXT;
			if (pRcvContext && pRcvContext->m_pBuf)
			{
				pRcvContext->m_hSock = pContext->m_hRemoteSock;
				pRcvContext->m_nOperation = OP_READ;
				CreateIoCompletionPort((HANDLE)(pRcvContext->m_hSock), m_hCompletion, NULL, 0);

				RcvBuf.buf = pRcvContext->m_pBuf;
				RcvBuf.len = TCP_CONTEXT::S_PAGE_SIZE;

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

	void TcpServer::RecvCompletionProc(BOOL bSuccess, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		TCP_CONTEXT* pRcvContext = CONTAINING_RECORD(lpOverlapped, TCP_CONTEXT, m_ol);
		DWORD dwFlag = 0;
		DWORD dwBytes = 0;
		WSABUF RcvBuf;
		int nErrCode = 0;
		int nPro = _SOCK_RECV;

		try
		{
			if ((FALSE == bSuccess || 0 == dwNumberOfBytesTransfered) && (WSA_IO_PENDING != WSAGetLastError()))
			{
				closesocket(pRcvContext->m_hSock);
				THROW_LINE;;
			}

			setsockopt(pRcvContext->m_hSock, SOL_SOCKET, SO_GROUP_PRIORITY, (char *)&nPro, sizeof(nPro));

#ifndef _XML_NET_		//处理二进制数据流
			//非法客户端发来的数据包, 关闭该客户端; 若第一次收到数据包并且数据包的长度消息包头长度则说明数据包错误
			if (0 == pRcvContext->m_nDataLen && dwNumberOfBytesTransfered < sizeof(PACKET_HEAD))
			{
				closesocket(pRcvContext->m_hSock);
				THROW_LINE;;
			}
#endif	//#ifndef _XML_NET_

#ifdef _XML_NET_	//处理XML流
			TCP_RCV_DATA* pRcvData = new TCP_RCV_DATA(
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
			RcvBuf.len = TCP_CONTEXT::S_PAGE_SIZE;

#else				//处理二进制数据流

			//解析包头信息中的应接受的数据包的长度
			pRcvContext->m_nDataLen += dwNumberOfBytesTransfered;
			//WORD nAllLen = *((WORD*)(pRcvContext->m_pBuf));		//此长度为数据的长度+sizeof(WORD)
			PACKET_HEAD* pHeadInfo = (PACKET_HEAD*)(pRcvContext->m_pBuf);
			//WSABUF RcvBuf;
			//数据长度合法才进行处理
			if ((pHeadInfo->nCurrentLen <= TCP_CONTEXT::S_PAGE_SIZE)
				//&& (0 == dwErrorCode)
				&& ((WORD)(pRcvContext->m_nDataLen) <= pHeadInfo->nCurrentLen + sizeof(PACKET_HEAD)))
			{
				//所有数据已接收完,将其放入接收队列中
				if ((WORD)(pRcvContext->m_nDataLen) == pHeadInfo->nCurrentLen + sizeof(PACKET_HEAD))
				{
					TCP_RCV_DATA* pRcvData = new TCP_RCV_DATA(
						pRcvContext->m_hSock
						, pRcvContext->m_pBuf
						, pRcvContext->m_nDataLen
						);
					
					if (pRcvData && pRcvData->m_pData)
					{
						EnterCriticalSection(&m_RcvQueLock);
						m_RcvDataQue.push_back(pRcvData);
						LeaveCriticalSection(&m_RcvQueLock);
					}

					pRcvContext->m_nDataLen = 0;
					RcvBuf.buf = pRcvContext->m_pBuf;
					RcvBuf.len = TCP_CONTEXT::S_PAGE_SIZE;
				}
				//数据没有接收完, 需要接收剩余的数据
				else
				{
					RcvBuf.buf = pRcvContext->m_pBuf +pRcvContext->m_nDataLen;
					RcvBuf.len = pHeadInfo->nCurrentLen - pRcvContext->m_nDataLen +sizeof(PACKET_HEAD);
				}
			}
			//数据非法, 则不处理数据直接进行下一次读投递
			else
			{
				pRcvContext->m_nDataLen = 0;
				RcvBuf.buf = pRcvContext->m_pBuf;
				RcvBuf.len = TCP_CONTEXT::S_PAGE_SIZE;
			}
#endif	//#ifdef _XML_NET_

			//继续投递读操作
			//DWORD dwBytes = 0;
			nErrCode = WSARecv(pRcvContext->m_hSock, &RcvBuf, 1, &dwBytes, &dwFlag, &(pRcvContext->m_ol), NULL);
			//Recv操作错误
			if (SOCKET_ERROR == nErrCode && WSA_IO_PENDING != WSAGetLastError())
			{
				closesocket(pRcvContext->m_hSock);
				THROW_LINE;;
			}
		}
		catch (const long &lErrLine)
		{
			_TRACE("Exp : %s -- %ld SOCKET = 0x%x ERR_CODE = 0x%x", __FILE__, lErrLine, pRcvContext->m_hSock, WSAGetLastError());
			delete pRcvContext;			
		}
	}

	void TcpServer::SendCompletionProc(BOOL bSuccess, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		TCP_CONTEXT* pSendContext = CONTAINING_RECORD(lpOverlapped, TCP_CONTEXT, m_ol);
		delete pSendContext;
		pSendContext = NULL;
	}

	UINT WINAPI TcpServer::AideThread(LPVOID lpParam)
	{
		TcpServer *pThis = (TcpServer *)lpParam;
		try
		{
			const int SOCK_CHECKS = 10000;
			int nSockTime = 0;
			int nPro = 0;
			int nTimeLen = 0;			
			vector<SOCKET>::iterator sock_itre = pThis->m_SocketQue.begin();

			while (TRUE)
			{
				for (int index = 0; index < SOCK_CHECKS; index++)
				{
					nPro = 0;
					nSockTime = 0x0000ffff;
					// 检查socket队列
					EnterCriticalSection(&(pThis->m_SockQueLock));

					if (pThis->m_SocketQue.end() != sock_itre)
					{
						nTimeLen = sizeof(nPro);
						getsockopt(*sock_itre, SOL_SOCKET, SO_GROUP_PRIORITY, (char *)&nPro, &nTimeLen);
						if (_SOCK_RECV != nPro)
						{
							nTimeLen = sizeof(nSockTime);
							getsockopt(*sock_itre, SOL_SOCKET, SO_CONNECT_TIME, (char *)&nSockTime, &nTimeLen);

							if (nSockTime > 120)
							{
								closesocket(*sock_itre);

								pThis->m_pCloseFun(pThis->m_pCloseParam, *sock_itre, OP_READ);
								pThis->m_SocketQue.erase(sock_itre);

								_TRACE("%s -- %ld SOCKET = 0x%x 出现错误, WAS_ERR = 0x%x, nPro = 0x%x, TIME = %ld", __FILE__, __LINE__, *sock_itre, WSAGetLastError(), nPro, nSockTime);
							}
							else
							{
								sock_itre++;
							}
						}
						else
						{
							sock_itre ++;						
						}			
					}
					else
					{
						sock_itre = pThis->m_SocketQue.begin();
						LeaveCriticalSection(&(pThis->m_SockQueLock));
						break;
					}

					LeaveCriticalSection(&(pThis->m_SockQueLock));
				}

				if (FALSE == InterlockedExchangeAdd(&(pThis->m_bThreadRun), 0))
				{
					THROW_LINE;;
				}

				Sleep(100);
			}
		}
		catch (const long &lErrLine)
		{
			_TRACE("Exp : %s -- %ld", __FILE__, lErrLine);
		}
			return 0;
	}
}