#include "UdpSer.h"
#include <assert.h>
#include <process.h>
#include <atltrace.h>

#include "RunLog.h"

namespace HelpMng
{
	vector<UDP_CONTEXT* > UDP_CONTEXT::s_IDLQue;
	CRITICAL_SECTION UDP_CONTEXT::s_IDLQueLock;
	HANDLE UDP_CONTEXT::s_hHeap = NULL;

	HANDLE UDP_RCV_DATA::s_DataHeap = NULL;
	HANDLE UDP_RCV_DATA::s_Heap = NULL;
	vector<UDP_RCV_DATA* > UDP_RCV_DATA::s_IDLQue;
	CRITICAL_SECTION UDP_RCV_DATA::s_IDLQueLock;

	//UDP_CONTEXT的实现
	void* UDP_CONTEXT:: operator new(size_t nSize)
	{
		void* pContext = NULL;

		try
		{
			EnterCriticalSection(&s_IDLQueLock);
			if (NULL == s_hHeap)
			{
				THROW_LINE;;
			}

			//bool bQueEmpty = true;			
			vector<UDP_CONTEXT* >::iterator iter = s_IDLQue.begin();
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

	void UDP_CONTEXT:: operator delete(void* p)
	{
		if (p)
		{
			EnterCriticalSection(&s_IDLQueLock);
			const DWORD QUE_SIZE = (DWORD)(s_IDLQue.size());
			UDP_CONTEXT* const pContext = (UDP_CONTEXT*)p;

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
	UDP_RCV_DATA::UDP_RCV_DATA(const CHAR* szBuf, int nLen, const IP_ADDR& PeerAddr)
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

	UDP_RCV_DATA::~UDP_RCV_DATA()
	{
		if (m_pData)
		{
			HeapFree(s_DataHeap, 0, m_pData);
			m_pData = NULL;
		}
	}

	void* UDP_RCV_DATA::operator new(size_t nSize)
	{
		void* pRcvData = NULL;

		try
		{
			EnterCriticalSection(&s_IDLQueLock);
			if (NULL == s_Heap || NULL == s_DataHeap)
			{
				THROW_LINE;;
			}

			//bool bQueEmpty = true;			
			vector<UDP_RCV_DATA* >::iterator iter = s_IDLQue.begin();
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

	void UDP_RCV_DATA::operator delete (void* p)
	{
		if (p)
		{
			EnterCriticalSection(&s_IDLQueLock);
			const DWORD QUE_SIZE = (DWORD)(s_IDLQue.size());
			UDP_RCV_DATA* const pContext = (UDP_RCV_DATA*)p;

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

	//UdpSer的实现
	void UdpSer::InitReource()
	{
		WSADATA wsData;
		WSAStartup(MAKEWORD(2, 2), &wsData);

		NET_CONTEXT::InitReource();

		//UDP_CONTEXT
		UDP_CONTEXT::s_hHeap = HeapCreate(0, UDP_CONTEXT::S_PAGE_SIZE, UDP_CONTEXT::HEAP_SIZE);
		InitializeCriticalSection(&(UDP_CONTEXT::s_IDLQueLock));
		UDP_CONTEXT::s_IDLQue.reserve(UDP_CONTEXT::MAX_IDL_DATA *sizeof(UDP_CONTEXT*));

		//UDP_RCV_DATA
		UDP_RCV_DATA::s_Heap = HeapCreate(0, 0, UDP_RCV_DATA::RCV_HEAP_SIZE);
		UDP_RCV_DATA::s_DataHeap = HeapCreate(0, 0, UDP_RCV_DATA::DATA_HEAP_SIZE);
		InitializeCriticalSection(&(UDP_RCV_DATA::s_IDLQueLock));
		UDP_RCV_DATA::s_IDLQue.reserve(UDP_RCV_DATA::MAX_IDL_DATA * sizeof(UDP_RCV_DATA*));
	}

	void UdpSer::ReleaseReource()
	{
		//UDP_CONTEXT
		HeapDestroy(UDP_CONTEXT::s_hHeap);
		UDP_CONTEXT::s_hHeap = NULL;

		EnterCriticalSection(&(UDP_CONTEXT::s_IDLQueLock));
		UDP_CONTEXT::s_IDLQue.clear();
		LeaveCriticalSection(&(UDP_CONTEXT::s_IDLQueLock));
		DeleteCriticalSection(&(UDP_CONTEXT::s_IDLQueLock));

		//UDP_RCV_DATA
		HeapDestroy(UDP_RCV_DATA::s_Heap);
		UDP_RCV_DATA::s_Heap = NULL;

		HeapDestroy(UDP_RCV_DATA::s_DataHeap);
		UDP_RCV_DATA::s_DataHeap = NULL;

		EnterCriticalSection(&(UDP_RCV_DATA::s_IDLQueLock));
		UDP_RCV_DATA::s_IDLQue.clear();
		LeaveCriticalSection(&(UDP_RCV_DATA::s_IDLQueLock));
		DeleteCriticalSection(&(UDP_RCV_DATA::s_IDLQueLock));

		NET_CONTEXT::ReleaseReource();
		WSACleanup();
	}

	UdpSer::UdpSer()
		: m_hSock(INVALID_SOCKET)
		, m_bThreadRun(TRUE)
		, m_bSerRun(FALSE)
	{
		InitializeCriticalSection(&m_RcvDataLock);

		//为相关队列进行空间预留
		m_RcvDataQue.reserve(50000 * sizeof(void*));

		//创建完成端口
        m_hCompletion = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

		//创建工作者线程, 工作线程的数目为CPU * 2 + 2
		SYSTEM_INFO sys_info;
		GetSystemInfo(&sys_info);

		const DWORD MAX_THREAD = sys_info.dwNumberOfProcessors * 2 +2;
		m_pThreads = new HANDLE[MAX_THREAD];
		assert(m_pThreads);

		for (DWORD nIndex = 0; nIndex < MAX_THREAD; nIndex++)
		{
			m_pThreads[nIndex] = (HANDLE)_beginthreadex(NULL, 0, WorkThread, this, 0, NULL);
		}		
	}

	UdpSer::~UdpSer()
	{
		//必须等待所有的后台线程退出后才能退出		
		SYSTEM_INFO sys_info;
		GetSystemInfo(&sys_info);
		const DWORD MAX_THREAD = sys_info.dwNumberOfProcessors * 2 +2;

		//通知后台线程退出
		InterlockedExchange((long volatile *)&m_bThreadRun, FALSE);

		WaitForMultipleObjects(MAX_THREAD, m_pThreads, TRUE, 30 * 1000);

		for (DWORD nIndex = 0; nIndex < MAX_THREAD; nIndex ++)
		{
			CloseHandle(m_pThreads[nIndex]);
		}

		delete []m_pThreads;

		CloseHandle(m_hCompletion);
		
		EnterCriticalSection(&m_RcvDataLock);
		m_RcvDataQue.clear();
		LeaveCriticalSection(&m_RcvDataLock);

		DeleteCriticalSection(&m_RcvDataLock);
	}

	void UdpSer::CloseServer()
	{
		m_bSerRun = FALSE;
		closesocket(m_hSock);
	}

	UDP_RCV_DATA *UdpSer::GetRcvData(DWORD* pCount)
	{
		UDP_RCV_DATA* pRcvData = NULL;

		EnterCriticalSection(&m_RcvDataLock);
		vector<UDP_RCV_DATA* >::iterator iterRcv = m_RcvDataQue.begin();
		if (iterRcv != m_RcvDataQue.end())
		{
			pRcvData = *iterRcv;
			m_RcvDataQue.erase(iterRcv);
		}

		if (pCount)
		{
			*pCount = (DWORD)(m_RcvDataQue.size());
		}
		LeaveCriticalSection(&m_RcvDataLock);

		return pRcvData;
	}

	void UdpSer::ReadCompletion(BOOL bSuccess, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		UDP_CONTEXT* pRcvContext = CONTAINING_RECORD(lpOverlapped, UDP_CONTEXT, m_ol);
		WSABUF RcvBuf = { NULL, 0 };
		DWORD dwBytes = 0;
		DWORD dwFlag = 0;
		INT nAddrLen = sizeof(IP_ADDR);
		INT iErrCode = 0;

		if (TRUE == bSuccess && dwNumberOfBytesTransfered <= UDP_CONTEXT::S_PAGE_SIZE)
		{
#ifdef _XML_NET_
			EnterCriticalSection(&m_RcvDataLock);

			UDP_RCV_DATA* pRcvData = new UDP_RCV_DATA(pRcvContext->m_pBuf, dwNumberOfBytesTransfered, pRcvContext->m_RemoteAddr);
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

				UDP_RCV_DATA* pRcvData = new UDP_RCV_DATA(pRcvContext->m_pBuf, dwNumberOfBytesTransfered, pRcvContext->m_RemoteAddr);
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
			RcvBuf.len = UDP_CONTEXT::S_PAGE_SIZE;

			iErrCode = WSARecvFrom(pRcvContext->m_hSock, &RcvBuf, 1, &dwBytes, &dwFlag, (sockaddr*)(&pRcvContext->m_RemoteAddr)
				, &nAddrLen, &(pRcvContext->m_ol), NULL);
			if (SOCKET_ERROR == iErrCode && ERROR_IO_PENDING != WSAGetLastError())
			{
				ATLTRACE("\r\n%s -- %ld dwNumberOfBytesTransfered = %ld, LAST_ERR = %ld"
					, __FILE__, __LINE__, dwNumberOfBytesTransfered, WSAGetLastError());
				delete pRcvContext;
				pRcvContext = NULL;
			}
		}
		else
		{
			delete pRcvContext;
		}
	}

	BOOL UdpSer::SendData(const IP_ADDR& PeerAddr, const CHAR* szData, INT nLen)
	{
		BOOL bRet = TRUE;
		try
		{
			if (nLen >= 1500)
			{
				THROW_LINE;;
			}

			UDP_CONTEXT* pSendContext = new UDP_CONTEXT();
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
			_TRACE("Exp : %s -- %ld ", __FILE__, lErrLine);			
		}

		return bRet;
	}

	BOOL UdpSer::StartServer(const CHAR* szIp /* =  */, INT nPort /* = 0 */)
	{
		BOOL bRet = TRUE;
		const int RECV_COUNT = 500;
		WSABUF RcvBuf = { NULL, 0 };
		DWORD dwBytes = 0;
		DWORD dwFlag = 0;
		INT nAddrLen = sizeof(IP_ADDR);
		INT iErrCode = 0;

		try
		{
			if (m_bSerRun)
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

			IP_ADDR addr(szIp, nPort);
			if (SOCKET_ERROR == bind(m_hSock, (sockaddr*)&addr, sizeof(addr)))
			{
				closesocket(m_hSock);
				THROW_LINE;;
			}

			//将SOCKET绑定到完成端口上
			CreateIoCompletionPort((HANDLE)m_hSock, m_hCompletion, 0, 0);

			//投递读操作
			for (int nIndex = 0; nIndex < RECV_COUNT; nIndex++)
			{
				UDP_CONTEXT* pRcvContext = new UDP_CONTEXT();
				if (pRcvContext && pRcvContext->m_pBuf)
				{
					dwFlag = 0;
					dwBytes = 0;
					nAddrLen = sizeof(IP_ADDR);
					RcvBuf.buf = pRcvContext->m_pBuf;
					RcvBuf.len = UDP_CONTEXT::S_PAGE_SIZE;

					pRcvContext->m_hSock = m_hSock;
					pRcvContext->m_nOperation = OP_READ;			
					iErrCode = WSARecvFrom(pRcvContext->m_hSock, &RcvBuf, 1, &dwBytes, &dwFlag, (sockaddr*)(&pRcvContext->m_RemoteAddr)
						, &nAddrLen, &(pRcvContext->m_ol), NULL);
					if (SOCKET_ERROR == iErrCode && ERROR_IO_PENDING != WSAGetLastError())
					{
						delete pRcvContext;
						pRcvContext = NULL;
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
			_TRACE("Exp : %s -- %ld ", __FILE__, lErrLine);			
		}

		return bRet;
	}

	UINT WINAPI UdpSer::WorkThread(LPVOID lpParam)
	{
		UdpSer *pThis = (UdpSer *)lpParam;
		DWORD dwTrans = 0, dwKey = 0;
		LPOVERLAPPED pOl = NULL;
		UDP_CONTEXT *pContext = NULL;

		while (TRUE)
		{
			BOOL bOk = GetQueuedCompletionStatus(pThis->m_hCompletion, &dwTrans, &dwKey, (LPOVERLAPPED *)&pOl, WSA_INFINITE);

			pContext = CONTAINING_RECORD(pOl, UDP_CONTEXT, m_ol);
			if (pContext)
			{
				switch (pContext->m_nOperation)
				{
				case OP_READ:
					pThis->ReadCompletion(bOk, dwTrans, pOl);
					break;
				case OP_WRITE:
					delete pContext;
					pContext = NULL;
					break;
				}
			}

			if (FALSE == InterlockedExchangeAdd(&(pThis->m_bThreadRun), 0))
			{
				break;
			}
		}

		return 0;
	}
}