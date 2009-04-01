#include "TcpClient.h"
#include "RunLog.h"

#include <assert.h>

//#define _XML_NET_
#define  _DEBUG_SQ_

namespace HelpMng
{
	//静态成员变量初始化
	//TCP_CLIENT_EX
	vector<TCP_CLIENT_EX* > TCP_CLIENT_EX::s_IDLQue;	
	CRITICAL_SECTION TCP_CLIENT_EX::s_IDLQueLock;	
	HANDLE TCP_CLIENT_EX::s_hHeap = NULL;	

	//TCP_CLIENT_RCV_EX
	HANDLE TCP_CLIENT_RCV_EX::s_DataHeap = NULL;
	HANDLE TCP_CLIENT_RCV_EX::s_hHeap = NULL;
	vector<TCP_CLIENT_RCV_EX* > TCP_CLIENT_RCV_EX::s_IDLQue;
	CRITICAL_SECTION TCP_CLIENT_RCV_EX::s_IDLQueLock;

	//TcpClient
	HANDLE TcpClient::s_hHeap = NULL;
	vector<TcpClient*> TcpClient::s_IDLQue;
	CRITICAL_SECTION TcpClient::s_IDLQueLock;
	LPFN_CONNECTEX TcpClient::s_pfConnectEx = NULL;
	BOOL TcpClient::s_bInit = FALSE;

	void *TCP_CLIENT_EX::operator new(size_t nSize)
	{
		void *pContext = NULL;

		try
		{
			if (NULL == s_hHeap)
			{
				THROW_LINE;
			}

			//申请堆内存, 先从空闲的队列中找若空闲队列为空则从堆内存上申请
			//bool bQueEmpty = true;

			EnterCriticalSection(&s_IDLQueLock);
			vector<TCP_CLIENT_EX *>::iterator iter = s_IDLQue.begin();

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

	void TCP_CLIENT_EX::operator delete(void* p)
	{
		if (p)		
		{
			//若空闲队列长度小于MAX_IDL_DATA, 则将其放入空闲队列中; 否则释放

			EnterCriticalSection(&s_IDLQueLock);
			const DWORD QUE_SIZE = (DWORD)(s_IDLQue.size());
			TCP_CLIENT_EX* const pContext = (TCP_CLIENT_EX*)p;

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

	TCP_CLIENT_RCV_EX::TCP_CLIENT_RCV_EX(const CHAR* pBuf, INT nLen)
		:m_nLen(nLen)
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

	TCP_CLIENT_RCV_EX::~TCP_CLIENT_RCV_EX()
	{
		if ((NULL != m_pData ) && (NULL != s_DataHeap))
		{
			HeapFree(s_DataHeap, 0, m_pData);
			m_pData = NULL;
		}
	}

	void* TCP_CLIENT_RCV_EX::operator new(size_t nSize)
	{
		void* pRcvData = NULL;

		try
		{
			if (NULL == s_hHeap || NULL == s_DataHeap)
			{
				THROW_LINE;
			}

			EnterCriticalSection(&s_IDLQueLock);
			vector<TCP_CLIENT_RCV_EX* >::iterator iter = s_IDLQue.begin();
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

	void TCP_CLIENT_RCV_EX::operator delete(void* p)
	{
		if (p)
		{
			EnterCriticalSection(&s_IDLQueLock);
			const DWORD QUE_SIZE = (DWORD)(s_IDLQue.size());
			TCP_CLIENT_RCV_EX* const pContext = (TCP_CLIENT_RCV_EX*)p;

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

	void TcpClient::InitReource()
	{
		if (FALSE == s_bInit)
		{
			s_bInit = TRUE;			

			//TCP_CLIENT_EX
			TCP_CLIENT_EX::s_hHeap = HeapCreate(0, TCP_CLIENT_EX::S_PAGE_SIZE, TCP_CLIENT_EX::E_TCP_HEAP_SIZE);
			InitializeCriticalSection(&(TCP_CLIENT_EX::s_IDLQueLock));
			TCP_CLIENT_EX::s_IDLQue.reserve(TCP_CLIENT_EX::MAX_IDL_DATA * sizeof(void*));

			//TCP_CLIENT_RCV_EX
			TCP_CLIENT_RCV_EX::s_hHeap = HeapCreate(0, 0, TCP_CLIENT_RCV_EX::HEAP_SIZE);
			TCP_CLIENT_RCV_EX::s_DataHeap = HeapCreate(0, 0, TCP_CLIENT_RCV_EX::DATA_HEAP_SIZE);
			InitializeCriticalSection(&(TCP_CLIENT_RCV_EX::s_IDLQueLock));
			TCP_CLIENT_RCV_EX::s_IDLQue.reserve(TCP_CLIENT_RCV_EX::MAX_IDL_DATA * sizeof(void*));

			//TcpClient
			s_hHeap = HeapCreate(0, 0, TcpClient::E_HEAP_SIZE);
			InitializeCriticalSection(&s_IDLQueLock);
			s_IDLQue.reserve(E_MAX_IDL_NUM * sizeof(void *));
		}
	}

	void TcpClient::ReleaseReource()
	{
		if (s_bInit)
		{
			s_bInit = FALSE;
			//TCP_CLIENT_EX
			HeapDestroy(TCP_CLIENT_EX::s_hHeap);
			TCP_CLIENT_EX::s_hHeap = NULL;

			EnterCriticalSection(&(TCP_CLIENT_EX::s_IDLQueLock));
			TCP_CLIENT_EX::s_IDLQue.clear();
			LeaveCriticalSection(&(TCP_CLIENT_EX::s_IDLQueLock));
			DeleteCriticalSection(&(TCP_CLIENT_EX::s_IDLQueLock));

			//TCP_CLIENT_RCV_EX
			HeapDestroy(TCP_CLIENT_RCV_EX::s_hHeap);
			TCP_CLIENT_RCV_EX::s_hHeap = NULL;

			HeapDestroy(TCP_CLIENT_RCV_EX::s_DataHeap);
			TCP_CLIENT_RCV_EX::s_DataHeap = NULL;

			EnterCriticalSection(&(TCP_CLIENT_RCV_EX::s_IDLQueLock));
			TCP_CLIENT_RCV_EX::s_IDLQue.clear();
			LeaveCriticalSection(&(TCP_CLIENT_RCV_EX::s_IDLQueLock));

			DeleteCriticalSection(&(TCP_CLIENT_RCV_EX::s_IDLQueLock));

			//TcpClient
			HeapDestroy(s_hHeap);
			s_hHeap = NULL;

			EnterCriticalSection(&s_IDLQueLock);
			s_IDLQue.clear();
			LeaveCriticalSection(&s_IDLQueLock);
			DeleteCriticalSection(&s_IDLQueLock);
		}
	}

	void *TcpClient::operator new(size_t nSize)
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
			vector<TcpClient* >::iterator iter = s_IDLQue.begin();

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

	void TcpClient::operator delete(void *p)
	{
		if (p)
		{
			EnterCriticalSection(&s_IDLQueLock);
			const DWORD QUE_SIZE = (DWORD)(s_IDLQue.size());
			TcpClient *const pContext = (TcpClient *)p;

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

	TcpClient::TcpClient( LPCLOSE_ROUTINE pCloseFun , void *pParam , LPONCONNECT pConnFun , void *pConnParam )
		: m_pCloseFun(pCloseFun)
		, m_pCloseParam(pParam)
		, m_pConnFun(pConnFun)
		, m_pConnParam(pConnParam)
		, m_bConnected(FALSE)
	{
		InitializeCriticalSection(&m_RcvQueLock);

		//为相关队列进行空间预留
		m_RcvDataQue.reserve(50 * sizeof(void*));
	}

	TcpClient::~TcpClient()
	{
		EnterCriticalSection(&m_RcvQueLock);
		for (vector<TCP_CLIENT_RCV_EX *>::iterator item_iter = m_RcvDataQue.begin(); m_RcvDataQue.end() != item_iter;)
		{
			TCP_CLIENT_RCV_EX *pRecvData = *item_iter;
			delete pRecvData;

			m_RcvDataQue.erase(item_iter);
		}
		LeaveCriticalSection(&m_RcvQueLock);
		DeleteCriticalSection(&m_RcvQueLock);

		while (m_bConnected)
		{
			Sleep(100);
		}
	}

	BOOL TcpClient::Init( const char *szIp , int nPort )
	{
		BOOL bSucc = TRUE;
		int nRet = 0;
		ULONG ul = 1;
		int nOpt = 1;
		INT nBufSize = 0;

		try
		{
			//客户端正在运行
			if (m_bConnected)
			{
				THROW_LINE;
			}

			//创建监听套接字
			m_hSock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
			if (INVALID_SOCKET == m_hSock)
			{
				THROW_LINE;
			}

			//加载ConnectEx函数
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
		
			ioctlsocket(m_hSock, FIONBIO, &ul);
		
			setsockopt(m_hSock, SOL_SOCKET, SO_REUSEADDR, (char*)&nOpt, sizeof(nOpt));

			setsockopt(m_hSock, SOL_SOCKET, SO_SNDBUF, (char*)&nBufSize, sizeof(nBufSize));
			setsockopt(m_hSock, SOL_SOCKET, SO_RCVBUF, (CHAR*)&nBufSize, sizeof(nBufSize));

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
		
			nRet = bind(m_hSock, (sockaddr*)&LocalAddr, sizeof(LocalAddr));
			if (SOCKET_ERROR == nRet)
			{
				THROW_LINE;
			}

			//绑定到完成端口
			CreateIoCompletionPort((HANDLE)m_hSock, s_hCompletion, (ULONG_PTR)this, 0);
		}
		catch (const long &lErrLine)
		{
			bSucc = FALSE;
			_TRACE("Exp : %s -- %ld", __FILE__, lErrLine);
		}

		return bSucc;
	}

	void TcpClient::Close()
	{		
		closesocket(m_hSock);

		EnterCriticalSection(&m_RcvQueLock);
		for (vector<TCP_CLIENT_RCV_EX *>::iterator item_iter = m_RcvDataQue.begin(); m_RcvDataQue.end() != item_iter;)
		{
			TCP_CLIENT_RCV_EX *pRecvData = *item_iter;
			delete pRecvData;

			m_RcvDataQue.erase(item_iter);
		}
		LeaveCriticalSection(&m_RcvQueLock);
	}

	BOOL TcpClient::SendData(const char * szData, int nDataLen, SOCKET hSock /* = INVALID_SOCKET */)
	{
#ifdef _XML_NET_
		//数据长度非法
		if (((DWORD)nDataLen > TCP_CLIENT_EX::S_PAGE_SIZE) || (NULL == szData))
		{
			return FALSE;
		}
#else
		//数据长度非法
		if ((nDataLen > (int)(TCP_CLIENT_EX::S_PAGE_SIZE)) || (NULL == szData) || (nDataLen < sizeof(PACKET_HEAD)))
		{
			return FALSE;
		}
#endif	//#ifdef _XML_NET_

		BOOL bResult = TRUE;
		DWORD dwBytes = 0;
		WSABUF SendBuf;
		TCP_CLIENT_EX *pSendContext = new TCP_CLIENT_EX();
		if (pSendContext && pSendContext->m_pBuf)
		{
			pSendContext->m_hSock = m_hSock;
			pSendContext->m_nDataLen = nDataLen;
			pSendContext->m_nOperation = OP_WRITE;
			memcpy(pSendContext->m_pBuf, szData, nDataLen);

			SendBuf.buf = pSendContext->m_pBuf;
			SendBuf.len = nDataLen;

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

	void *TcpClient::GetRecvData(DWORD *const pQueLen)
	{
		TCP_CLIENT_RCV_EX *pRcvData = NULL;

		EnterCriticalSection(&m_RcvQueLock);
		vector<TCP_CLIENT_RCV_EX *>::iterator iter = m_RcvDataQue.begin();
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

	BOOL TcpClient::Connect( const char *szIp , int nPort , const char *szData /* = NULL  */, int nLen /* = 0 */ )
	{
		BOOL bSucc = TRUE;
		int nRet = 0;
		DWORD nBytes = 0;
		TCP_CLIENT_EX *pContext = NULL;

		try
		{
			//客户端已连接, 必须先关闭连接
			if (m_bConnected)
			{
				THROW_LINE;
			}

			IP_ADDR ser_addr(szIp, nPort);
			pContext = new TCP_CLIENT_EX();
			if (NULL == pContext || NULL == pContext->m_pBuf)
			{
				THROW_LINE;
			}

			pContext->m_hSock = m_hSock;
			pContext->m_nDataLen = 0;
			pContext->m_nOperation = OP_CONNECT;
			if (szData)
			{
				memcpy(pContext->m_pBuf, szData, nLen);

				nRet = s_pfConnectEx(pContext->m_hSock, (sockaddr*)&ser_addr, sizeof(ser_addr), pContext->m_pBuf, nLen, &nBytes, &(pContext->m_ol));
				if (FALSE == nRet && ERROR_IO_PENDING != WSAGetLastError())
				{
					THROW_LINE;;
				}
			}
			else
			{
				nRet = s_pfConnectEx(pContext->m_hSock, (sockaddr*)&ser_addr, sizeof(ser_addr), NULL, 0, NULL, &(pContext->m_ol));
				if (FALSE == nRet && ERROR_IO_PENDING != WSAGetLastError())
				{
					THROW_LINE;;
				}
			}
		}
		catch(const long &lErrLine)
		{
			delete pContext;
			bSucc= FALSE;
			_TRACE("Exp : %s -- %ld", __FILE__, lErrLine);
		}

		return bSucc;
	}

	void TcpClient::IOCompletionProc(BOOL bSuccess, DWORD dwTrans, LPOVERLAPPED lpOverlapped)
	{
		TCP_CLIENT_EX * pContext = CONTAINING_RECORD(lpOverlapped, TCP_CLIENT_EX, m_ol);

		if (pContext)
		{
			switch (pContext->m_nOperation)
			{
			case OP_CONNECT:
				OnConnect(bSuccess, dwTrans, lpOverlapped);
				break;
			case OP_READ:
				OnRead(bSuccess, dwTrans, lpOverlapped);
				break;
			case OP_WRITE:

#ifdef _DEBUG_SQ_
				if (pContext->m_nDataLen != dwTrans)
				{
					_TRACE("Exp : %s -- %ld 数据未发送完成 DATA_LEN = %ld SEND_LEN = %ld", __FILE__, __LINE__, pContext->m_nDataLen, dwTrans);
					assert(0);
				}
#endif

				delete pContext;
				pContext = NULL;
				break;
			}
		}
	}

	void TcpClient::OnConnect(BOOL bSuccess, DWORD dwTrans, LPOVERLAPPED lpOverlapped)
	{
		TCP_CLIENT_EX *pContext = CONTAINING_RECORD(lpOverlapped, TCP_CLIENT_EX, m_ol);
		DWORD dwFlag = 0;
		DWORD dwBytes = 0;
		WSABUF RcvBuf;
		int nErrCode = 0;

		try
		{
			if ( FALSE == bSuccess)
			{
				closesocket(m_hSock);				
				m_pCloseFun(m_pCloseParam, m_hSock, OP_CONNECT);
				THROW_LINE;
			}
			else
			{
				m_bConnected = TRUE;

				//投递recv操作
				pContext->m_hSock = m_hSock;
				pContext->m_nOperation = OP_READ;
				pContext->m_nDataLen = 0;
				RcvBuf.buf = pContext->m_pBuf;
				RcvBuf.len = TCP_CLIENT_EX::S_PAGE_SIZE;

				nErrCode = WSARecv(m_hSock, &RcvBuf, 1, &dwBytes, &dwFlag, &(pContext->m_ol), NULL);
				if (SOCKET_ERROR == nErrCode && WSA_IO_PENDING != WSAGetLastError())
				{
					closesocket(m_hSock);
					m_pCloseFun(m_pCloseParam, m_hSock, OP_CONNECT);
					THROW_LINE;
				}

				m_pConnFun(m_pConnParam);				
			}
		}
		catch (const long &lErrLine)
		{
			delete pContext;
			pContext = NULL;
			m_bConnected = FALSE;
			_TRACE("Exp : %s -- %ld SOCKET = 0x%x, ERR_CODE = 0x%x", __FILE__, lErrLine, m_hSock, WSAGetLastError());
		}
	}

	void TcpClient::OnRead(BOOL bSuccess, DWORD dwTrans, LPOVERLAPPED lpOverlapped)
	{
		TCP_CLIENT_EX * pRcvContext = CONTAINING_RECORD(lpOverlapped, TCP_CLIENT_EX, m_ol);
		DWORD dwFlag = 0;
		DWORD dwBytes = 0;
		WSABUF RcvBuf;
		int nErrCode = 0;
		int nOffSet = 0;

		try
		{
			if ( FALSE == bSuccess || 0 == dwTrans)
			{
				closesocket(pRcvContext->m_hSock);
				THROW_LINE;;
			}

#ifdef _XML_NET_	//处理XML流
			TCP_CLIENT_RCV_EX* pRcvData = new TCP_CLIENT_RCV_EX(
				pRcvContext->m_pBuf
				, dwTrans
				);

			if (pRcvData && pRcvData->m_pData)
			{					
				EnterCriticalSection(&m_RcvQueLock);
				m_RcvDataQue.push_back(pRcvData);
				LeaveCriticalSection(&m_RcvQueLock);
			}

			pRcvContext->m_nDataLen = 0;
			RcvBuf.buf = pRcvContext->m_pBuf;
			RcvBuf.len = TCP_CLIENT_EX::S_PAGE_SIZE;

#else				//处理二进制数据流

			//当前已接收的数据包总长
			pRcvContext->m_nDataLen += dwTrans;		
			while (true)
			{
				if (pRcvContext->m_nDataLen >= sizeof(PACKET_HEAD))
				{
					PACKET_HEAD *pHeadInfo = (PACKET_HEAD *)(pRcvContext->m_pBuf + nOffSet);
					//数据长度合法才进行处理
					if (pHeadInfo->nCurrentLen + sizeof(PACKET_HEAD) <= TCP_CLIENT_EX::S_PAGE_SIZE )
					{
						//已接收完一个或多个数据包
						if (pHeadInfo->nCurrentLen + sizeof(PACKET_HEAD) <= pRcvContext->m_nDataLen)
						{
							TCP_CLIENT_RCV_EX *pRcvData = new TCP_CLIENT_RCV_EX(
								pRcvContext->m_pBuf + nOffSet
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
							RcvBuf.len = TCP_CLIENT_EX::S_PAGE_SIZE - pRcvContext->m_nDataLen;
							break;
						}
					}
					//数据非法, 则不处理数据直接进行下一次读投递
					else
					{
						pRcvContext->m_nDataLen = 0;
						RcvBuf.buf = pRcvContext->m_pBuf;
						RcvBuf.len = TCP_CLIENT_EX::S_PAGE_SIZE;
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
					RcvBuf.len = TCP_CLIENT_EX::S_PAGE_SIZE - pRcvContext->m_nDataLen;
					break;			
				}
			}
#endif	//#ifdef _XML_NET_

			//继续投递读操作
			//DWORD dwBytes = 0;
			nErrCode = WSARecv(pRcvContext->m_hSock, &RcvBuf, 1, &dwBytes, &dwFlag, &(pRcvContext->m_ol), NULL);
			//Recv操作错误
			if (SOCKET_ERROR == nErrCode && WSA_IO_PENDING != WSAGetLastError())
			{
				//closesocket(pRcvContext->m_hSock);
				THROW_LINE;;
			}			
		}
		catch (const long &lErrLine)
		{
			_TRACE("Exp : %s -- %ld SOCKET = 0x%x ERR_CODE = 0x%x", __FILE__, lErrLine, pRcvContext->m_hSock, WSAGetLastError());
			m_pCloseFun(m_pCloseParam, m_hSock, OP_READ);
			m_bConnected = FALSE;
			delete pRcvContext;			
		}
	}

	void ReleaseTcpClient()
	{
		TcpClient::ReleaseReource();
	}
}