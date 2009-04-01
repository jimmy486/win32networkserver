#include "UdpMng.h"
#include <assert.h>
#include <process.h>
#include <atltrace.h>

#include "RunLog.h"

//#ifndef _XML_NET_
//#define _XML_NET_
//#endif
/**
* 若要处理XML流请打开宏开关 _XML_NET_ 
* 否则网络模块将按二进制数据进行处理, 
* 其二进制数据头参见 HeadFile.h中的 
* PACKET_HEAD 定义
*/

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
				throw ((long)(__LINE__));
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
				throw ((long)(__LINE__));
			}
			LeaveCriticalSection(&s_IDLQueLock);
		}
		catch (const long& iErrCode)
		{
			pContext = NULL;
			LeaveCriticalSection(&s_IDLQueLock);
			_TRACE("\r\nExcept : %s--%ld", __FILE__, iErrCode);
		}
		catch (...)
		{
			pContext = NULL;
			LeaveCriticalSection(&s_IDLQueLock);
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

	BOOL UDP_CONTEXT::IsAddressValid(LPCVOID lpMem)
	{
		BOOL bResult = HeapValidate(s_hHeap, 0, lpMem);

		if (NULL == lpMem)
		{
			bResult = FALSE;
		}	

		return bResult;
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
				throw ((long)(__LINE__));
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
			_TRACE("\r\nExcept : %s--%ld", __FILE__, iErrCode);
		}
		catch (...)
		{
			m_pData = NULL;
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
				throw ((long)(__LINE__));
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
				throw ((long)(__LINE__));
			}

			LeaveCriticalSection(&s_IDLQueLock);
		}
		catch (const long& iErrCode)
		{
			_TRACE("\r\nExcept : %s--%ld", __FILE__, iErrCode);
			pRcvData = NULL;
			LeaveCriticalSection(&s_IDLQueLock);
		}
		catch (...)
		{
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

	BOOL UDP_RCV_DATA::IsAddressValid(LPCVOID pMem)
	{
		BOOL bResult = HeapValidate(s_Heap, 0, pMem);

		if (NULL == pMem)
		{
			bResult = FALSE;
		}

		return bResult;
	}

	BOOL UDP_RCV_DATA::IsDataVailid()
	{
		BOOL bResult = HeapValidate(s_DataHeap, 0, m_pData);

		if (NULL == m_pData)
		{
			bResult = FALSE;
		}

		return bResult;
	}

	//CUdpMng的实现
	CUdpMng::CUdpMng()
	{
		m_hSock = INVALID_SOCKET;

		InitializeCriticalSection(&m_RcvDataLock);
		//为相关队列进行空间预留
		m_RcvDataQue.reserve(50000 * sizeof(void*));
	}

	CUdpMng::~CUdpMng(void)
	{
		closesocket(m_hSock);
		Sleep(1000);

		EnterCriticalSection(&m_RcvDataLock);

		const DWORD RCV_DATA_SIZE = (DWORD)(m_RcvDataQue.size());
		for (DWORD index = 0; index < RCV_DATA_SIZE; index++)
		{
			UDP_RCV_DATA* pRcvData = m_RcvDataQue[index];
			delete pRcvData;
			pRcvData = NULL;
		}
		m_RcvDataQue.clear();
		LeaveCriticalSection(&m_RcvDataLock);

		DeleteCriticalSection(&m_RcvDataLock);

	}

	BOOL CUdpMng::Init(const CHAR* szIp, INT nPort /* = 0 */)
	{
		BOOL bResult = TRUE;
		try
		{
			if (INVALID_SOCKET != m_hSock)
			{
				closesocket(m_hSock);
			}

			m_hSock = WSASocket(AF_INET, SOCK_DGRAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			if (INVALID_SOCKET == m_hSock)
			{
				throw ((long)(__LINE__));
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
				throw ((long)(__LINE__));
			}
			//将其绑定到完成端口上
			if (FALSE == BindIoCompletionCallback((HANDLE)m_hSock, IOCompletionRoutine, 0))
			{
				throw ((long)(__LINE__));
			}
			//为m_hSock投递10个WSARecv操作
			PostRecv(5);
		}
		catch (const long& iErrCode)
		{
			bResult = FALSE;
			closesocket(m_hSock);
			m_hSock = INVALID_SOCKET;
			_TRACE("\r\nExcept : %s--%ld", __FILE__, iErrCode);
		}

		return bResult;
	}

	void CUdpMng::PostRecv(INT nNum)
	{
		WSABUF RcvBuf = { NULL, 0 };
		DWORD dwBytes = 0;
		DWORD dwFlag = 0;
		INT nAddrLen = sizeof(IP_ADDR);
		INT iErrCode = 0;

		for (INT index = 0; index < nNum; index++)
		{
			UDP_CONTEXT* pRcvContext = new UDP_CONTEXT(this);
			if (pRcvContext)
			{
				dwFlag = 0;
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
		}
	}

	UDP_RCV_DATA* CUdpMng::GetRcvDataFromQue(DWORD* pCount)
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

	void CALLBACK CUdpMng::IOCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		UDP_CONTEXT* pContext = CONTAINING_RECORD(lpOverlapped, UDP_CONTEXT, m_ol);

		switch (pContext->m_nOperation)
		{
		case OP_READ:
			ReadCompletion(dwErrorCode, dwNumberOfBytesTransfered, lpOverlapped);
			break;

		case OP_WRITE:
			SendCompletion(dwErrorCode, dwNumberOfBytesTransfered, lpOverlapped);
			break;

		default:
			break;
		}
	}

	void CUdpMng::ReadCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		UDP_CONTEXT* pRcvContext = CONTAINING_RECORD(lpOverlapped, UDP_CONTEXT, m_ol);

#if 0
		//网络上有错误发生
		if ((0 != dwErrorCode) && (ERROR_IO_PENDING != WSAGetLastError()))
		{
			//delete pRcvContext;
			//pRcvContext = NULL;
			//ATLTRACE("\r\n%s -- %ld dwErrorCode = %ld, dwNumberOfBytesTransfered = %ld, LAST_ERR = %ld"
			//	, __FILE__, __LINE__, dwErrorCode, dwNumberOfBytesTransfered, WSAGetLastError());			

			//return;
		}
		//XML数据包直接放入接收队列中
#ifdef _XML_NET_
		else
		{
			pRcvContext->m_pMng->PushInRcvDataQue(pRcvContext->m_pBuf, dwNumberOfBytesTransfered, pRcvContext->m_RemoteAddr);
		}
#else
		else if (dwNumberOfBytesTransfered >= sizeof(PACKET_HEAD))
		{
			//closesocket(pRcvContext->m_hSock);
			//InterlockedExchange(&(pRcvContext->m_nPostCount), 0);
			//判断数据包的类型
			const PACKET_HEAD* pHeadInfo = (const PACKET_HEAD*)(pRcvContext->m_pBuf);
			//数据包正确将其压入到接收队列中
			if (dwNumberOfBytesTransfered == pHeadInfo->nCurrentLen +sizeof(PACKET_HEAD))
			{
				pRcvContext->m_pMng->PushInRcvDataQue(pRcvContext->m_pBuf
					, dwNumberOfBytesTransfered, pRcvContext->m_RemoteAddr);
			}
		}
#endif	//#ifdef _XML_NET_

#endif		//#if 0

		if (0 == dwErrorCode && dwNumberOfBytesTransfered <= UDP_CONTEXT::S_PAGE_SIZE)
		{
#ifdef _XML_NET_
			pRcvContext->m_pMng->PushInRcvDataQue(pRcvContext->m_pBuf
				, dwNumberOfBytesTransfered, pRcvContext->m_RemoteAddr);
#else
			if (dwNumberOfBytesTransfered >= sizeof(PACKET_HEAD))
			{
				pRcvContext->m_pMng->PushInRcvDataQue(pRcvContext->m_pBuf
					, dwNumberOfBytesTransfered, pRcvContext->m_RemoteAddr);
			}
#endif
		}

		WSABUF RcvBuf = { NULL, 0 };
		DWORD dwBytes = 0;
		DWORD dwFlag = 0;
		INT nAddrLen = sizeof(IP_ADDR);
		INT iErrCode = 0;

		//投递下一个接收操作
		RcvBuf.buf = pRcvContext->m_pBuf;
		RcvBuf.len = UDP_CONTEXT::S_PAGE_SIZE;

		iErrCode = WSARecvFrom(pRcvContext->m_hSock, &RcvBuf, 1, &dwBytes, &dwFlag, (sockaddr*)(&pRcvContext->m_RemoteAddr)
			, &nAddrLen, &(pRcvContext->m_ol), NULL);
		if (SOCKET_ERROR == iErrCode && ERROR_IO_PENDING != WSAGetLastError())
		{
			ATLTRACE("\r\n%s -- %ld dwErrorCode = %ld, dwNumberOfBytesTransfered = %ld, LAST_ERR = %ld"
				, __FILE__, __LINE__, dwErrorCode, dwNumberOfBytesTransfered, WSAGetLastError());
			delete pRcvContext;
			pRcvContext = NULL;
		}
	}

	void CUdpMng::SendCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		UDP_CONTEXT* pSendContext = CONTAINING_RECORD(lpOverlapped, UDP_CONTEXT, m_ol);

		delete pSendContext;
		pSendContext = NULL;
	}
	
	void CUdpMng::PushInRcvDataQue(const CHAR* szBuf, INT nLen, const IP_ADDR& addr)
	{
		UDP_RCV_DATA* pRcvData = new UDP_RCV_DATA(szBuf, nLen, addr);
		if (pRcvData && pRcvData->m_pData)
		{
			EnterCriticalSection(&m_RcvDataLock);
			m_RcvDataQue.push_back(pRcvData);
			LeaveCriticalSection(&m_RcvDataLock);
		}	
		else
		{
			delete pRcvData;
		}
	}

	BOOL CUdpMng::SendData(const IP_ADDR &PeerAddr, const CHAR *szData, INT nLen)
	{
		if (nLen >= 1500)
		{
			return FALSE;
		}

		UDP_CONTEXT* pSendContext = new UDP_CONTEXT(this);

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
			//InterlockedExchange((volatile LONG*)&(pSendContext->m_hSock), INVALID_SOCKET);
		}

		return TRUE;
	}

	void CUdpMng::InitReource()
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

	void CUdpMng::ReleaseReource()
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
}