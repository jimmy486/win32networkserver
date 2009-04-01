#include "./ClientNet.h"

#ifdef WIN32
#include <atltrace.h>

#ifdef _DEBUG_ATL
#define _TRACE ATLTRACE
#else
#define _TRACE 
#endif		//#ifdef _DEBUG

#endif		//#ifdef WIN32

namespace HelpMng
{

	//CLIENT_TCP_CONTEXT
	HANDLE CLIENT_CONTEXT::s_hHeap = NULL;
	vector<CLIENT_CONTEXT* > CLIENT_CONTEXT::s_IDLQue;
	CRITICAL_SECTION CLIENT_CONTEXT::s_IDLQueLock;


	//CLIENT_SEND_DATA
	HANDLE CLIENT_SEND_DATA::s_DataHeap = NULL;
	HANDLE CLIENT_SEND_DATA::s_hHeap = NULL;
	vector<CLIENT_SEND_DATA*> CLIENT_SEND_DATA::s_IDLQue;
	CRITICAL_SECTION CLIENT_SEND_DATA::s_IDLQueLock;		

	//ClientNet
	LPFN_CONNECTEX ClientNet::s_pfConnectEx = NULL;
	LPFN_ACCEPTEX ClientNet::s_pfAcceptEx = NULL;
	LPFN_GETACCEPTEXSOCKADDRS ClientNet::s_pfGetAddrs = NULL;
	HANDLE ClientNet::s_hHeap = NULL;
	vector<ClientNet* >ClientNet::s_IDLQue;
	CRITICAL_SECTION ClientNet::s_IDLQueLock;

	//CLIENT_TCP_CONTEXT的相关实现
	void* CLIENT_CONTEXT:: operator new(size_t nSize)
	{
		void* pContext = NULL;

		try
		{		
			if (NULL == s_hHeap)
			{
				THROW_LINE;
			}

			//申请内存, 先从空闲队列中申请；当空闲队列为空时再从堆中申请
            //bool bQueEmpty = true;
			EnterCriticalSection(&s_IDLQueLock);
			vector<CLIENT_CONTEXT* >::iterator iter = s_IDLQue.begin();

			if (s_IDLQue.end() != iter)
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

	void CLIENT_CONTEXT:: operator delete(void* p)
	{
		if (p)
		{
			EnterCriticalSection(&s_IDLQueLock);
			const DWORD QUE_SIZE = (DWORD)(s_IDLQue.size());
			CLIENT_CONTEXT* const pContext = (CLIENT_CONTEXT*)p;

			if (QUE_SIZE <= MAX_IDL_NUM)
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

	BOOL CLIENT_CONTEXT::IsAddressValid(LPCVOID lpMem)
	{
		BOOL bResult = HeapValidate(s_hHeap, 0, lpMem);

		if (NULL == lpMem)
		{
			bResult = FALSE;
		}	

		return bResult;
	}

	//CLIENT_SEND_DATA的相关实现	
	CLIENT_SEND_DATA::CLIENT_SEND_DATA(const CHAR* szData, INT nLen)	
	{
		try
		{
			if (NULL == s_DataHeap)
			{
				THROW_LINE;
			}

			m_pData = (CHAR*)HeapAlloc(s_DataHeap, HEAP_ZERO_MEMORY, nLen);
			m_nLen = nLen;
			if (m_pData)
			{
				memcpy(m_pData, szData, nLen);
			}
		}
		catch (const long& iErrCode)
		{
			_TRACE("Exp : %s--%ld", __FILE__, iErrCode);
			m_pData = NULL;
		}
	}

	CLIENT_SEND_DATA::CLIENT_SEND_DATA(const CHAR* szData, INT nLen, const IP_ADDR& peer_ip)
		: m_PeerIP(peer_ip)
		, m_nLen(nLen)
	{
		try
		{
			if (NULL == s_DataHeap)
			{
				THROW_LINE;
			}

			m_pData = (CHAR*)HeapAlloc(s_DataHeap, HEAP_ZERO_MEMORY, nLen);
			if (m_pData)
			{
				memcpy(m_pData, szData, nLen);
			}
		}
		catch (const long& iErrCode)
		{
			_TRACE("Exp : %s--%ld", __FILE__, iErrCode);
			m_pData = NULL;
		}
	}

	CLIENT_SEND_DATA::~CLIENT_SEND_DATA()
	{
		if (IsDataValid())
		{
			HeapFree(s_DataHeap, 0, m_pData);
			m_pData = NULL;
		}
	}

	void* CLIENT_SEND_DATA::operator new(size_t nSize)
	{
		void* pSendData = NULL;
		try
		{		
			if (NULL == s_hHeap || NULL == s_DataHeap)
			{
				THROW_LINE;
			}

			//申请内存现在空闲队列中找, 当找不到时则从堆上申请
			//bool bQueEmpty = true;

			EnterCriticalSection(&s_IDLQueLock);
			vector<CLIENT_SEND_DATA* >::iterator iter = s_IDLQue.begin();
			if (s_IDLQue.end() != iter)
			{
				pSendData = *iter;
				s_IDLQue.erase(iter);
				//bQueEmpty = false;
			}
			else
			{
				pSendData = HeapAlloc(s_hHeap, HEAP_ZERO_MEMORY | HEAP_NO_SERIALIZE, nSize);
			}
			LeaveCriticalSection(&s_IDLQueLock);
			
			if (NULL == pSendData)
			{
				THROW_LINE;
			}
		}
		catch (const long& iErrCode)
		{
			pSendData = NULL;
			_TRACE("Exp : %s--%ld", __FILE__, iErrCode);
		}

		return pSendData;
	}

	void CLIENT_SEND_DATA::operator delete(void* p)
	{
		if (p)
		{
			EnterCriticalSection(&s_IDLQueLock);
			const DWORD QUE_SIZE = (DWORD)(s_IDLQue.size());
			CLIENT_SEND_DATA* const pContext = (CLIENT_SEND_DATA*)p;

			if (QUE_SIZE <= MAX_IDL_NUM)
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


	BOOL CLIENT_SEND_DATA::IsAddressValid(LPCVOID pMem)
	{
		BOOL bResult = HeapValidate(s_hHeap, 0, pMem);

		if (NULL == pMem)
		{
			bResult = FALSE;
		}

		return bResult;
	}

	BOOL CLIENT_SEND_DATA::IsDataValid()
	{
		BOOL bResult = HeapValidate(s_DataHeap, 0, m_pData);

		if (NULL == m_pData)
		{
			bResult = FALSE;
		}

		return bResult;
	}


	//ClientNet的相关实现
	ClientNet::ClientNet(BOOL bCallback)
		: c_bUseCallback(bCallback)
		, m_pParam(NULL)
		, m_pOnAccept(NULL)
		, m_pOnError(NULL)		
		, m_pOnRead(NULL)
		, m_pOnReadFrom(NULL)
		, m_pOnConnect(NULL)
		, m_pRcvContext(NULL)
		, m_pSendContext(NULL)
		, m_hSock(INVALID_SOCKET)
	{
		InitializeCriticalSection(&m_SendDataLock);
	}

	ClientNet::ClientNet(
		const SOCKET& SockClient
		, BOOL bUseCallback
		, LPVOID pParam			//bUseCallback = true时有效
		, LPONREAD pOnRead		//bUseCallback = true时有效
		, LPONERROR pOnError	//bUseCallback = true时有效
		)
		: m_hSock(SockClient)
		, c_bUseCallback(bUseCallback)
		, m_pParam(pParam)
		, m_pOnAccept(NULL)
		, m_pOnError(pOnError)
		, m_pOnRead(pOnRead)
		, m_pOnReadFrom(NULL)
		, m_pOnConnect(NULL)
		, m_pRcvContext(NULL)
		, m_pSendContext(NULL)
	{
		InitializeCriticalSection(&m_SendDataLock);

		try 
		{
			if (INVALID_SOCKET == SockClient)
			{
				if (FALSE == bUseCallback)
				{
					OnError(ERR_SOCKET_INVALID);
				}
				else if (m_pOnError)
				{
					m_pOnError(m_pParam, ERR_SOCKET_INVALID);
				}
				THROW_LINE;
			}

			//只能用于TCP类型

			ULONG ul = 1;
			ioctlsocket(m_hSock, FIONBIO, &ul);
			//设置socket的缓存, 此处将用到系统接收缓存
			INT nBufSize = 0;
			setsockopt(m_hSock, SOL_SOCKET, SO_SNDBUF, (char*)&nBufSize, sizeof(nBufSize));
			nBufSize = 65535;
			setsockopt(m_hSock, SOL_SOCKET, SO_RCVBUF, (CHAR*)&nBufSize, sizeof(nBufSize));
            
			//申请收发缓冲区	
			m_pSendContext = new CLIENT_CONTEXT(this);
			m_pRcvContext = new CLIENT_CONTEXT(this);


			if (NULL == m_pSendContext || NULL == m_pRcvContext)
			{
				if (FALSE == bUseCallback)
				{
					OnError(ERR_INIT);
				}
				else if (m_pOnError)
				{
					m_pOnError(m_pParam, ERR_INIT);
				}

				THROW_LINE;
			}

			//将socket与IO端口进行帮定
			if (FALSE == BindIoCompletionCallback((HANDLE)m_hSock, IOCompletionRoutine, 0))
			{
				if (FALSE == bUseCallback)
				{
					OnError(ERR_INIT);
				}
				else if (m_pOnError)
				{
					m_pOnError(m_pParam, ERR_INIT);
				}

				THROW_LINE;
			}

			//投递读操作
			if (FALSE == PostRecv(m_pRcvContext, m_hSock))
			{
				if (FALSE == bUseCallback)
				{
					OnError(ERR_INIT);
				}
				else if (m_pOnError)
				{
					m_pOnError(m_pParam, ERR_INIT);
				}

				THROW_LINE;
			}

		}
		catch (const long& iErrCode)
		{
			closesocket(m_hSock);
			m_hSock = INVALID_SOCKET;

			delete m_pSendContext;
			m_pSendContext = NULL;

			delete m_pRcvContext;
			m_pRcvContext = NULL;

			_TRACE("Exp : %s--%ld", __FILE__, iErrCode);
		}
	}

	ClientNet::~ClientNet(void)
	{
		closesocket(m_hSock);
		m_hSock = INVALID_SOCKET;

		//释放发送缓冲区队列
		EnterCriticalSection(&m_SendDataLock);

		for (vector<CLIENT_SEND_DATA* >::iterator iterData = m_SendDataQue.begin(); m_SendDataQue.end() != iterData; iterData++)
		{
			CLIENT_SEND_DATA* pSendData = *iterData;
			delete pSendData;
			pSendData = NULL;
		}
		m_SendDataQue.clear();

		LeaveCriticalSection(&m_SendDataLock);

		//等待一段时间以使IO线程有足够的时间完成操作
		while (true)
		{
			if (NULL != m_pRcvContext /*&& NULL != m_pSendContext*/)
			{
				if (m_pRcvContext->m_nPostCount <= 0 /*&& m_pSendContext->m_nPostCount <= 0*/)
				{
					Sleep(100);
					break;
				}
			}
			else
			{
				Sleep(100);
				break;
			}
		}

		delete m_pRcvContext;
		m_pRcvContext = NULL;

		delete m_pSendContext;
		m_pSendContext = NULL;

		DeleteCriticalSection(&m_SendDataLock);

	}

	void* ClientNet::operator new(size_t nSize)
	{
		void* pContext = NULL;

		try
		{		
			if (NULL == s_hHeap)
			{
				THROW_LINE;
			}

			//申请内存, 先从空闲队列中申请；当空闲队列为空时再从堆中申请
			//bool bQueEmpty = true;
			EnterCriticalSection(&s_IDLQueLock);
			vector<ClientNet* >::iterator iter = s_IDLQue.begin();

			if (s_IDLQue.end() != iter)
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

	void ClientNet::operator delete(void* p)
	{
		if (p)
		{
			EnterCriticalSection(&s_IDLQueLock);
			const DWORD QUE_SIZE = (DWORD)(s_IDLQue.size());
			ClientNet* const pContext = (ClientNet*)p;

			if (QUE_SIZE <= E_MAX_IDL_NUM)
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


	BOOL ClientNet::IsAddressValid(LPCVOID lpMem)
	{
		BOOL bResult = HeapValidate(s_hHeap, 0, lpMem);

		if (NULL == lpMem)
		{
			bResult = FALSE;
		}	

		return bResult;
	}


	//BOOL ClientNet::Init(INT nNetType, const CHAR* szSerIP, INT nSerPort)
	//{
	//	return TRUE;
	//}

	void CALLBACK ClientNet::IOCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		CLIENT_CONTEXT* pContext = CONTAINING_RECORD(lpOverlapped, CLIENT_CONTEXT, m_ol);

		//当网络关闭或有错误时清除发送队列
		if (0 != dwErrorCode 
			|| (0 == dwNumberOfBytesTransfered && OP_ACCEPT != pContext->m_nOperation && OP_CONNECT != pContext->m_nOperation))
		{
			_TRACE("%s -- %ld : LAST_ERROR = %ld, dwErrorCode = %ld, dwNumberOfBytesTransfered = %ld"
				, __FILE__, __LINE__, WSAGetLastError(), dwErrorCode, dwNumberOfBytesTransfered);
			//pContext->m_pClient->ClearSendQue();
			closesocket(pContext->m_hSock);
			InterlockedExchange(&(pContext->m_nPostCount), 0);

			if (FALSE == pContext->m_pClient->c_bUseCallback)
			{
				pContext->m_pClient->OnError(pContext->m_nOperation);
			}
			else if (pContext->m_pClient->m_pOnError)
			{
				pContext->m_pClient->m_pOnError(pContext->m_pClient->m_pParam, pContext->m_nOperation);
			}

			return;
		}

		switch (pContext->m_nOperation)
		{
		case OP_ACCEPT:
			AcceptCompletion(dwErrorCode, dwNumberOfBytesTransfered, lpOverlapped);
			break;

		case OP_CONNECT:
			//为其投递读操作
			InterlockedExchange(&(pContext->m_nPostCount), 0);

			pContext->m_pClient->PostRecv();

			if (FALSE == pContext->m_pClient->c_bUseCallback)
			{
				pContext->m_pClient->OnConnect();
			}
			else if (pContext->m_pClient->m_pOnConnect)
			{
				pContext->m_pClient->m_pOnConnect(pContext->m_pClient->m_pParam);
			}

			break;

		case OP_READ:
			ReadCompletion(dwErrorCode, dwNumberOfBytesTransfered, lpOverlapped);
			break;

		case OP_WRITE:
			SendCompletion(dwErrorCode, dwNumberOfBytesTransfered, lpOverlapped);
			break;

		default :
			break;
		}

		return;
	}

	void ClientNet::OnConnect()
	{
		return;
	}

	INT ClientNet::OnRead(const CHAR* szData, INT nDataLen)
	{
		return 0;
	}

	VOID ClientNet::OnReadFrom(const IP_ADDR& PeerAddr, const CHAR* szData, INT nDataLen)
	{
		return;
	}

	void ClientNet::OnAccept(const SOCKET& SockClient, const IP_ADDR& AddrClient)
	{
		return;
	}

	void ClientNet::OnError(int nOperation )
	{

	}

	BOOL ClientNet::PostRecv(CLIENT_CONTEXT* const pContext, const SOCKET& sock)
	{
		BOOL bResult = TRUE;

		try
		{
			WSABUF RcvBuf;
			DWORD dwBytes;
			DWORD dwFlag = 0;
			INT nErr;
			INT nSoType = 0xffffffff;
			INT nTypeLen = sizeof(nSoType);

			//已经投递了读操作, 不能在投递
			if (1 == pContext->m_nPostCount)
			{
				THROW_LINE;
			}

			getsockopt(sock, SOL_SOCKET, SO_TYPE, (char*)(&nSoType), &nTypeLen);
			if (SOCK_STREAM == nSoType)
			{
				InterlockedExchange(&(pContext->m_nPostCount), 1);
				pContext->m_hSock = sock;
				pContext->m_nOperation = OP_READ;
				pContext->m_nAllLen = 0;	
				pContext->m_nCurrentLen = 0;
				RcvBuf.buf = pContext->m_pBuf;
				RcvBuf.len = CLIENT_CONTEXT::S_PAGE_SIZE;
				nErr = WSARecv(pContext->m_hSock, &RcvBuf, 1, &dwBytes, &dwFlag, &(pContext->m_ol), NULL);

				if (SOCKET_ERROR == nErr && ERROR_IO_PENDING != WSAGetLastError())
				{
					//INT xx = WSAGetLastError();
					_TRACE("%s : %ld LAST_ERROR = %ld", __FILE__, __LINE__, WSAGetLastError());
					closesocket(pContext->m_hSock);
					InterlockedExchange(&(pContext->m_nPostCount), 0);
					THROW_LINE;
				}
			}
			else if (SOCK_DGRAM == nSoType)
			{
				InterlockedExchange(&(pContext->m_nPostCount), 1);
				pContext->m_hSock = sock;
				pContext->m_nOperation = OP_READ;			
				//pContext->m_PeerIP = IP_ADDR(m_szSerIP, m_nSerPort);
				RcvBuf.buf = pContext->m_pBuf;
				RcvBuf.len = CLIENT_CONTEXT::S_PAGE_SIZE;

				INT nAddrLen = sizeof(pContext->m_PeerIP);
				nErr = WSARecvFrom(pContext->m_hSock, &RcvBuf, 1, &dwBytes, &dwFlag, (sockaddr*)&(pContext->m_PeerIP)
					, &nAddrLen, &(pContext->m_ol), NULL);

				//ATLTRACE("\r\n%s -- %ld LAST_ERR = %ld", __FILE__, __LINE__, WSAGetLastError());
				if (SOCKET_ERROR == nErr && ERROR_IO_PENDING != WSAGetLastError())
				{
					closesocket(pContext->m_hSock);
					InterlockedExchange(&(pContext->m_nPostCount), 0);
					THROW_LINE;
				}
			}
			else
			{
				THROW_LINE;
			}
		}
		catch (const long& iErrCode)
		{
			bResult = FALSE;
			_TRACE("Exp : %s--%ld", __FILE__, iErrCode);
		}

		return bResult;
	}

	BOOL ClientNet::PostRecv()
	{
		return PostRecv(m_pRcvContext, m_hSock);
	}

	void ClientNet::ReadCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		CLIENT_CONTEXT* pContext = CONTAINING_RECORD(lpOverlapped, CLIENT_CONTEXT, m_ol);
		WSABUF RcvBuf;
		INT iErrCode;
		INT nRcvLen;			//还需接受多少数据只对TCP有效
		INT nSoType = 0xffffffff;
		DWORD dwBytes;
		DWORD dwFlag = 0;
		INT nTypeLen = sizeof(nSoType);

		getsockopt(pContext->m_hSock, SOL_SOCKET, SO_TYPE, (char*)&nSoType, &nTypeLen);
		_TRACE("%s -- %ld nSoType = %ld", __FILE__, __LINE__, nSoType);
		//TCP模式
		if (SOCK_STREAM == nSoType)
		{
			pContext->m_nCurrentLen += (WORD)dwNumberOfBytesTransfered;

			if (FALSE == pContext->m_pClient->c_bUseCallback)
			{
				nRcvLen = pContext->m_pClient->OnRead(pContext->m_pBuf, pContext->m_nCurrentLen);
			}
			else if (pContext->m_pClient->m_pOnRead)
			{
				nRcvLen = pContext->m_pClient->m_pOnRead(pContext->m_pClient->m_pParam, pContext->m_pBuf, pContext->m_nCurrentLen);
			}

			//一个完整的数据包已接收完, 可以继续接受下一个数据包
			if (nRcvLen <= 0 || (DWORD)(nRcvLen + pContext->m_nCurrentLen) > CLIENT_CONTEXT::S_PAGE_SIZE)
			{
				pContext->m_nCurrentLen = 0;
				pContext->m_nOperation = OP_READ;
				RcvBuf.buf = pContext->m_pBuf;
				RcvBuf.len = CLIENT_CONTEXT::S_PAGE_SIZE;
			}
			//该数据包还未接收完毕, 需要继续接收
			else 
			{
				pContext->m_nOperation = OP_READ;
				RcvBuf.buf = pContext->m_pBuf + pContext->m_nCurrentLen;
				RcvBuf.len = nRcvLen;
			}
			_TRACE("%s -- %ld RcvBuf.len = %ld", __FILE__, __LINE__, RcvBuf.len);
			
			iErrCode = WSARecv(pContext->m_hSock, &RcvBuf, 1, &dwBytes, &dwFlag, &(pContext->m_ol), NULL);
			if (SOCKET_ERROR == iErrCode && ERROR_IO_PENDING != WSAGetLastError())
			{
				_TRACE("%s -- %ld LAST_ERROR = %ld", __FILE__, __LINE__, WSAGetLastError());
				closesocket(pContext->m_hSock);
				InterlockedExchange(&(pContext->m_nPostCount), 0);
			}
		}
		//UDP模式
		else if (SOCK_DGRAM == nSoType)
		{
			if (FALSE == pContext->m_pClient->c_bUseCallback)
			{
				pContext->m_pClient->OnReadFrom(pContext->m_PeerIP, pContext->m_pBuf, dwNumberOfBytesTransfered);
			}
			else if (pContext->m_pClient->m_pOnReadFrom)
			{
				pContext->m_pClient->m_pOnReadFrom(
					pContext->m_pClient->m_pParam
					, pContext->m_PeerIP
					, pContext->m_pBuf
					, dwNumberOfBytesTransfered);
			}

			//继续投递接受操作
			pContext->m_nOperation = OP_READ;
			RcvBuf.buf = pContext->m_pBuf;
			RcvBuf.len = CLIENT_CONTEXT::S_PAGE_SIZE;
			INT nAddrLen = sizeof(pContext->m_PeerIP);

			iErrCode = WSARecvFrom(pContext->m_hSock, &RcvBuf, 1, &dwBytes, &dwFlag
				, (sockaddr*)&(pContext->m_PeerIP), &nAddrLen, &(pContext->m_ol), NULL);

			if (SOCKET_ERROR == iErrCode && ERROR_IO_PENDING != WSAGetLastError())
			{
				_TRACE("%s -- %ld LAST_ERROR = %ld", __FILE__, __LINE__, WSAGetLastError());
				closesocket(pContext->m_hSock);
				InterlockedExchange(&(pContext->m_nPostCount), 0);
			}
		}
	}

	void ClientNet::SendCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		CLIENT_CONTEXT* pContext = CONTAINING_RECORD(lpOverlapped, CLIENT_CONTEXT, m_ol);
		WSABUF SendBuf;
		INT iErrCode;
		INT nSoType = 0xffffffff;
		BOOL bSend = FALSE;		//是否继续投递发送操作
		DWORD dwBytes;
		CLIENT_SEND_DATA* pSendData = NULL;
		INT nTypeLen = sizeof(nSoType);

		getsockopt(pContext->m_hSock, SOL_SOCKET, SO_TYPE, (char*)&nSoType, &nTypeLen);

		//TCP模式
		if (SOCK_STREAM == nSoType)
		{
			pContext->m_nCurrentLen += (WORD)dwNumberOfBytesTransfered;

			//本数据包还未发送完成
			if(pContext->m_nCurrentLen < pContext->m_nAllLen)
			{
				pContext->m_nOperation = OP_WRITE;
				SendBuf.buf = pContext->m_pBuf +pContext->m_nCurrentLen;
				SendBuf.len = pContext->m_nAllLen - pContext->m_nCurrentLen;
				bSend = TRUE;
			}
			//本数据包已发送完毕, 可以发送下一个数据包
			else
			{
				pContext->m_nCurrentLen = 0;
				pContext->m_nOperation = OP_WRITE;
				pSendData = pContext->m_pClient->GetSendData();

				//对列为空则不需要再投递发送操作
				if (NULL == pSendData)
				{
					InterlockedExchange(&(pContext->m_nPostCount), 0);
					bSend = FALSE;
				}
				else
				{
					memcpy(pContext->m_pBuf, pSendData->m_pData, pSendData->m_nLen);
					pContext->m_nAllLen = pSendData->m_nLen;
					SendBuf.buf = pContext->m_pBuf;
					SendBuf.len = pContext->m_nAllLen;
					bSend = TRUE;
					delete pSendData;
					pSendData = NULL;
				}
			}

			if (TRUE == bSend)
			{				
				iErrCode = WSASend(pContext->m_hSock, &SendBuf, 1, &dwBytes, 0, &(pContext->m_ol), NULL);
				//若发送失败先不用关闭socket
				if (SOCKET_ERROR == iErrCode && ERROR_IO_PENDING != WSAGetLastError())
				{
					//closesocket(pContext->m_hSock);
					InterlockedExchange(&(pContext->m_nPostCount), 0);
				}
			}
		}
		//UDP模式
		else if (SOCK_DGRAM == nSoType)
		{
			pContext->m_nOperation = OP_WRITE;
			pSendData = pContext->m_pClient->GetSendData();

			//ATLTRACE("\r\n %s -- %ld m_nPostCount = %ld pSendData = 0x%x", __FILE__, __LINE__, pContext->m_nPostCount, pSendData);
			//对列为空则不需要再投递发送操作
			if (NULL == pSendData)
			{
				InterlockedExchange(&(pContext->m_nPostCount), 0);
				//ATLTRACE("\r\n %s -- %ld m_nPostCount = %ld", __FILE__, __LINE__, pContext->m_nPostCount);
				bSend = FALSE;
			}
			else
			{
				memcpy(pContext->m_pBuf, pSendData->m_pData, pSendData->m_nLen);
				pContext->m_nAllLen = pSendData->m_nLen;
				pContext->m_PeerIP = pSendData->m_PeerIP;
				SendBuf.buf = pContext->m_pBuf;
				SendBuf.len = pContext->m_nAllLen;
				bSend = TRUE;
				delete pSendData;
				pSendData = NULL;
			}

			if (TRUE == bSend)
			{
				iErrCode = WSASendTo(pContext->m_hSock, &SendBuf, 1, &dwBytes, 0
					, (sockaddr*)&(pContext->m_PeerIP), sizeof(pContext->m_PeerIP), &(pContext->m_ol), NULL);

				if (SOCKET_ERROR == iErrCode && ERROR_IO_PENDING != WSAGetLastError())
				{
					//closesocket(pContext->m_hSock);
					InterlockedExchange(&(pContext->m_nPostCount), 0);
				}
			}
		}
	}

	void ClientNet::AcceptCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		CLIENT_CONTEXT* pContext = CONTAINING_RECORD(lpOverlapped, CLIENT_CONTEXT, m_ol);

		IP_ADDR* pClientAddr;
		IP_ADDR* pLocalAddr;
		INT nClientLen;
		INT nLocalLen;

		setsockopt(pContext->m_ClientSock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&(pContext->m_hSock), sizeof(pContext->m_hSock));

		s_pfGetAddrs(pContext->m_pBuf, 0, sizeof(sockaddr_in) +16, sizeof(sockaddr_in) +16
			, (LPSOCKADDR*)&pLocalAddr, &nLocalLen, (LPSOCKADDR*)&pClientAddr, &nClientLen);

		if (FALSE == pContext->m_pClient->c_bUseCallback)
		{
			pContext->m_pClient->OnAccept(pContext->m_ClientSock, *pClientAddr);			
		}
		else if (pContext->m_pClient->m_pOnAccept)
		{
			pContext->m_pClient->m_pOnAccept(pContext->m_pClient->m_pParam, pContext->m_ClientSock, *pClientAddr);
		}

		//继续投递acceptex操作
		SOCKET SockClient = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		DWORD dwBytes;

		pContext->m_ClientSock = SockClient;
		INT nRet = s_pfAcceptEx(pContext->m_hSock, pContext->m_ClientSock, pContext->m_pBuf
			, 0, sizeof(sockaddr_in) +16, sizeof(sockaddr_in) +16, &dwBytes, &(pContext->m_ol));
		if (FALSE == nRet && ERROR_IO_PENDING != WSAGetLastError())
		{		
			closesocket(pContext->m_hSock);
			closesocket(SockClient);
			InterlockedExchange(&pContext->m_nPostCount, 0);
		}

	}

	BOOL ClientNet::Send(const CHAR* szData, INT nLen)
	{
		//数据非法
		if (NULL == szData || (DWORD)nLen > CLIENT_CONTEXT::S_PAGE_SIZE)
		{
			return FALSE;
		}

		INT iErrCode = 0;
		INT nSoType = 0xffffffff;
		BOOL bResult = TRUE;
		INT nTypeLen = sizeof(nSoType);

		getsockopt(m_hSock, SOL_SOCKET, SO_TYPE, (char*)&nSoType, &nTypeLen);

		//socket不是TCP类型的
		if (SOCK_STREAM != nSoType)
		{
			return FALSE;
		}

		//网络上没有发送操作直接进行投递操作
		if ( 0 == m_pSendContext->m_nPostCount)
		{
			WSABUF SendBuf;
			DWORD dwBytes;
			InterlockedExchange(&(m_pSendContext->m_nPostCount), 1);
			m_pSendContext->m_hSock = m_hSock;
			m_pSendContext->m_nAllLen = nLen;
			m_pSendContext->m_nCurrentLen = 0;
			m_pSendContext->m_nOperation = OP_WRITE;
			memcpy(m_pSendContext->m_pBuf, szData, nLen);	
			SendBuf.buf = m_pSendContext->m_pBuf;
			SendBuf.len = nLen;

			iErrCode = WSASend(m_pSendContext->m_hSock, &SendBuf, 1, &dwBytes, 0, &(m_pSendContext->m_ol), NULL);
			if (SOCKET_ERROR == iErrCode && ERROR_IO_PENDING != WSAGetLastError())
			{
				//closesocket(m_pSendContext->m_hSock);
				InterlockedExchange(&(m_pSendContext->m_nPostCount), 0);
				bResult = FALSE;
				_TRACE("%s : %ld LAST_ERROR = %ld", __FILE__, __LINE__, WSAGetLastError());
			}
		}
		//网络IO正在进行发送操作, 将数据压入到队列中
		else
		{
			EnterCriticalSection(&m_SendDataLock);

			CLIENT_SEND_DATA* pSendData = new CLIENT_SEND_DATA(szData, nLen);
			if (pSendData)
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

	void ClientNet::GetLocalAddr(string& szIp, int& nPort)
	{
		IP_ADDR local_addr;
		int nBytes = sizeof(local_addr);

		getsockname(m_hSock, (sockaddr*)&local_addr, &nBytes);
		szIp = inet_ntoa(local_addr.sin_addr);
		nPort = ntohs(local_addr.sin_port);
	}

	BOOL ClientNet::SendTo(const IP_ADDR& PeerAddr, const CHAR* szData, INT nLen)
	{
		//数据非法
		if (NULL == szData || nLen > 1500)
		{
			return FALSE;
		}

		BOOL bResult = TRUE;
		INT iErrCode = 0;
		INT nSoType = 0xffffffff;
		INT nTypeLen = sizeof(nSoType);

		getsockopt(m_hSock, SOL_SOCKET, SO_TYPE, (char*)&nSoType, &nTypeLen);
		if (SOCK_DGRAM != nSoType)
		{
			return FALSE;
		}

		//网络上没有发送操作直接进行投递操作
		if ( 0 == m_pSendContext->m_nPostCount)
		{
			WSABUF SendBuf;
			DWORD dwBytes;
			InterlockedExchange(&(m_pSendContext->m_nPostCount), 1);
			m_pSendContext->m_hSock = m_hSock;
			m_pSendContext->m_nAllLen = nLen;
			m_pSendContext->m_nCurrentLen= 0;
			m_pSendContext->m_nOperation = OP_WRITE;
			m_pSendContext->m_PeerIP = PeerAddr;
			memcpy(m_pSendContext->m_pBuf, szData, nLen);	
			SendBuf.buf = m_pSendContext->m_pBuf;
			SendBuf.len = nLen;

			iErrCode = WSASendTo(m_pSendContext->m_hSock, &SendBuf, 1, &dwBytes, 0
				,(sockaddr*)&(m_pSendContext->m_PeerIP), sizeof(m_pSendContext->m_PeerIP), &(m_pSendContext->m_ol), NULL);
			if (SOCKET_ERROR == iErrCode && ERROR_IO_PENDING != WSAGetLastError())
			{
				//closesocket(m_pSendContext->m_hSock);
				InterlockedExchange(&(m_pSendContext->m_nPostCount), 0);
				bResult = FALSE;
			}
		}
		//网络IO正在进行发送操作, 将数据放入队列中
		else
		{
			EnterCriticalSection(&m_SendDataLock);

			CLIENT_SEND_DATA* pSendData = new CLIENT_SEND_DATA(szData, nLen, PeerAddr);
			if (pSendData)
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

	CLIENT_SEND_DATA* ClientNet::GetSendData()
	{
		CLIENT_SEND_DATA* pSendData = NULL;

		EnterCriticalSection(&m_SendDataLock);

		vector<CLIENT_SEND_DATA* >::iterator iterData = m_SendDataQue.begin();
		if (m_SendDataQue.end() != iterData)
		{			
			pSendData = *iterData;
			m_SendDataQue.erase(iterData);
		}
		LeaveCriticalSection(&m_SendDataLock);

		return pSendData;
	}

	BOOL ClientNet::Listen(INT nPort, BOOL bReuse, const char* szLocalIp)
	{
		BOOL bResult = TRUE;

		SOCKET sockClient = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		try
		{
			if (INVALID_SOCKET != m_hSock)	
			{
				closesocket(m_hSock);
			}

			m_hSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

			if (INVALID_SOCKET == m_hSock)
			{
				THROW_LINE;
			}

			ULONG ul = 1;
			ioctlsocket(m_hSock, FIONBIO, &ul);

			if (bReuse)
			{
				//设置为地址重用，优点在于服务器关闭后可以立即启用
				int nOpt = 1;
				setsockopt(m_hSock, SOL_SOCKET, SO_REUSEADDR, (char*)&nOpt, sizeof(nOpt));
			}

			//加载acceptex函数
			DWORD dwBytes;
			GUID guidAccept = WSAID_ACCEPTEX;
			INT nRet;

			if (NULL == s_pfAcceptEx)
			{
				nRet = WSAIoctl(m_hSock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidAccept, sizeof(guidAccept)
					, &s_pfAcceptEx, sizeof(s_pfAcceptEx), &dwBytes, NULL, NULL);
			}

			//加载GetAcceptExSockaddrs函数
			GUID guidGetAddr = WSAID_GETACCEPTEXSOCKADDRS;
			if (NULL == s_pfGetAddrs)
			{
				nRet = WSAIoctl(m_hSock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidGetAddr, sizeof(guidGetAddr)
					, &s_pfGetAddrs, sizeof(s_pfGetAddrs), &dwBytes, NULL, NULL);
			}

			if (NULL == s_pfGetAddrs || NULL == s_pfAcceptEx)
			{
				THROW_LINE;
			}

			//帮定socket
			IP_ADDR localAddr("0.0.0.0", nPort);
			if (bReuse)
			{
				localAddr = IP_ADDR(szLocalIp, nPort);
			}

			if (SOCKET_ERROR == bind(m_hSock, (sockaddr*)&localAddr, sizeof(localAddr)))
			{
				THROW_LINE;
			}

			//监听
			if (SOCKET_ERROR == listen(m_hSock, 5))
			{
				THROW_LINE;
			}

			//将其帮定到IO端口上
			if (FALSE == BindIoCompletionCallback((HANDLE)m_hSock, IOCompletionRoutine, 0))
			{
				THROW_LINE;
			}

			//为其投递accept操作
			if (NULL == m_pRcvContext)
			{
				m_pRcvContext = new CLIENT_CONTEXT(this);
			}

			if (NULL == m_pRcvContext)
			{
				THROW_LINE;
			}

			if (INVALID_SOCKET == sockClient)
			{
				THROW_LINE;
			}
			InterlockedExchange(&m_pRcvContext->m_nPostCount, 1);
			m_pRcvContext->m_ClientSock = sockClient;
			m_pRcvContext->m_hSock = m_hSock;
			m_pRcvContext->m_nOperation = OP_ACCEPT;

			nRet = s_pfAcceptEx(m_pRcvContext->m_hSock, m_pRcvContext->m_ClientSock, m_pRcvContext->m_pBuf
				, 0, sizeof(sockaddr_in) +16, sizeof(sockaddr_in) +16, &dwBytes, &(m_pRcvContext->m_ol));
			if (FALSE == nRet && ERROR_IO_PENDING != WSAGetLastError())
			{
				InterlockedExchange(&m_pSendContext->m_nPostCount, 0);
				THROW_LINE;
			}
		}
		catch (const long& iErrCode)
		{
			bResult = FALSE;
			closesocket(m_hSock);
			_TRACE("Exp : %s--%ld", __FILE__, iErrCode);

			delete m_pRcvContext;
			m_pRcvContext = NULL;

			closesocket(sockClient);
			sockClient = INVALID_SOCKET;			
		}

		return bResult;
	}

	BOOL ClientNet::Bind(INT nPort, BOOL bReuse, const char* szLocalIp)
	{
		BOOL bResult = TRUE;

		try 
		{
			if (INVALID_SOCKET != m_hSock)
			{
				closesocket(m_hSock);
			}

			ClearSendQue();

			m_hSock = WSASocket(AF_INET, SOCK_DGRAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

			ULONG ul = 1;
			ioctlsocket(m_hSock, FIONBIO, &ul);

			if (bReuse)
			{
				//设置为地址重用，优点在于服务器关闭后可以立即启用
				int nOpt = 1;
				setsockopt(m_hSock, SOL_SOCKET, SO_REUSEADDR, (char*)&nOpt, sizeof(nOpt));
			}

			//设置socket的缓存, 此处将用到系统接收缓存
			INT nBufSize = 0;
			setsockopt(m_hSock, SOL_SOCKET, SO_SNDBUF, (char*)&nBufSize, sizeof(nBufSize));
			nBufSize = 65535;
			setsockopt(m_hSock, SOL_SOCKET, SO_RCVBUF, (CHAR*)&nBufSize, sizeof(nBufSize));

			IP_ADDR addr("0.0.0.0", nPort);
			if (bReuse)
			{
				addr = IP_ADDR(szLocalIp, nPort);
			}

			if (0 != bind(m_hSock, (sockaddr*)&addr, sizeof(addr)))
			{
				THROW_LINE;
			}

			if (NULL == m_pSendContext)
			{
				m_pSendContext = new CLIENT_CONTEXT(this);
			}

			if (NULL == m_pRcvContext)
			{
				m_pRcvContext = new CLIENT_CONTEXT(this);
			}		

			if (NULL == m_pRcvContext || NULL == m_pSendContext)
			{
				THROW_LINE;
			}

			//将socket与IO端口进行帮定
			if (FALSE == BindIoCompletionCallback((HANDLE)m_hSock, IOCompletionRoutine, 0))
			{
				THROW_LINE;
			}

			//投递读操作
			if (FALSE == PostRecv(m_pRcvContext, m_hSock))
			{
				THROW_LINE;
			}
		}
		catch (const long& iErrCode)
		{
			_TRACE("Exp : %s--%ld", __FILE__, iErrCode);
			closesocket(m_hSock);
			m_hSock = INVALID_SOCKET;

			delete m_pSendContext;
			delete m_pRcvContext;
			m_pRcvContext = NULL;
			m_pSendContext = NULL;

			bResult = FALSE;
		}

		return bResult;
	}

	BOOL ClientNet::Connect(const CHAR* szHost, INT nPort, BOOL bReuse,const char* szLocalIp, int nLocalPort)
	{
		BOOL bResult = TRUE;

		try 
		{
			if (INVALID_SOCKET != m_hSock)
			{
				closesocket(m_hSock);
				m_hSock = NULL;
			}

			ClearSendQue();
			m_hSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

			ULONG ul = 1;
			ioctlsocket(m_hSock, FIONBIO, &ul);

			if (bReuse)
			{
				//设置为地址重用，优点在于服务器关闭后可以立即启用
				int nOpt = 1;
				setsockopt(m_hSock, SOL_SOCKET, SO_REUSEADDR, (char*)&nOpt, sizeof(nOpt));
			}

			//设置socket的缓存, 此处将用到系统接收缓存
			INT nBufSize = 0;
			setsockopt(m_hSock, SOL_SOCKET, SO_SNDBUF, (char*)&nBufSize, sizeof(nBufSize));
			nBufSize = 65535;
			setsockopt(m_hSock, SOL_SOCKET, SO_RCVBUF, (CHAR*)&nBufSize, sizeof(nBufSize));

			//加载connectex函数
			if (NULL == s_pfConnectEx)
			{
				GUID guidProc =  WSAID_CONNECTEX;
				DWORD dwBytes;

				WSAIoctl(m_hSock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidProc, sizeof(guidProc)
					, &s_pfConnectEx, sizeof(s_pfConnectEx), &dwBytes, NULL, NULL);
			}

			if (NULL == s_pfConnectEx)
			{
				THROW_LINE;
			}

			//先绑定到本地端口
			IP_ADDR localAddr("0.0.0.0", 0);
			if (bReuse)
			{
				localAddr = IP_ADDR(szLocalIp, nLocalPort);
			}

			if (0 != bind(m_hSock, (sockaddr*)&localAddr, sizeof(localAddr)))
			{
				THROW_LINE;
			}

			if (NULL == m_pSendContext)
			{
				m_pSendContext = new CLIENT_CONTEXT(this);
			}

			if (NULL == m_pRcvContext)
			{
				m_pRcvContext = new CLIENT_CONTEXT(this);
			}		

			if (NULL == m_pRcvContext || NULL == m_pSendContext)
			{
				THROW_LINE;
			}

			//将socket与IO端口进行帮定
			if (FALSE == BindIoCompletionCallback((HANDLE)m_hSock, IOCompletionRoutine, 0))
			{
				THROW_LINE;
			}

			//进行连接操作
			IP_ADDR serAddr(szHost, nPort);
			InterlockedExchange(&(m_pSendContext->m_nPostCount), 1);
			m_pSendContext->m_hSock = m_hSock;
			m_pSendContext->m_nOperation = OP_CONNECT;	

			INT nRet = s_pfConnectEx(m_pSendContext->m_hSock, (sockaddr*)&serAddr, sizeof(serAddr), NULL, 0, NULL, &(m_pSendContext->m_ol));
			if (FALSE == nRet && ERROR_IO_PENDING != WSAGetLastError())
			{
				InterlockedExchange(&(m_pSendContext->m_nPostCount), 0);
				THROW_LINE;
			}
		}
		catch (const long& iErrCode)
		{
			_TRACE("Exp : %s--%ld", __FILE__, iErrCode);
			closesocket(m_hSock);
			m_hSock = INVALID_SOCKET;

			delete m_pSendContext;
			delete m_pRcvContext;
			m_pRcvContext = NULL;
			m_pSendContext = NULL;

			bResult = FALSE;
		}

		return bResult;
	}

	void ClientNet::Close()
	{
		closesocket(m_hSock);
	}

	void ClientNet::ClearSendQue()
	{
		EnterCriticalSection(&m_SendDataLock);

		for (vector<CLIENT_SEND_DATA* >::iterator iterData = m_SendDataQue.begin(); m_SendDataQue.end() != iterData; iterData++)
		{
			CLIENT_SEND_DATA* pSendData = *iterData;
			delete pSendData;
			pSendData = NULL;
		}
		m_SendDataQue.clear();

		LeaveCriticalSection(&m_SendDataLock);
	}

	int ClientNet::GetSockType()
	{
		INT nSoType = 0xffffffff;
		INT nTypeLen = sizeof(nSoType);

		getsockopt(m_hSock, SOL_SOCKET, SO_TYPE, (char*)&nSoType, &nTypeLen);

		return nSoType;
	}

	void ClientNet::SetTCPCallback(
		LPVOID pParam 
		, LPONACCEPT pOnAccept 
		, LPONCONNECT pOnConnect
		, LPONREAD pOnRead
		, LPONERROR pOnError 
		)
	{
		if (c_bUseCallback)
		{
			m_pParam = pParam;
			m_pOnAccept = pOnAccept;
			m_pOnConnect = pOnConnect;
			m_pOnRead = pOnRead;
			m_pOnError = pOnError;
		}
	}

	void ClientNet::SetUDPCallback(LPVOID pParam , LPONREADFROM pOnReadFrom , LPONERROR pOnError )
	{
		if (c_bUseCallback)
		{
			m_pParam = pParam;
			m_pOnReadFrom = pOnReadFrom;
			m_pOnError = pOnError;
		}
	}

	void ClientNet::InitReource()
	{
		NET_CONTEXT::InitReource();

		//CLIENT_CONTEXT
		CLIENT_CONTEXT::s_hHeap = HeapCreate(0, CLIENT_CONTEXT::S_PAGE_SIZE, CLIENT_CONTEXT::HEAP_SIZE);
		InitializeCriticalSection(&(CLIENT_CONTEXT::s_IDLQueLock));
		CLIENT_CONTEXT::s_IDLQue.reserve(CLIENT_CONTEXT::MAX_IDL_NUM * sizeof(CLIENT_CONTEXT*));

		//CLIENT_SEND_DATA
		CLIENT_SEND_DATA::s_hHeap = HeapCreate(0, 0, CLIENT_SEND_DATA::RELAY_HEAP_SIZE);
		CLIENT_SEND_DATA::s_DataHeap = HeapCreate(0, 0, CLIENT_SEND_DATA::DATA_HEAP_SIZE);
		InitializeCriticalSection(&(CLIENT_SEND_DATA::s_IDLQueLock));
		CLIENT_SEND_DATA::s_IDLQue.reserve(CLIENT_SEND_DATA::MAX_IDL_NUM * sizeof(CLIENT_SEND_DATA*));

		//ClientNet
		WSADATA wsData;
		WSAStartup(MAKEWORD(2, 2), &wsData);

		ClientNet::s_hHeap = HeapCreate(0, ClientNet::E_BUF_SIZE, ClientNet::E_HEAP_SIZE);
		InitializeCriticalSection(&(ClientNet::s_IDLQueLock));
		ClientNet::s_IDLQue.reserve(ClientNet::E_MAX_IDL_NUM * sizeof(ClientNet*));
	}

	void ClientNet::ReleaseReource()
	{
		//CLIENT_CONTEXT
		HeapDestroy(CLIENT_CONTEXT::s_hHeap);
		CLIENT_CONTEXT::s_hHeap = NULL;

		EnterCriticalSection(&(CLIENT_CONTEXT::s_IDLQueLock));
		CLIENT_CONTEXT::s_IDLQue.clear();
		LeaveCriticalSection(&(CLIENT_CONTEXT::s_IDLQueLock));

		DeleteCriticalSection(&(CLIENT_CONTEXT::s_IDLQueLock));

		//CLIENT_SEND_DATA
		HeapDestroy(CLIENT_SEND_DATA::s_hHeap);
		CLIENT_SEND_DATA::s_hHeap = NULL;

		HeapDestroy(CLIENT_SEND_DATA::s_DataHeap);
		CLIENT_SEND_DATA::s_DataHeap = NULL;

		EnterCriticalSection(&(CLIENT_SEND_DATA::s_IDLQueLock));
		CLIENT_SEND_DATA::s_IDLQue.clear();
		LeaveCriticalSection(&(CLIENT_SEND_DATA::s_IDLQueLock));

		DeleteCriticalSection(&(CLIENT_SEND_DATA::s_IDLQueLock));

		//ClientNet
		WSACleanup();

		HeapDestroy(ClientNet::s_hHeap);
		ClientNet::s_hHeap = NULL;

		EnterCriticalSection(&(ClientNet::s_IDLQueLock));
		ClientNet::s_IDLQue.clear();
		LeaveCriticalSection(&(ClientNet::s_IDLQueLock));

		DeleteCriticalSection(&(ClientNet::s_IDLQueLock));

		NET_CONTEXT::ReleaseReource();
	}
}