#include "TcpMng.h"
#include <process.h>
#include <assert.h>
#include <time.h>

#ifdef WIN32

#ifdef _DEBUG_SQ
#include <atltrace.h>

#define _TRACE ATLTRACE
#else
#include "runlog.h"
#endif		//#ifdef _DEBUG

#endif		//#ifdef WIN32

#ifndef _XML_NET_
#define _XML_NET_
#endif
/**
* 若要处理XML流请打开宏开关 _XML_NET_ 
* 否则网络模块将按二进制数据进行处理, 
* 其二进制数据头参见 HeadFile.h中的 
* PACKET_HEAD 定义
*/

namespace HelpMng
{
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

	vector<SOCKET_CONTEXT*> SOCKET_CONTEXT::s_IDLQue;
	CRITICAL_SECTION SOCKET_CONTEXT::s_IDLQueLock;
	HANDLE SOCKET_CONTEXT::s_hHeap = NULL;			

	LPFN_ACCEPTEX CTcpMng::s_pfAcceptEx = NULL;
	LPFN_CONNECTEX CTcpMng::s_pfConnectEx = NULL;
	LPFN_GETACCEPTEXSOCKADDRS CTcpMng::s_pfGetAddrs = NULL;

	//class TCP_CONTEXT的相关实现

	void* TCP_CONTEXT::operator new(size_t nSize)
	{
		void* pContext = NULL;

		try
		{
			if (NULL == s_hHeap)
			{
				throw ((long)(__LINE__));
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
				throw ((long)(__LINE__));
			}
		}
		catch (const long& iErrCode)
		{
			pContext = NULL;
			_TRACE("\r\nExcept : %s--%ld", __FILE__, iErrCode);
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

	BOOL TCP_CONTEXT::IsAddressValid(LPCVOID lpMem)
	{
		BOOL bResult = HeapValidate(s_hHeap, 0, lpMem);

		if (NULL == lpMem)
		{
			bResult = FALSE;
		}	

		return bResult;
	}

	//class ACCEPT_CONTEXT的相关实现
	void* ACCEPT_CONTEXT:: operator new(size_t nSize)
	{
		void* pContext = NULL;

		try
		{	
			if (NULL == s_hHeap)
			{
				throw ((long)(__LINE__));
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
				throw ((long)(__LINE__));
			}
		}
		catch (const long& iErrCode)
		{
			pContext = NULL;
			_TRACE("\r\nExcept : %s--%ld", __FILE__, iErrCode);
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

	BOOL ACCEPT_CONTEXT::IsAddressValid(LPCVOID lpMem)
	{
		BOOL bResult = HeapValidate(s_hHeap, 0, lpMem);

		if (NULL == lpMem)
		{
			bResult = FALSE;
		}	

		return bResult;
	}

	//TCP_RCV_DATA 相关实现
	TCP_RCV_DATA::TCP_RCV_DATA(SOCKET_CONTEXT *pSocket, const CHAR* pBuf, INT nLen)
		: m_pSocket(pSocket)
		, m_nLen(nLen)
	{
		try
		{
			assert(pBuf);
			if (NULL == s_DataHeap)
			{
				throw ((long)(__LINE__));
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
			_TRACE("\r\nExcept : %s--%ld", __FILE__, iErrCode);
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
				throw ((long)(__LINE__));
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
				throw ((long)(__LINE__));
			}
		}
		catch (const long& iErrCode)
		{
			pRcvData = NULL;
			_TRACE("\r\nExcept : %s--%ld", __FILE__, iErrCode);
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

	BOOL TCP_RCV_DATA::IsAddressValid(LPCVOID pMem)
	{
		BOOL bResult = HeapValidate(s_hHeap, 0, pMem);

		if (NULL == pMem)
		{
			bResult = FALSE;
		}

		return bResult;
	}

	BOOL TCP_RCV_DATA::IsDataValid()
	{
		BOOL bResult = HeapValidate(s_DataHeap, 0, m_pData);

		if (NULL == m_pData)
		{
			bResult = FALSE;
		}

		return bResult;
	}

	//SOCKET_CONTEXT的相关实现
	SOCKET_CONTEXT::SOCKET_CONTEXT(const IP_ADDR& peer_addr, const SOCKET &sock)
		: m_Addr(peer_addr)
		, m_hSocket(sock)
		, m_nInteractiveTime((DWORD)time(NULL))
	{
	}

	void* SOCKET_CONTEXT::operator new(size_t nSize)
	{
		void* pContext = NULL;

		try
		{
			if (NULL == s_hHeap)
			{
				throw ((long)(__LINE__));
			}

			EnterCriticalSection(&s_IDLQueLock);
			//bool bQueEmpty = true;
			vector<SOCKET_CONTEXT*>::iterator iter = s_IDLQue.begin();
			if (s_IDLQue.end() != iter)
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
				throw ((long)(__LINE__));
			}
		}
		catch (const long& iErrCode)
		{
			pContext = NULL;
			_TRACE("\r\nExcept : %s--%ld", __FILE__, iErrCode);
		}

		return pContext;
	}

	void SOCKET_CONTEXT::operator delete(void* p)
	{
		if (p)
		{
			EnterCriticalSection(&s_IDLQueLock);
			const DWORD QUE_SIZE = (DWORD)(s_IDLQue.size());
			SOCKET_CONTEXT* const pContext = (SOCKET_CONTEXT*)p;

			if (QUE_SIZE <= MAX_IDL_SIZE)
			{
				s_IDLQue.push_back(pContext);
			}
			else
			{	
				HeapFree(s_hHeap, 0, p);
			}	
			LeaveCriticalSection(&s_IDLQueLock);
		}

		return;
	}

	//CTcpMng的相关实现
	CTcpMng::CTcpMng(void)
		: m_pCloseFun(NULL)
		, m_pCloseParam(NULL)
		, m_hSock(INVALID_SOCKET)
		, m_bThreadRun(TRUE)
	{
		InitializeCriticalSection(&m_RcvQueLock);
		//InitializeCriticalSection(&m_SendDataMapLock);
		InitializeCriticalSection(&m_SockMapLock);

		//为相关队列预留空间
		m_RcvDataQue.reserve(5000 * sizeof(void*));
		m_AcceptQue.reserve(200 * sizeof(void*));
		m_SockIter = m_SocketMap.begin();
	}

	CTcpMng::~CTcpMng(void)
	{
		CloseServer();

		//确保IO线程删除所有的SOCKET上下文
		DWORD SOCK_SIZE = 0;
		while (true)
		{
			EnterCriticalSection(&m_SockMapLock);

			SOCK_SIZE = (DWORD)(m_SocketMap.size());

			LeaveCriticalSection(&m_SockMapLock);

			if (SOCK_SIZE)
			{
				Sleep(100);
			}
			else
			{
				break;
			}
		}

		DeleteCriticalSection(&m_RcvQueLock);
		//DeleteCriticalSection(&m_SendDataMapLock);
		DeleteCriticalSection(&m_SockMapLock);
	}

	BOOL CTcpMng::StartServer(
		INT nPort
		, LPCLOSE_ROUTINE pCloseFun		//某个socket关闭的回调函数
		, LPVOID pParam					//回调函数的pParam参数
		)
	{
		BOOL bResult = TRUE;
		INT nRet = 0;

		m_pCloseFun = pCloseFun;
		m_pCloseParam = pParam;

		//先关闭上次启动的服务
		CloseServer();

		try
		{
			//创建监听套接字
			m_hSock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
			if (INVALID_SOCKET == m_hSock)
			{
				throw ((long)(__LINE__));
			}
			//加载AcceptEx函数
			DWORD dwBytes;
			GUID guidProc = WSAID_ACCEPTEX;
			if (NULL == s_pfAcceptEx)
			{
				nRet = WSAIoctl(m_hSock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidProc, sizeof(guidProc)
					, &s_pfAcceptEx, sizeof(s_pfAcceptEx), &dwBytes, NULL, NULL);
			}
			if (NULL == s_pfAcceptEx || SOCKET_ERROR == nRet)
			{
				throw ((long)(__LINE__));
			}

			//加载GetAcceptExSockaddrs函数
			GUID guidGetAddr = WSAID_GETACCEPTEXSOCKADDRS;
			if (NULL == s_pfGetAddrs)
			{
				nRet = WSAIoctl(m_hSock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidGetAddr, sizeof(guidGetAddr)
					, &s_pfGetAddrs, sizeof(s_pfGetAddrs), &dwBytes, NULL, NULL);
			}
			if (NULL == s_pfGetAddrs)
			{
				throw ((long)(__LINE__));
			}

			//启动监听服务，并投递ACCEPT_NUM个accept操作
			ULONG ul = 1;
			ioctlsocket(m_hSock, FIONBIO, &ul);

			//设置为地址重用，优点在于服务器关闭后可以立即启用
			int nOpt = 1;
			setsockopt(m_hSock, SOL_SOCKET, SO_REUSEADDR, (char*)&nOpt, sizeof(nOpt));

			sockaddr_in LocalAddr;
			LocalAddr.sin_family = AF_INET;
			LocalAddr.sin_port = htons(nPort);
			LocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);
			nRet = bind(m_hSock, (sockaddr*)&LocalAddr, sizeof(LocalAddr));
			if (SOCKET_ERROR == nRet)
			{
				throw ((long)(__LINE__));
			}

			nRet = listen(m_hSock, 200);
			if (SOCKET_ERROR == nRet)
			{
				throw ((long)(__LINE__));
			}
			//将m_ListenSock邦定完成端口
			if (FALSE == BindIoCompletionCallback((HANDLE)m_hSock, IOCompletionRoutine, 0))
			{
				throw ((long)(__LINE__));
			}

			//投递ACCEPT_NUM个Accept操作
#define ACCEPT_NUM 5
			for (INT index = 0; index < ACCEPT_NUM; )
			{
				//创建客户端socket
				SOCKET clientSock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
				if (INVALID_SOCKET == clientSock)
				{
					continue;
				}

				ULONG ul = 1;
				ioctlsocket(clientSock, FIONBIO, &ul);

				//创建一个ACCEPT_CONTEXT
				ACCEPT_CONTEXT* pAccContext = new ACCEPT_CONTEXT(this);
				if (NULL == pAccContext)
				{
					closesocket(clientSock);
					throw ((long)(__LINE__));
				}
				pAccContext->m_hSock = m_hSock;
				pAccContext->m_hRemoteSock = clientSock;
				pAccContext->m_nOperation = OP_ACCEPT;

				nRet = s_pfAcceptEx(m_hSock, clientSock, pAccContext->m_pBuf, 0
					, sizeof(sockaddr_in) +16, sizeof(sockaddr_in) +16, &dwBytes, &(pAccContext->m_ol));
				//accept发生错误, 关闭服务器
				if (FALSE == nRet && ERROR_IO_PENDING != WSAGetLastError())
				{
					closesocket(clientSock);
					delete pAccContext;
					pAccContext = NULL;
					throw ((long)(__LINE__));
				}
				//将ACCEPT_CONTEXT压入到ACCEPT_CONTEXT队列中
				m_AcceptQue.push_back(pAccContext);
				index++;
			}

			//启动后台线程
			m_bThreadRun = TRUE;
			_beginthreadex(NULL, 0, ThreadProc, this, 0, NULL);
		}
		catch (const long& iErrCode)
		{
			closesocket(m_hSock);
			_TRACE("\r\nExcept : %s--%ld; LAST_ERROR = %ld", __FILE__, iErrCode, WSAGetLastError());

			bResult = FALSE;
		}
		return bResult;
	}

	void CTcpMng::CloseServer()
	{
		closesocket(m_hSock);
		m_hSock = INVALID_SOCKET;

		//m_bThreadRun = FALSE;
		InterlockedExchange((long volatile *)&m_bThreadRun, FALSE);
		
		EnterCriticalSection(&m_SockMapLock);
		for (CSocketMap::iterator sock_iter = m_SocketMap.begin(); m_SocketMap.end() != sock_iter; sock_iter++)
		{
			closesocket(sock_iter->first);
			//SOCKET_CONTEXT* pContext = sock_iter->second;
			//delete pContext;
			//pContext = NULL;
		}

		//m_SocketMap.clear();
		LeaveCriticalSection(&m_SockMapLock);


		//清理接收数据的缓冲区队列
		TCP_RCV_DATA* pRcvData = NULL;

		EnterCriticalSection(&m_RcvQueLock);
		const DWORD RCV_QUE_NUM = (DWORD)(m_RcvDataQue.size());
		for (DWORD i = 0; i < RCV_QUE_NUM; i++)
		{
			pRcvData = m_RcvDataQue[i];

			delete pRcvData;
			pRcvData = NULL;
		}

		m_RcvDataQue.clear();
		LeaveCriticalSection(&m_RcvQueLock);

		for (vector<ACCEPT_CONTEXT*>::iterator iterAcc = m_AcceptQue.begin(); m_AcceptQue.end() != iterAcc; iterAcc++)
		{
			ACCEPT_CONTEXT* pContext = *iterAcc;
			delete pContext;
			pContext = NULL;
		}
		m_AcceptQue.clear();

		//EnterCriticalSection(&m_SendDataMapLock);
		//for (CSendMap::iterator iterMap = m_SendDataMap.begin(); m_SendDataMap.end() != iterMap; iterMap++)
		//{
		//	SEND_CONTEXT* pContext = iterMap->second;
		//	delete pContext;
		//	pContext = NULL;
		//}
		//m_SendDataMap.clear();
		//LeaveCriticalSection(&m_SendDataMapLock);

		//EnterCriticalSection(&m_SockMapLock);
		//for (CSocketMap::iterator sock_iter = m_SocketMap.begin(); m_SocketMap.end() != sock_iter; sock_iter++)
		//{
		//	SOCKET_CONTEXT* pContext = sock_iter->second;
		//	delete pContext;
		//	pContext = NULL;
		//}

		//m_SocketMap.clear();
		//LeaveCriticalSection(&m_SockMapLock);

		//m_InvalidQue.clear();

	}

	void CALLBACK CTcpMng::IOCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{

		TCP_CONTEXT* pContext = CONTAINING_RECORD(lpOverlapped, TCP_CONTEXT, m_ol);
		switch (pContext->m_nOperation)
		{
		case OP_ACCEPT:		//有客户端连接到服务器
			AcceptCompletionProc(dwErrorCode, dwNumberOfBytesTransfered, lpOverlapped);
			break;

		case OP_READ:		//读数据操作已完成
			RecvCompletionProc(dwErrorCode, dwNumberOfBytesTransfered, lpOverlapped);
			break;

		case OP_WRITE:		//写数据操作已完成
			SendCompletionProc(dwErrorCode, dwNumberOfBytesTransfered, lpOverlapped);
			break;

		case OP_CONNECT:	//已成功连接到服务器
			//ConnectCompletionProc(dwErrorCode, dwNumberOfBytesTransfered, lpOverlapped);
			break;

		default:
			break;
		}
	}
	
	//当AcceptEx操作完成所需要进行的处理
	void CTcpMng::AcceptCompletionProc(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		//try
		//{
		INT iErrCode = 0;
		DWORD nBytes = 0;
		INT nZero = 0;
		ULONG ul = 1;
		IP_ADDR* pClientAddr = NULL;
		IP_ADDR* pLocalAddr = NULL;
		INT nClientLen = 0;
		INT nLocalLen = 0;
		SOCKET clientSock = INVALID_SOCKET; 
		DWORD nFlag = 0;					
		WSABUF RcvBuf;

		ACCEPT_CONTEXT* pContext = CONTAINING_RECORD(lpOverlapped, ACCEPT_CONTEXT, m_ol);

		//有错误发生, 关闭客户端socket
		if (0 != dwErrorCode)
		{
			_TRACE("\r\n%s : %ld LAST_ERROR = %ld", __FILE__, __LINE__, WSAGetLastError());
			closesocket(pContext->m_hRemoteSock);
		}
		else
		{
			//为到来的连接申请TCP_CONTEXT，投递WSARecv操作, 并将其放入到m_ContextMap中
			//关闭系统缓存，使用自己的缓存以防止数据的复制操作

			setsockopt(pContext->m_hRemoteSock, SOL_SOCKET, SO_SNDBUF, (char*)&nZero, sizeof(nZero));
			//nZero = 65535;
			setsockopt(pContext->m_hRemoteSock, SOL_SOCKET, SO_RCVBUF, (CHAR*)&nZero, sizeof(nZero));
			setsockopt(pContext->m_hRemoteSock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&(pContext->m_hSock), sizeof(pContext->m_hSock));

			s_pfGetAddrs(pContext->m_pBuf, 0, sizeof(sockaddr_in) +16, sizeof(sockaddr_in) +16
				, (LPSOCKADDR*)&pLocalAddr, &nLocalLen, (LPSOCKADDR*)&pClientAddr, &nClientLen);

			//申请一个新的接收缓存，并为其投递读操作
			TCP_CONTEXT* pReadContext = new TCP_CONTEXT(pContext->m_pMng);
			//_TRACE("\r\n%s : %ld pReadContext = 0x%x", __FILE__, __LINE__, pReadContext);
			//系统可能已经没有足够的内存可以使用
			if (NULL == pReadContext)
			{
				closesocket(pContext->m_hRemoteSock);
				_TRACE("\r\n%s : %ld LAST_ERROR = %ld", __FILE__, __LINE__, WSAGetLastError());
			}
			else
			{
				//投递读操作
				//InterlockedExchange(&(pTcpContext->m_pRcvContext->m_nPostCount), 1);
				pReadContext->m_hSock = pContext->m_hRemoteSock;
				pReadContext->m_nOperation = OP_READ;

				//绑定到完成端口失败
				//pTcpContext->m_pSendContext->m_hSock = pContext->m_hRemoteSock;
				if (FALSE == BindIoCompletionCallback((HANDLE)(pReadContext->m_hSock), IOCompletionRoutine, 0))
				{
					closesocket(pContext->m_hRemoteSock);
					delete pReadContext;
					pReadContext = NULL;
					_TRACE("\r\n%s : %ld LAST_ERROR = %ld", __FILE__, __LINE__, GetLastError());
				}
				//绑定成功
				else
				{					
					RcvBuf.buf = pReadContext->m_pBuf;
					RcvBuf.len = TCP_CONTEXT::S_PAGE_SIZE;
					iErrCode = WSARecv(pReadContext->m_hSock, &RcvBuf, 1, &nBytes, &nFlag, &(pReadContext->m_ol), NULL);

					//投递失败
					if (SOCKET_ERROR == iErrCode && WSA_IO_PENDING != WSAGetLastError())
					{
						closesocket(pReadContext->m_hSock);
						delete pReadContext;
						pReadContext = NULL;
						_TRACE("\r\n%s : %ld LAST_ERROR = %ld", __FILE__, __LINE__, WSAGetLastError());
					}
					//投递成功将其放入socket队列中
					else if (pClientAddr)
					{
						SOCKET_CONTEXT* pScokContext = new SOCKET_CONTEXT(*pClientAddr, pContext->m_hRemoteSock);
						pContext->m_pMng->PushInSocketMap(pContext->m_hRemoteSock, pScokContext);
					}
				}
			}
		}

		//投递新的Accept操作		
		clientSock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

		ioctlsocket(clientSock, FIONBIO, &ul);

		pContext->m_hRemoteSock = clientSock;
		iErrCode = s_pfAcceptEx(pContext->m_hSock, clientSock, pContext->m_pBuf, 0
			, sizeof(sockaddr_in) +16, sizeof(sockaddr_in) +16, &nBytes, &(pContext->m_ol));
		//accept发生错误
		if (FALSE == iErrCode && ERROR_IO_PENDING != WSAGetLastError())
		{
			_TRACE("\r\n%s : %ld LAST_ERROR = %ld", __FILE__, __LINE__, WSAGetLastError());
			closesocket(clientSock);
			//closesocket(pContext->m_hSock);
		}	
	}

	void CTcpMng::PushInSocketMap(const SOCKET& s, SOCKET_CONTEXT *pSocket)
	{
		EnterCriticalSection(&m_SockMapLock);

		m_SocketMap[s] = pSocket;			
		_TRACE("%s -- %ld SOCKET = %ld 连接到服务器 服务器当前共有 %ld 个连接", __FILE__, __LINE__, s, m_SocketMap.size());

		LeaveCriticalSection(&m_SockMapLock);
	}

	//当WSARecv操作完成所需要进行的处理
	void CTcpMng::RecvCompletionProc(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		TCP_CONTEXT* pRcvContext = CONTAINING_RECORD(lpOverlapped, TCP_CONTEXT, m_ol);
		DWORD dwFlag = 0;
		DWORD dwBytes = 0;
		WSABUF RcvBuf;

		try
		{
			//有错误产生, 关闭socket，并释放CONTEXT
			if (0 != dwErrorCode && ERROR_IO_PENDING != WSAGetLastError())
			{
				throw ((long)(__LINE__));
			}

#ifndef _XML_NET_		//处理二进制数据流
			//非法客户端发来的数据包, 关闭该客户端; 若第一次收到数据包并且数据包的长度消息包头长度则说明数据包错误
			if (0 == pRcvContext->m_nDataLen && dwNumberOfBytesTransfered < sizeof(PACKET_HEAD))
			{
				throw ((long)(__LINE__));
			}
#endif	//#ifndef _XML_NET_

			//修改交互时间
			pRcvContext->m_pMng->ModifyInteractiveTime(pRcvContext->m_hSock);


#ifdef _XML_NET_	//处理XML流
			TCP_RCV_DATA* pRcvData = new TCP_RCV_DATA(
				pRcvContext->m_pMng->GetSocketContext(pRcvContext->m_hSock)
				, pRcvContext->m_pBuf
				, dwNumberOfBytesTransfered
				);

			//表明没有足够的内存可使用
			if (pRcvData && pRcvData->m_pData)
			{					
				pRcvContext->m_pMng->PushInRecvQue(pRcvData);
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
						pRcvContext->m_pMng->GetSocketContext(pRcvContext->m_hSock)
						, pRcvContext->m_pBuf
						, pRcvContext->m_nDataLen
						);
					//表明没有足够的内存可使用
					if (pRcvData && pRcvData->m_pData)
					{
						pRcvContext->m_pMng->PushInRecvQue(pRcvData);
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
			INT iErrCode = WSARecv(pRcvContext->m_hSock, &RcvBuf, 1, &dwBytes, &dwFlag, &(pRcvContext->m_ol), NULL);
			//Recv操作错误
			if (SOCKET_ERROR == iErrCode && WSA_IO_PENDING != WSAGetLastError())
			{
				throw ((long)(__LINE__));
			}
		}
		catch (const long& iErrCode)
		{				
			//closesocket(pRcvContext->m_hSock);

			//通知上层网络接口关闭
			//pRcvContext->m_pMng->m_pCloseFun(
			//	pRcvContext->m_pMng->m_pCloseParam
			//	, pRcvContext->m_pMng->GetSocketContext(pRcvContext->m_hSock)
			//	);

			//删除socket上下文
			pRcvContext->m_pMng->DelSocketContext(pRcvContext->m_hSock);

			//接收错误释放缓冲区
			delete pRcvContext;
			pRcvContext = NULL;
			_TRACE("\r\nExcept : %s--%ld LAST_ERROR = %ld", __FILE__, iErrCode, WSAGetLastError());
		}
	}

	void CTcpMng::DelSocketContext(const SOCKET &s)
	{
		EnterCriticalSection(&m_SockMapLock);

		m_SockIter = m_SocketMap.find(s);
		if (m_SocketMap.end() != m_SockIter)
		{
			SOCKET_CONTEXT *pSocket = m_SockIter->second;

			delete pSocket;
			m_SocketMap.erase(m_SockIter++);
			_TRACE("\r\n %s -- %ld SOCKET = %ld 被删除, 还有 %ld 个SOCKET", __FILE__, __LINE__, s, m_SocketMap.size());
		}

		LeaveCriticalSection(&m_SockMapLock);
	}

	SOCKET_CONTEXT *CTcpMng::GetSocketContext(const SOCKET &sock)
	{
		SOCKET_CONTEXT *pSocket = NULL;

		EnterCriticalSection(&m_SockMapLock);

		CSocketMap::iterator sock_iter = m_SocketMap.find(sock);
		if (m_SocketMap.end() != sock_iter)
		{
			pSocket = sock_iter->second;
		}

		LeaveCriticalSection(&m_SockMapLock);

		return pSocket;
	}

	//当WSASend操作完成所需要进行的处理
	void CTcpMng::SendCompletionProc(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		TCP_CONTEXT* pSendContext = CONTAINING_RECORD(lpOverlapped, TCP_CONTEXT, m_ol);

#ifndef _XML_NET_
		WSABUF SendBuf;
#endif

		try
		{
			//若有错误产生将其放入无效的socket队列中
			if (0 != dwErrorCode && WSA_IO_PENDING != WSAGetLastError())
			{				
				closesocket(pSendContext->m_hSock);
				throw ((long)(__LINE__));
			}

#ifndef _XML_NET_
			pSendContext->m_nDataLen += dwNumberOfBytesTransfered;
			PACKET_HEAD* pHeadInfo = (PACKET_HEAD*)(pSendContext->m_pBuf);
#endif	//#ifndef _XML_NET_

			//修改交互时间
			pSendContext->m_pMng->ModifyInteractiveTime(pSendContext->m_hSock);

#ifndef _XML_NET_
			//数据没有发送完还需继续投递发送操作
			if ((pHeadInfo->nCurrentLen +sizeof(PACKET_HEAD) <= TCP_CONTEXT::S_PAGE_SIZE) 
				&& (pSendContext->m_nDataLen < pHeadInfo->nCurrentLen +sizeof(PACKET_HEAD)))
			{
				SendBuf.buf = pSendContext->m_pBuf +pSendContext->m_nDataLen;
				SendBuf.len = pHeadInfo->nCurrentLen - pSendContext->m_nDataLen +sizeof(PACKET_HEAD);

				INT iErrCode = WSASend(pSendContext->m_hSock, &SendBuf, 1, NULL, 0, &(pSendContext->m_ol), NULL);

				//投递失败
				if (SOCKET_ERROR == iErrCode && ERROR_IO_PENDING != WSAGetLastError())
				{						
					closesocket(pSendContext->m_hSock);
					throw ((long)(__LINE__));
				}
			}
			//发送完毕删除上下文
			else
			{
				delete pSendContext;
				pSendContext = NULL;
			}
#else
			delete pSendContext;
			pSendContext = NULL;
#endif	//#ifndef _XML_NET_

		}
		catch (const long& iErrCode)
		{
			//InterlockedExchange(&(pSendContext->m_nPostCount), 0);
			//修改发送标志, 释放发送缓冲区
			delete pSendContext;
			pSendContext = NULL;
			_TRACE("\r\nExcept : %s--%ld LAST_ERROR = %ld", __FILE__, iErrCode, WSAGetLastError());
		}
	}

	UINT WINAPI CTcpMng::ThreadProc(LPVOID lpParam)
	{	
		CTcpMng* const pThis = (CTcpMng*)lpParam;
		const DWORD TIME_OUT = 60 * 60;		

		while (InterlockedExchangeAdd((long volatile *)&(pThis->m_bThreadRun), 0))
		{		
			EnterCriticalSection(&(pThis->m_SockMapLock));
			const DWORD NOW_TIME = (DWORD)time(NULL);	

			if (pThis->m_SocketMap.end() != pThis->m_SockIter)
			{
				SOCKET_CONTEXT *pSocket = pThis->m_SockIter->second;
				//socket已超时
				if (NOW_TIME - pSocket->m_nInteractiveTime >= TIME_OUT)
				{
					_TRACE("\r\n%s -- %ld SOCKET %ld 已超时将被删除, NOW_TIME = %ld, pSocket->m_nInteractiveTime = %ld"
						, __FILE__, __LINE__, pThis->m_SockIter->first, NOW_TIME, pSocket->m_nInteractiveTime);

					pSocket->m_nInteractiveTime = 0;
					closesocket(pThis->m_SockIter->first);
				}

				pThis->m_SockIter++;
			}
			else
			{
				pThis->m_SockIter = pThis->m_SocketMap.begin();
			}

			LeaveCriticalSection(&(pThis->m_SockMapLock));			
			Sleep(500);
		}

		return 0;
	}
	
	void CTcpMng::PushInRecvQue(TCP_RCV_DATA* const pRcvData)
	{		
		assert(pRcvData);
		EnterCriticalSection(&m_RcvQueLock);
		m_RcvDataQue.push_back(pRcvData);
		LeaveCriticalSection(&m_RcvQueLock);
	}

	TCP_RCV_DATA* CTcpMng::GetRcvData(DWORD* const pQueLen, const SOCKET_CONTEXT *pSocket)
	{
		TCP_RCV_DATA* pRcvData = NULL;

#ifdef _XML_NET_		//XML数据流
		EnterCriticalSection(&m_RcvQueLock);
		//从队列头中取一个数据
		if (NULL == pSocket)
		{
			vector<TCP_RCV_DATA*>::iterator iter = m_RcvDataQue.begin();
			if (m_RcvDataQue.end() != iter)
			{
				pRcvData = *iter;
				m_RcvDataQue.erase(iter);
			}
		}
		//取一个与peer_sock相关的数据
		else
		{
			vector<TCP_RCV_DATA*>::iterator iter = m_RcvDataQue.begin();
			for (; m_RcvDataQue.end() != iter; iter++)
			{
				pRcvData = *iter;
				if (NULL != pRcvData && pSocket == pRcvData->m_pSocket)
				{
					m_RcvDataQue.erase(iter);
					break;
				}
				pRcvData = NULL;
			}
		}

		if (NULL != pQueLen)
		{
			*pQueLen = (DWORD)(m_RcvDataQue.size());
		}
		LeaveCriticalSection(&m_RcvQueLock);
#else					//二进制数据流
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
#endif
		return pRcvData;
	}
	
	BOOL CTcpMng::SendData(SOCKET_CONTEXT *const pSocket, const CHAR* szData, INT nDataLen)
	{
#ifdef _XML_NET_
		//数据长度非法
		if ((nDataLen > TCP_CONTEXT::S_PAGE_SIZE) || (NULL == szData))
		{
			return FALSE;
		}
#else
		//数据长度非法
		if ((nDataLen > TCP_CONTEXT::S_PAGE_SIZE) || (NULL == szData) || (nDataLen < sizeof(PACKET_HEAD)))
		{
			return FALSE;
		}
#endif	//#ifdef _XML_NET_

		BOOL bResult = TRUE;
		DWORD dwBytes = 0;
		WSABUF SendBuf;
		TCP_CONTEXT *pSendContext = new TCP_CONTEXT(this);
		if (NULL == pSendContext)
		{
			return FALSE;
		}

		pSendContext->m_hSock = pSocket->m_hSocket;
		pSendContext->m_nDataLen = 0;
		pSendContext->m_nOperation = OP_WRITE;
		memcpy(pSendContext->m_pBuf, szData, nDataLen);

		SendBuf.buf = pSendContext->m_pBuf;
		SendBuf.len = nDataLen;

		assert(szData);		
		INT iErr = WSASend(pSendContext->m_hSock, &SendBuf, 1, &dwBytes, 0, &(pSendContext->m_ol), NULL);

		if (SOCKET_ERROR == iErr && ERROR_IO_PENDING != WSAGetLastError())
		{
			closesocket(pSendContext->m_hSock);

			delete pSendContext;
			pSendContext = NULL;
			_TRACE("\r\n%s : %ld LAST_ERROR = %ld", __FILE__, __LINE__, WSAGetLastError());
			//SetSocketClosed(sock);			
			bResult = FALSE;
		}

		return bResult;
	}

	void CTcpMng::ModifyInteractiveTime(const SOCKET& s)
	{
		SOCKET_CONTEXT* pContext = NULL;

		EnterCriticalSection(&m_SockMapLock);
		CSocketMap::iterator iter = m_SocketMap.find(s);

		if (m_SocketMap.end() != iter)
		{
			pContext = iter->second;
			pContext->m_nInteractiveTime = (DWORD)time(NULL);
			//_TRACE("\r\n%s : %ld SOCKET = %ld pContext->m_nInteractiveTime = %ld"
			//	, __FILE__
			//	, __LINE__
			//	, s
			//	, pContext->m_nInteractiveTime
			//	);
		}
		LeaveCriticalSection(&m_SockMapLock);
	}
	
	void CTcpMng::InitReource()
	{
		WSADATA wsData;
		WSAStartup(MAKEWORD(2, 2), &wsData);

		//TCP_CONTEXT
		TCP_CONTEXT::s_hHeap = HeapCreate(0, TCP_CONTEXT::S_PAGE_SIZE, TCP_CONTEXT::E_TCP_HEAP_SIZE);
		InitializeCriticalSection(&(TCP_CONTEXT::s_IDLQueLock));
		TCP_CONTEXT::s_IDLQue.reserve(TCP_CONTEXT::MAX_IDL_DATA * sizeof(void*));
		//_TRACE("\r\n%s:%ld TCPMNG_CONTEXT::s_hHeap = 0x%x sizeof(TCPMNG_CONTEXT*) = %ld"
		//	, __FILE__, __LINE__, TCPMNG_CONTEXT::s_hHeap, sizeof(TCPMNG_CONTEXT*));

		//CONNECT_CONTEXT
		//CONNECT_CONTEXT::s_hHeap = HeapCreate(0, CONNECT_CONTEXT::S_PAGE_SIZE, CONNECT_CONTEXT::HEAP_SIZE);
		//InitializeCriticalSection(&(CONNECT_CONTEXT::s_IDLQueLock));

		//ACCEPT_CONTEXT
		ACCEPT_CONTEXT::s_hHeap = HeapCreate(0, ACCEPT_CONTEXT::S_PAGE_SIZE, 100 * 1024);
		InitializeCriticalSection(&(ACCEPT_CONTEXT::s_IDLQueLock));
		ACCEPT_CONTEXT::s_IDLQue.reserve(500 * sizeof(void*));
		//_TRACE("\r\n%s:%ld nACCEPT_CONTEXT::s_hHeap = 0x%x", __FILE__, __LINE__, ACCEPT_CONTEXT::s_hHeap);

		//TCP_RCV_DATA
		TCP_RCV_DATA::s_hHeap = HeapCreate(0, 0, TCP_RCV_DATA::HEAP_SIZE);
		TCP_RCV_DATA::s_DataHeap = HeapCreate(0, 0, TCP_RCV_DATA::DATA_HEAP_SIZE);
		InitializeCriticalSection(&(TCP_RCV_DATA::s_IDLQueLock));
		TCP_RCV_DATA::s_IDLQue.reserve(TCP_RCV_DATA::MAX_IDL_DATA * sizeof(void*));

		//_TRACE("\r\n%s:%ld TCP_RCV_DATA::s_hHeap = 0x%x", __FILE__, __LINE__, TCP_RCV_DATA::s_hHeap);
		//_TRACE("\r\n%s:%ld TCP_RCV_DATA::s_DataHeap = 0x%x", __FILE__, __LINE__, TCP_RCV_DATA::s_DataHeap);

		//SOCKET_CONTEXT
		SOCKET_CONTEXT::s_hHeap = HeapCreate(0, 0, SOCKET_CONTEXT::HEAP_SIZE);
		InitializeCriticalSection(&(SOCKET_CONTEXT::s_IDLQueLock));
		SOCKET_CONTEXT::s_IDLQue.reserve(SOCKET_CONTEXT::MAX_IDL_SIZE* sizeof(void*));
	}

	void CTcpMng::ReleaseReource()
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

		//SOCKET_CONTEXT
		HeapDestroy(SOCKET_CONTEXT::s_hHeap);
		SOCKET_CONTEXT::s_hHeap = NULL;

		EnterCriticalSection(&(SOCKET_CONTEXT::s_IDLQueLock));
		SOCKET_CONTEXT::s_IDLQue.clear();
		LeaveCriticalSection(&(SOCKET_CONTEXT::s_IDLQueLock));
		DeleteCriticalSection(&(SOCKET_CONTEXT::s_IDLQueLock));


		WSACleanup();
	}
}