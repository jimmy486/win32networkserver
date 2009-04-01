#include "UdpSerEx.h"
#include <assert.h>
#include <process.h>
#include <atltrace.h>

#include "RunLog.h"

namespace HelpMng
{
	vector<UDP_CONTEXT_EX* > UDP_CONTEXT_EX::s_IDLQue;
	CRITICAL_SECTION UDP_CONTEXT_EX::s_IDLQueLock;
	HANDLE UDP_CONTEXT_EX::s_hHeap = NULL;

	HANDLE UDP_RCV_DATA_EX::s_DataHeap = NULL;
	HANDLE UDP_RCV_DATA_EX::s_Heap = NULL;
	vector<UDP_RCV_DATA_EX* > UDP_RCV_DATA_EX::s_IDLQue;
	CRITICAL_SECTION UDP_RCV_DATA_EX::s_IDLQueLock;

	HANDLE UdpSerEx::s_hHeap = NULL;
	vector<UdpSerEx*> UdpSerEx:: s_IDLQue;
	CRITICAL_SECTION UdpSerEx::s_IDLQueLock;
	BOOL UdpSerEx::s_bInit = FALSE;

	//UDP_CONTEXT_EX的实现
	void* UDP_CONTEXT_EX:: operator new(size_t nSize)
	{
		void* pContext = NULL;

		try
		{
			EnterCriticalSection(&s_IDLQueLock);
			if (NULL == s_hHeap)
			{
				THROW_LINE;
			}

			//bool bQueEmpty = true;			
			vector<UDP_CONTEXT_EX* >::iterator iter = s_IDLQue.begin();
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

			if (NULL == pContext)
			{
				THROW_LINE;;
			}
			LeaveCriticalSection(&s_IDLQueLock);
		}
		catch (const long& iErrCode)
		{
			pContext = NULL;
			LeaveCriticalSection(&s_IDLQueLock);
			_TRACE("Exp : %s--%ld", __FILE__, iErrCode);
		}

		return pContext;
	}

	void UDP_CONTEXT_EX:: operator delete(void* p)
	{
		if (p)
		{
			EnterCriticalSection(&s_IDLQueLock);
			const DWORD QUE_SIZE = (DWORD)(s_IDLQue.size());
			UDP_CONTEXT_EX* const pContext = (UDP_CONTEXT_EX*)p;

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

	//RCV_DATA的实现
	UDP_RCV_DATA_EX::UDP_RCV_DATA_EX(const CHAR* szBuf, int nLen, const IP_ADDR& PeerAddr)
		: m_PeerAddr(PeerAddr)
		, m_nLen(nLen)
	{
		try
		{
			//assert(szBuf);			
			if (NULL == s_DataHeap)
			{
				THROW_LINE;;
			}

			m_pData = (CHAR*)HeapAlloc(s_DataHeap, HEAP_ZERO_MEMORY, nLen);
			//assert(m_pData);
			if (m_pData)
			{
				memcpy(m_pData, szBuf, nLen);
			}
		}
		catch (const long& iErrCode)
		{
			m_pData = NULL;
			_TRACE("Exp : %s--%ld", __FILE__, iErrCode);
		}
	}

	UDP_RCV_DATA_EX::~UDP_RCV_DATA_EX()
	{
		if (m_pData)
		{
			HeapFree(s_DataHeap, 0, m_pData);
			m_pData = NULL;
		}
	}

	void* UDP_RCV_DATA_EX::operator new(size_t nSize)
	{
		void* pRcvData = NULL;

		try
		{
			EnterCriticalSection(&s_IDLQueLock);
			if (NULL == s_Heap || NULL == s_DataHeap)
			{
				THROW_LINE;
			}

			//bool bQueEmpty = true;			
			vector<UDP_RCV_DATA_EX* >::iterator iter = s_IDLQue.begin();
			if (s_IDLQue.end() != iter)
			{
				pRcvData = *iter;
				s_IDLQue.erase(iter);
				//bQueEmpty = false;
			}
			else
			{
				pRcvData = HeapAlloc(s_Heap, HEAP_ZERO_MEMORY | HEAP_NO_SERIALIZE, nSize);
			}			

			if (NULL == pRcvData)
			{
				THROW_LINE;;
			}

			LeaveCriticalSection(&s_IDLQueLock);
		}
		catch (const long& iErrCode)
		{
			_TRACE("Exp : %s--%ld", __FILE__, iErrCode);
			pRcvData = NULL;
			LeaveCriticalSection(&s_IDLQueLock);
		}

		return pRcvData;
	}

	void UDP_RCV_DATA_EX::operator delete (void* p)
	{
		if (p)
		{
			EnterCriticalSection(&s_IDLQueLock);
			const DWORD QUE_SIZE = (DWORD)(s_IDLQue.size());
			UDP_RCV_DATA_EX* const pContext = (UDP_RCV_DATA_EX*)p;

			if (QUE_SIZE <= MAX_IDL_DATA)
			{
				s_IDLQue.push_back(pContext);
			}
			else
			{
				HeapFree(s_Heap, HEAP_NO_SERIALIZE, p);
			}
			LeaveCriticalSection(&s_IDLQueLock);

		}

		return;
	}

	//UdpSerEx的实现
	void UdpSerEx::InitReource()
	{
		if (FALSE == s_bInit)
		{
			s_bInit = TRUE;

			//UDP_CONTEXT_EX
			UDP_CONTEXT_EX::s_hHeap = HeapCreate(0, UDP_CONTEXT_EX::S_PAGE_SIZE, UDP_CONTEXT_EX::HEAP_SIZE);
			InitializeCriticalSection(&(UDP_CONTEXT_EX::s_IDLQueLock));
			UDP_CONTEXT_EX::s_IDLQue.reserve(UDP_CONTEXT_EX::MAX_IDL_DATA *sizeof(UDP_CONTEXT_EX*));

			//UDP_RCV_DATA_EX
			UDP_RCV_DATA_EX::s_Heap = HeapCreate(0, 0, UDP_RCV_DATA_EX::RCV_HEAP_SIZE);
			UDP_RCV_DATA_EX::s_DataHeap = HeapCreate(0, 0, UDP_RCV_DATA_EX::DATA_HEAP_SIZE);
			InitializeCriticalSection(&(UDP_RCV_DATA_EX::s_IDLQueLock));
			UDP_RCV_DATA_EX::s_IDLQue.reserve(UDP_RCV_DATA_EX::MAX_IDL_DATA * sizeof(UDP_RCV_DATA_EX*));

			//UdpSerEx
			UdpSerEx::s_hHeap = HeapCreate(0, UdpSerEx::E_BUF_SIZE, UdpSerEx::E_HEAP_SIZE);
			InitializeCriticalSection(&(UdpSerEx::s_IDLQueLock));
			UdpSerEx::s_IDLQue.reserve(UdpSerEx::E_MAX_IDL_NUM * sizeof(UdpSerEx*));
		}
	}

	void UdpSerEx::ReleaseReource()
	{
		if (s_bInit)
		{
			s_bInit = FALSE;

			//UDP_CONTEXT_EX
			HeapDestroy(UDP_CONTEXT_EX::s_hHeap);
			UDP_CONTEXT_EX::s_hHeap = NULL;

			EnterCriticalSection(&(UDP_CONTEXT_EX::s_IDLQueLock));
			UDP_CONTEXT_EX::s_IDLQue.clear();
			LeaveCriticalSection(&(UDP_CONTEXT_EX::s_IDLQueLock));
			DeleteCriticalSection(&(UDP_CONTEXT_EX::s_IDLQueLock));

			//UDP_RCV_DATA_EX
			HeapDestroy(UDP_RCV_DATA_EX::s_Heap);
			UDP_RCV_DATA_EX::s_Heap = NULL;

			HeapDestroy(UDP_RCV_DATA_EX::s_DataHeap);
			UDP_RCV_DATA_EX::s_DataHeap = NULL;

			EnterCriticalSection(&(UDP_RCV_DATA_EX::s_IDLQueLock));
			UDP_RCV_DATA_EX::s_IDLQue.clear();
			LeaveCriticalSection(&(UDP_RCV_DATA_EX::s_IDLQueLock));
			DeleteCriticalSection(&(UDP_RCV_DATA_EX::s_IDLQueLock));

			//UdpSerEx
			HeapDestroy(UdpSerEx::s_hHeap);
			UdpSerEx::s_hHeap = NULL;

			EnterCriticalSection(&(UdpSerEx::s_IDLQueLock));
			UdpSerEx::s_IDLQue.clear();
			LeaveCriticalSection(&(UdpSerEx::s_IDLQueLock));

			DeleteCriticalSection(&(UdpSerEx::s_IDLQueLock));
		}
	}

	UdpSerEx::UdpSerEx( LPCLOSE_ROUTINE pCloseFun , void *pParam, BOOL bClient)
		: m_bSerRun(FALSE)
		, m_pCloseProc(pCloseFun)
		, m_pCloseParam(pParam)
		, m_lReadCount(0)
		, c_bClient(bClient)
	{
		InitializeCriticalSection(&m_RcvDataLock);

		if (bClient)
		{
			//为相关队列进行空间预留
			m_RcvDataQue.reserve(50 * sizeof(void*));
		}
		else
		{
			//为相关队列进行空间预留
			m_RcvDataQue.reserve(50000 * sizeof(void*));
		}	
	}

	UdpSerEx::~UdpSerEx()
	{
		EnterCriticalSection(&m_RcvDataLock);
		for (vector<UDP_RCV_DATA_EX* >::iterator item_iter = m_RcvDataQue.begin(); m_RcvDataQue.end() != item_iter;)
		{
			UDP_RCV_DATA_EX *pRecvData = *item_iter;
			delete pRecvData;

			m_RcvDataQue.erase(item_iter);
		}		
		LeaveCriticalSection(&m_RcvDataLock);

		DeleteCriticalSection(&m_RcvDataLock);

		//必须等待所有的recv操作完成才能退出
		while(InterlockedExchangeAdd(&m_lReadCount, 0))
		{
			Sleep(100);
		}
	}

	void *UdpSerEx::operator new(size_t nSize)
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
			vector<UdpSerEx* >::iterator iter = s_IDLQue.begin();

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

	void UdpSerEx::operator delete(void *p)
	{
		if (p)
		{
			EnterCriticalSection(&s_IDLQueLock);
			const DWORD QUE_SIZE = (DWORD)(s_IDLQue.size());
			UdpSerEx* const pContext = (UdpSerEx*)p;

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

	BOOL UdpSerEx::Init( const char *szIp , int nPort )
	{
		BOOL bRet = TRUE;
		WSABUF RcvBuf = { NULL, 0 };
		DWORD dwBytes = 0;
		DWORD dwFlag = 0;
		INT nAddrLen = sizeof(IP_ADDR);
		INT iErrCode = 0;
		int RECV_COUNT = 0;

		try
		{
			if (m_bSerRun || InterlockedExchangeAdd(&m_lReadCount, 0))
			{
				THROW_LINE;;
			}

			m_bSerRun = TRUE;
			
			m_hSock = WSASocket(AF_INET, SOCK_DGRAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			if (INVALID_SOCKET == m_hSock)
			{
				THROW_LINE;;
			}
			ULONG ul = 1;
			ioctlsocket(m_hSock, FIONBIO, &ul);

			//设置为地址重用，优点在于服务器关闭后可以立即启用
			int nOpt = 1;
			setsockopt(m_hSock, SOL_SOCKET, SO_REUSEADDR, (char*)&nOpt, sizeof(nOpt));

			//关闭系统缓存，使用自己的缓存以防止数据的复制操作
			INT nZero = 0;
			setsockopt(m_hSock, SOL_SOCKET, SO_SNDBUF, (char*)&nZero, sizeof(nZero));
			setsockopt(m_hSock, SOL_SOCKET, SO_RCVBUF, (CHAR*)&nZero, sizeof(nZero));

			IP_ADDR addr("0.0.0.0", nPort);
			if (szIp)
			{
				addr = IP_ADDR(szIp, nPort);
			}

			if (SOCKET_ERROR == bind(m_hSock, (sockaddr*)&addr, sizeof(addr)))
			{
				closesocket(m_hSock);
				THROW_LINE;
			}

			//将SOCKET绑定到完成端口上
			CreateIoCompletionPort((HANDLE)m_hSock, s_hCompletion, (ULONG_PTR)this, 0);

			if (c_bClient)
			{
				RECV_COUNT = 5;
			}
			else
			{
				RECV_COUNT = 500;
			}

			//投递读操作
			for (int nIndex = 0; nIndex < RECV_COUNT; nIndex++)
			{
				UDP_CONTEXT_EX* pRcvContext = new UDP_CONTEXT_EX();
				if (pRcvContext && pRcvContext->m_pBuf)
				{
					dwFlag = 0;
					dwBytes = 0;
					nAddrLen = sizeof(IP_ADDR);
					RcvBuf.buf = pRcvContext->m_pBuf;
					RcvBuf.len = UDP_CONTEXT_EX::S_PAGE_SIZE;

					pRcvContext->m_hSock = m_hSock;
					pRcvContext->m_nOperation = OP_READ;			
					iErrCode = WSARecvFrom(pRcvContext->m_hSock, &RcvBuf, 1, &dwBytes, &dwFlag, (sockaddr*)(&pRcvContext->m_RemoteAddr)
						, &nAddrLen, &(pRcvContext->m_ol), NULL);
					if (SOCKET_ERROR == iErrCode && ERROR_IO_PENDING != WSAGetLastError())
					{
						delete pRcvContext;
						pRcvContext = NULL;
						closesocket(m_hSock);
						THROW_LINE;
					}
					else
					{
						InterlockedExchangeAdd(&m_lReadCount, 1);
					}
				}
				else
				{
					delete pRcvContext;
				}
			}
		}
		catch (const long &lErrLine)
		{			
			bRet = FALSE;
			_TRACE("Exp : %s -- %ld SOCKET = 0x%x, ERR_CODE = %ld", __FILE__, lErrLine, m_hSock, WSAGetLastError());			
		}

		return bRet;
	}

	void UdpSerEx::Close()
	{
		m_bSerRun = FALSE;
		closesocket(m_hSock);

		EnterCriticalSection(&m_RcvDataLock);
		for (vector<UDP_RCV_DATA_EX* >::iterator item_iter = m_RcvDataQue.begin(); m_RcvDataQue.end() != item_iter;)
		{
			UDP_RCV_DATA_EX *pRecvData = *item_iter;
			delete pRecvData;

			m_RcvDataQue.erase(item_iter);
		}		
		LeaveCriticalSection(&m_RcvDataLock);
	}

	BOOL UdpSerEx::SendData(const IP_ADDR& PeerAddr, const char * szData, int nLen)
	{
		BOOL bRet = TRUE;
		try
		{
			//if (nLen >= 1500)
			//{
			//	THROW_LINE;;
			//}

			UDP_CONTEXT_EX *pSendContext = new UDP_CONTEXT_EX();
			if (pSendContext && pSendContext->m_pBuf)
			{
				pSendContext->m_nOperation = OP_WRITE;
				pSendContext->m_RemoteAddr = PeerAddr;		

				memcpy(pSendContext->m_pBuf, szData, nLen);

				WSABUF SendBuf = { NULL, 0 };
				DWORD dwBytes = 0;
				SendBuf.buf = pSendContext->m_pBuf;
				SendBuf.len = nLen;

				INT iErrCode = WSASendTo(m_hSock, &SendBuf, 1, &dwBytes, 0, (sockaddr*)&PeerAddr, sizeof(PeerAddr), &(pSendContext->m_ol), NULL);
				if (SOCKET_ERROR == iErrCode && ERROR_IO_PENDING != WSAGetLastError())
				{
					delete pSendContext;
					THROW_LINE;;
				}
			}
			else
			{
				delete pSendContext;
				THROW_LINE;;
			}
		}
		catch (const long &lErrLine)
		{
			bRet = FALSE;
			ATLTRACE("\r\nExp : %s -- %ld ", __FILE__, lErrLine);			
		}

		return bRet;
	}

	void *UdpSerEx::GetRecvData(DWORD* const pQueLen)
	{
		UDP_RCV_DATA_EX* pRcvData = NULL;

		EnterCriticalSection(&m_RcvDataLock);
		vector<UDP_RCV_DATA_EX* >::iterator iterRcv = m_RcvDataQue.begin();
		if (iterRcv != m_RcvDataQue.end())
		{
			pRcvData = *iterRcv;
			m_RcvDataQue.erase(iterRcv);
		}

		if (pQueLen)
		{
			*pQueLen = (DWORD)(m_RcvDataQue.size());
		}
		LeaveCriticalSection(&m_RcvDataLock);

		return pRcvData;
	}

	void UdpSerEx::ReadCompletion(BOOL bSuccess, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		UDP_CONTEXT_EX* pRcvContext = CONTAINING_RECORD(lpOverlapped, UDP_CONTEXT_EX, m_ol);
		WSABUF RcvBuf = { NULL, 0 };
		DWORD dwBytes = 0;
		DWORD dwFlag = 0;
		INT nAddrLen = sizeof(IP_ADDR);
		INT iErrCode = 0;

		try
		{
			if ( FALSE == bSuccess || 0 == dwNumberOfBytesTransfered )
			{
				THROW_LINE;;
			}

#ifdef _XML_NET_
			EnterCriticalSection(&m_RcvDataLock);

			UDP_RCV_DATA_EX* pRcvData = new UDP_RCV_DATA_EX(pRcvContext->m_pBuf, dwNumberOfBytesTransfered, pRcvContext->m_RemoteAddr);
			if (pRcvData && pRcvData->m_pData)
			{
				m_RcvDataQue.push_back(pRcvData);
			}	
			else
			{
				delete pRcvData;
			}

			LeaveCriticalSection(&m_RcvDataLock);
#else
			if (dwNumberOfBytesTransfered >= sizeof(PACKET_HEAD))
			{
				EnterCriticalSection(&m_RcvDataLock);

				UDP_RCV_DATA_EX* pRcvData = new UDP_RCV_DATA_EX(pRcvContext->m_pBuf, dwNumberOfBytesTransfered, pRcvContext->m_RemoteAddr);
				if (pRcvData && pRcvData->m_pData)
				{
					m_RcvDataQue.push_back(pRcvData);
				}	
				else
				{
					delete pRcvData;
				}

				LeaveCriticalSection(&m_RcvDataLock);
			}
#endif

			//投递下一个接收操作
			RcvBuf.buf = pRcvContext->m_pBuf;
			RcvBuf.len = UDP_CONTEXT_EX::S_PAGE_SIZE;

			iErrCode = WSARecvFrom(pRcvContext->m_hSock, &RcvBuf, 1, &dwBytes, &dwFlag, (sockaddr*)(&pRcvContext->m_RemoteAddr)
				, &nAddrLen, &(pRcvContext->m_ol), NULL);
			if (SOCKET_ERROR == iErrCode && ERROR_IO_PENDING != WSAGetLastError())
			{
				ATLTRACE("\r\n%s -- %ld dwNumberOfBytesTransfered = %ld, LAST_ERR = %ld"
					, __FILE__, __LINE__, dwNumberOfBytesTransfered, WSAGetLastError());

				THROW_LINE;
			}			
		}
		catch (const long &lErrLine)
		{
			if (1 == InterlockedExchangeAdd(&m_lReadCount, -1))
			{
				_TRACE("Exp : %s -- %ld  SOCKET = 0x%x", __FILE__, lErrLine, m_hSock);
				m_pCloseProc(m_pCloseParam, pRcvContext->m_hSock, OP_READ);
			}			

			delete pRcvContext;
		}
	}

	void UdpSerEx::IOCompletionProc(BOOL bSuccess, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		UDP_CONTEXT_EX *pContext = CONTAINING_RECORD(lpOverlapped, UDP_CONTEXT_EX, m_ol);

		if (pContext)
		{
			switch (pContext->m_nOperation)
			{
			case OP_READ:
				ReadCompletion(bSuccess, dwNumberOfBytesTransfered, lpOverlapped);
				break;

			case OP_WRITE:
				delete pContext;
				pContext = NULL;
				break;
			}
		}
	}

	void ReleaseUdpSerEx()
	{
		UdpSerEx::ReleaseReource();
	}
}