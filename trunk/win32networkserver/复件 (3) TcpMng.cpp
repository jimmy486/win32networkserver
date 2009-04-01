#include "TcpMng.h"
#include <process.h>
#include <assert.h>

#ifdef WIN32
#include <atltrace.h>

#ifdef _DEBUG
#define _TRACE ATLTRACE
#else
#define _TRACE 
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
	vector<TCPMNG_CONTEXT* > TCPMNG_CONTEXT::s_IDLQue;
	CRITICAL_SECTION TCPMNG_CONTEXT::s_IDLQueLock;
	HANDLE TCPMNG_CONTEXT::s_hHeap = NULL;
	volatile LONG TCPMNG_CONTEXT::s_nCount = 0;

	vector<CONNECT_CONTEXT* > CONNECT_CONTEXT::s_IDLQue;
	CRITICAL_SECTION CONNECT_CONTEXT::s_IDLQueLock;
	HANDLE CONNECT_CONTEXT::s_hHeap = NULL;
	volatile LONG CONNECT_CONTEXT::s_nCount = 0;

	vector<ACCEPT_CONTEXT* > ACCEPT_CONTEXT::s_IDLQue;
	CRITICAL_SECTION ACCEPT_CONTEXT::s_IDLQueLock;
	HANDLE ACCEPT_CONTEXT::s_hHeap = NULL;
	volatile LONG ACCEPT_CONTEXT::s_nCount = 0;	

	HANDLE TCP_RCV_DATA::s_DataHeap = NULL;
	HANDLE TCP_RCV_DATA::s_hHeap = NULL;
	volatile LONG TCP_RCV_DATA::s_nCount = 0;
	vector<TCP_RCV_DATA* > TCP_RCV_DATA::s_IDLQue;
	CRITICAL_SECTION TCP_RCV_DATA::s_IDLQueLock;

	HANDLE TCP_SEND_DATA::s_hHeap = NULL;
	HANDLE TCP_SEND_DATA::s_DataHeap = NULL;
	volatile LONG TCP_SEND_DATA::s_nCount = 0;
	vector<TCP_SEND_DATA* > TCP_SEND_DATA::s_IDLQue;
	CRITICAL_SECTION TCP_SEND_DATA::s_IDLQueLock;

	HANDLE TCP_CONTEXT::s_hHeap = NULL;
	volatile LONG TCP_CONTEXT::s_nCount = 0;
	vector<TCP_CONTEXT* > TCP_CONTEXT::s_IDLQue;
	CRITICAL_SECTION TCP_CONTEXT::s_IDLQueLock;
	
	volatile LONG CTcpMng::s_nCount = 0;							
	LPFN_ACCEPTEX CTcpMng::s_pfAcceptEx = NULL;
	LPFN_CONNECTEX CTcpMng::s_pfConnectEx = NULL;
	//LPFN_GETACCEPTEXSOCKADDRS CTcpMng::s_pfGetAddrs = NULL;

	//class TCPMNG_CONTEXT的相关实现

	void* TCPMNG_CONTEXT::operator new(size_t nSize)
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
			vector<TCPMNG_CONTEXT* >::iterator iter = s_IDLQue.begin();
	
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

			//if (bQueEmpty)
			//{
			//	pContext = HeapAlloc(s_hHeap, HEAP_ZERO_MEMORY, nSize);
			//}

			if (NULL == pContext)
			{
				throw ((long)(__LINE__));
			}
		}
		catch (const long& iErrCode)
		{
			InterlockedExchangeAdd(&s_nCount, -1);
			pContext = NULL;
			_TRACE("\r\nExcept : %s--%ld", __FILE__, iErrCode);
		}

		return pContext;
	}

	void TCPMNG_CONTEXT::operator delete(void* p)
	{
		if (p)		
		{
			//若空闲队列长度小于10000, 则将其放入空闲队列中; 否则释放

			EnterCriticalSection(&s_IDLQueLock);
			const DWORD QUE_SIZE = (DWORD)(s_IDLQue.size());
			TCPMNG_CONTEXT* const pContext = (TCPMNG_CONTEXT*)p;

			if (QUE_SIZE <= MAX_IDL_DATA)
			{
				s_IDLQue.push_back(pContext);
			}
			else
			{
				HeapFree(s_hHeap, HEAP_NO_SERIALIZE, p);
			}
			LeaveCriticalSection(&s_IDLQueLock);

			//if (QUE_SIZE > MAX_IDL_DATA)
			//{
			//	HeapFree(s_hHeap, 0, p);
			//}
			//InterlockedExchangeAdd(&s_nCount, -1);

			//已经没有对象再被释放需要将堆释放
			//if (0 >= s_nCount)
			//{
			//	HeapDestroy(s_hHeap);
			//	s_hHeap = NULL;

			//	EnterCriticalSection(&s_IDLQueLock);
			//	s_IDLQue.clear();
			//	LeaveCriticalSection(&s_IDLQueLock);

			//	DeleteCriticalSection(&s_IDLQueLock);
			//}
		}

		return;
	}

	BOOL TCPMNG_CONTEXT::IsAddressValid(LPCVOID lpMem)
	{
		BOOL bResult = HeapValidate(s_hHeap, 0, lpMem);

		if (NULL == lpMem)
		{
			bResult = FALSE;
		}	

		return bResult;
	}

	//class CONNECT_CONTEXT的相关实现
	CONNECT_CONTEXT::CONNECT_CONTEXT(LPCONNECT_ROUTINE lpConProc, LPVOID pParam, CTcpMng* pMng)
		: TCPMNG_CONTEXT(pMng)
	{
		m_pConProc = lpConProc;
		m_pParam = pParam;
	}

	void* CONNECT_CONTEXT:: operator new(size_t nSize)
	{
		void* pContext = NULL;

		try
		{
			//还没有对象被申请过，需要先创建堆
			//if (0 == InterlockedExchangeAdd(&s_nCount, 1))
			//{			
			//	s_hHeap = HeapCreate(0, BUF_SIZE, HEAP_SIZE);
			//	InitializeCriticalSection(&s_IDLQueLock);
			//}
			if (NULL == s_hHeap)
			{
				throw ((long)(__LINE__));
			}

			//申请堆内存, 现在空闲队列中寻找, 当空闲队列中没有时则在堆上申请
			//bool bQueEmpty = true;

			EnterCriticalSection(&s_IDLQueLock);
			vector<CONNECT_CONTEXT* >::iterator iter = s_IDLQue.begin();

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

			//if (bQueEmpty)
			//{
			//	pContext = HeapAlloc(s_hHeap, HEAP_ZERO_MEMORY, nSize);
			//}

			if (NULL == pContext)
			{
				throw ((long)(__LINE__));
			}
		}
		catch (const long& iErrCode)
		{
			InterlockedExchangeAdd(&s_nCount, -1);
			pContext = NULL;
			_TRACE("\r\nExcept : %s--%ld", __FILE__, iErrCode);
		}

		return pContext;
	}

	void CONNECT_CONTEXT:: operator delete(void* p)
	{
		if (p)
		{
			//若空闲队列长度小于10000, 则将其放入空闲队列中; 否则释放

			EnterCriticalSection(&s_IDLQueLock);
			const DWORD QUE_SIZE = (DWORD)(s_IDLQue.size());
			CONNECT_CONTEXT* const pContext = (CONNECT_CONTEXT*)p;

			if (QUE_SIZE <= MAX_IDL_DATA)
			{
				s_IDLQue.push_back(pContext);
			}
			else
			{
				HeapFree(s_hHeap, HEAP_NO_SERIALIZE, p);
			}
			LeaveCriticalSection(&s_IDLQueLock);

			//if (QUE_SIZE > MAX_IDL_DATA)
			//{
			//	HeapFree(s_hHeap, 0, p);
			//}
			//InterlockedExchangeAdd(&s_nCount, -1);

			//已经没有对象再被释放需要将堆释放
			//if (0 >= s_nCount)
			//{
			//	HeapDestroy(s_hHeap);
			//	s_hHeap = NULL;

			//	EnterCriticalSection(&s_IDLQueLock);
			//	s_IDLQue.clear();
			//	LeaveCriticalSection(&s_IDLQueLock);

			//	DeleteCriticalSection(&s_IDLQueLock);
			//}
		}


		return;
	}

	BOOL CONNECT_CONTEXT::IsAddressValid(LPCVOID lpMem)
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
			//还没有对象被申请过，需要先创建堆
			//if (0 == InterlockedExchangeAdd(&s_nCount, 1))
			//{			
			//	s_hHeap = HeapCreate(0, BUF_SIZE, HEAP_SIZE);
			//	InitializeCriticalSection(&s_IDLQueLock);
			//}
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
				//bQueEmpty = false;
			}
			else
			{
				pContext = HeapAlloc(s_hHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, nSize);
			}
			LeaveCriticalSection(&s_IDLQueLock);

			//if (bQueEmpty)
			//{
			//	pContext = HeapAlloc(s_hHeap, HEAP_ZERO_MEMORY, nSize);
			//}

			if (NULL == pContext)
			{
				throw ((long)(__LINE__));
			}
		}
		catch (const long& iErrCode)
		{
			InterlockedExchangeAdd(&s_nCount, -1);
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

			//if (QUE_SIZE > MAX_IDL)
			//{
			//	HeapFree(s_hHeap, 0, p);
			//}
			//InterlockedExchangeAdd(&s_nCount, -1);

			////已经没有对象再被释放需要将堆释放
			//if (0 >= s_nCount)
			//{
			//	HeapDestroy(s_hHeap);
			//	s_hHeap = NULL;

			//	EnterCriticalSection(&s_IDLQueLock);
			//	s_IDLQue.clear();
			//	LeaveCriticalSection(&s_IDLQueLock);

			//	DeleteCriticalSection(&s_IDLQueLock);
			//}
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
	TCP_RCV_DATA::TCP_RCV_DATA(const SOCKET& hSock, const CHAR* pBuf, INT nLen)
		: m_hSock(hSock)
		, m_nLen(nLen)
	{
		try
		{
			assert(pBuf);
			if (NULL == s_DataHeap)
			{
				throw ((long)(__LINE__));
			}

			//_TRACE("\r\n%s:%ld s_DataHeap = 0x%x; m_pData = 0x%x", __FILE__, __LINE__, s_DataHeap, m_pData);
			m_pData = (CHAR*)HeapAlloc(s_DataHeap, HEAP_ZERO_MEMORY, nLen);
			memcpy(m_pData, pBuf, nLen);

			//_TRACE("\r\n%s:%ld s_DataHeap = 0x%x; m_pData = 0x%x", __FILE__, __LINE__, s_DataHeap, m_pData);
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
			//还没有对象被申请过，需要先创建堆
			//if (0 == InterlockedExchangeAdd(&s_nCount, 1))
			//{			
			//	s_hHeap = HeapCreate(0, 0, HEAP_SIZE);
			//	s_DataHeap = HeapCreate(0, 0, DATA_HEAP_SIZE);
			//	InitializeCriticalSection(&s_IDLQueLock);
			//}
			if (NULL == s_hHeap || NULL == s_DataHeap)
			{
				throw ((long)(__LINE__));
			}

			//bool bQueEmpty = true;
			EnterCriticalSection(&s_IDLQueLock);
			vector<TCP_RCV_DATA* >::iterator iter = s_IDLQue.begin();
			if (s_IDLQue.end() != iter)
			{
				pRcvData = *iter;
				s_IDLQue.erase(iter);
				//bQueEmpty = false;
			}
			else
			{
				pRcvData = HeapAlloc(s_hHeap, HEAP_ZERO_MEMORY | HEAP_NO_SERIALIZE, nSize);
			}
			LeaveCriticalSection(&s_IDLQueLock);

			//if (bQueEmpty)
			//{
			//	//申请堆内存
			//	pRcvData = HeapAlloc(s_hHeap, HEAP_ZERO_MEMORY, nSize);
			//}
			//_TRACE("\r\n%s:%ld pRcvData= 0x%x; s_hHeap= 0x%x", __FILE__, __LINE__, pRcvData, s_hHeap);

			if (NULL == pRcvData)
			{
				throw ((long)(__LINE__));
			}
		}
		catch (const long& iErrCode)
		{
			InterlockedExchangeAdd(&s_nCount, -1);
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

			//if (QUE_SIZE > MAX_IDL_DATA)
			//{
			//	HeapFree(s_hHeap, 0, p);
			//}
			//InterlockedExchangeAdd(&s_nCount, -1);

			////已经没有对象再被释放需要将堆释放
			//if (0 >= s_nCount)
			//{
			//	HeapDestroy(s_hHeap);
			//	s_hHeap = NULL;

			//	HeapDestroy(s_DataHeap);
			//	s_DataHeap = NULL;

			//	EnterCriticalSection(&s_IDLQueLock);
			//	s_IDLQue.clear();
			//	LeaveCriticalSection(&s_IDLQueLock);
   //             
			//	DeleteCriticalSection(&s_IDLQueLock);
			//}
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

	//TCP_SEND_DATA 相关实现
	TCP_SEND_DATA::TCP_SEND_DATA(const CHAR* szData, INT nLen)
		: m_nLen(nLen)
	{
		assert(szData);
		try
		{
			if (NULL == s_DataHeap)
			{
				throw ((long)(__LINE__));
			}

			m_pData = (CHAR*)HeapAlloc(s_DataHeap, HEAP_ZERO_MEMORY, nLen);
			memcpy(m_pData, szData, nLen);
		}
		catch (const long& iErrCode)
		{
			_TRACE("\r\nExcept : %s--%ld", __FILE__, iErrCode);
		}
	}

	TCP_SEND_DATA::~TCP_SEND_DATA()
	{
		if ((NULL != m_pData ) && (NULL != s_DataHeap))
		{
			HeapFree(s_DataHeap, 0, m_pData);
			m_pData = NULL;
		}
	}

	void* TCP_SEND_DATA::operator new(size_t nSize)
	{
		void* pSendData = NULL;

		try
		{
			//还没有对象被申请过，需要先创建堆
			//if (0 == InterlockedExchangeAdd(&s_nCount, 1))
			//{			
			//	s_hHeap = HeapCreate(0, 0, SEND_DATA_HEAP_SIZE);
			//	s_DataHeap = HeapCreate(0, 0, DATA_HEAP_SIZE);
			//	InitializeCriticalSection(&s_IDLQueLock);
			//}
			if (NULL == s_hHeap || NULL == s_DataHeap)
			{
				throw ((long)(__LINE__));
			}

			//bool bQueEmpty = true;
			EnterCriticalSection(&s_IDLQueLock);
			vector<TCP_SEND_DATA* >::iterator iter = s_IDLQue.begin();
			if (s_IDLQue.end() != iter)
			{
				pSendData = *iter;
				s_IDLQue.erase(iter);
				//bQueEmpty = false;
			}
			else
			{
				pSendData = HeapAlloc(s_hHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, nSize);
			}
			LeaveCriticalSection(&s_IDLQueLock);

			//if (bQueEmpty)
			//{
			//	//申请堆内存
			//	pSendData = HeapAlloc(s_hHeap, HEAP_ZERO_MEMORY, nSize);
			//}

			if (NULL == pSendData)
			{
				throw ((long)(__LINE__));
			}
		}
		catch (const long& iErrCode)
		{
			InterlockedExchangeAdd(&s_nCount, -1);
			pSendData = NULL;
			_TRACE("\r\nExcept : %s--%ld", __FILE__, iErrCode);
		}

		return pSendData;
	}

	void TCP_SEND_DATA::operator delete(void* p)
	{
		if (p)
		{
			EnterCriticalSection(&s_IDLQueLock);
			const DWORD QUE_SIZE = (DWORD)(s_IDLQue.size());
			TCP_SEND_DATA* pContext = (TCP_SEND_DATA*)p;

			if (QUE_SIZE <= MAX_IDL_DATA)
			{
				s_IDLQue.push_back(pContext);
			}
			else
			{
				HeapFree(s_hHeap, HEAP_NO_SERIALIZE, p);
			}
			LeaveCriticalSection(&s_IDLQueLock);

			//if (QUE_SIZE > MAX_IDL_DATA)
			//{
			//	HeapFree(s_hHeap, 0, p);
			//}
			//InterlockedExchangeAdd(&s_nCount, -1);

			////已经没有对象再被释放需要将堆释放
			//if (0 >= s_nCount)
			//{
			//	HeapDestroy(s_hHeap);
			//	s_hHeap = NULL;

			//	HeapDestroy(s_DataHeap);
			//	s_DataHeap = NULL;

			//	EnterCriticalSection(&s_IDLQueLock);
			//	s_IDLQue.clear();
			//	LeaveCriticalSection(&s_IDLQueLock);
			//	DeleteCriticalSection(&s_IDLQueLock);
			//}
		}

		return;
	}

	BOOL TCP_SEND_DATA::IsAddressValid(LPCVOID pMem)
	{
		BOOL bResult = HeapValidate(s_hHeap, 0, pMem);

		if (NULL == pMem)
		{
			bResult = FALSE;
		}

		return bResult;
	}

	BOOL TCP_SEND_DATA::IsDataValid()
	{
		BOOL bResult = HeapValidate(s_DataHeap, 0, m_pData);

		if (NULL == m_pData)
		{
			bResult = FALSE;
		}

		return bResult;
	}

	//TCP_CONTEXT的实现
	TCP_CONTEXT::TCP_CONTEXT(CTcpMng* pMng)
	{
		m_pRcvContext = new TCPMNG_CONTEXT(pMng);
		m_pSendContext = new TCPMNG_CONTEXT(pMng);
		m_nInteractiveTime = GetTickCount();

		assert(m_pRcvContext);
		assert(m_pSendContext);
	}

	TCP_CONTEXT::~TCP_CONTEXT()
	{
		if (NULL != m_pRcvContext)
		{
			delete m_pRcvContext;
			m_pRcvContext = NULL;
		}

		if (NULL != m_pSendContext)
		{
			delete m_pSendContext;
			m_pSendContext = NULL;
		}

		TCP_SEND_DATA* pSendData = NULL;
		while (FALSE == m_SendDataQue.empty())
		{
			pSendData = m_SendDataQue.front();
			m_SendDataQue.pop();
			if (NULL != pSendData)
			{
				delete pSendData;
				pSendData = NULL;
			}
		}
	}

	void* TCP_CONTEXT::operator new(size_t nSize)
	{
		void* pContext = NULL;

		try
		{
			//还没有对象被申请过，需要先创建堆
			//if (0 == InterlockedExchangeAdd(&s_nCount, 1))
			//{			
			//	s_hHeap = HeapCreate(0, 0, TCP_CONTEXT_HEAP_SIZE);
			//	InitializeCriticalSection(&s_IDLQueLock);
			//}
			if (NULL == s_hHeap)
			{
				throw ((long)(__LINE__));
			}

			//bool bQueEmpty = true;
			EnterCriticalSection(&s_IDLQueLock);
			vector<TCP_CONTEXT*>::iterator iter = s_IDLQue.begin();
			if (s_IDLQue.end() != iter)
			{
				pContext = *iter;
				s_IDLQue.erase(iter);
				//bQueEmpty = false;
			}
			else
			{
				pContext = HeapAlloc(s_hHeap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, nSize);
			}
			LeaveCriticalSection(&s_IDLQueLock);

			//if (bQueEmpty)
			//{
			//	//申请堆内存
			//	pContext = HeapAlloc(s_hHeap, HEAP_ZERO_MEMORY, nSize);
			//}

			if (NULL == pContext)
			{
				throw ((long)(__LINE__));
			}
		}
		catch (const long& iErrCode)
		{
			InterlockedExchangeAdd(&s_nCount, -1);
			pContext = NULL;
			_TRACE("\r\nExcept : %s--%ld", __FILE__, iErrCode);
		}

		return pContext;
	}

	void TCP_CONTEXT::operator delete(void* p)
	{
		if (p)
		{
			EnterCriticalSection(&s_IDLQueLock);
			const DWORD QUE_SIZE = (DWORD)(s_IDLQue.size());
			TCP_CONTEXT* const pContext = (TCP_CONTEXT*)p;

			if (QUE_SIZE <= MAX_IDL_DATA)
			{
				s_IDLQue.push_back(pContext);
			}
			else
			{	
				HeapFree(s_hHeap, 0, p);
			}	
			LeaveCriticalSection(&s_IDLQueLock);

			//if (QUE_SIZE > MAX_IDL_DATA)
			//{
			//	HeapFree(s_hHeap, 0, p);
			//}
			//InterlockedExchangeAdd(&s_nCount, -1);

			////已经没有对象再被释放需要将堆释放
			//if (0 >= s_nCount)
			//{
			//	HeapDestroy(s_hHeap);
			//	s_hHeap = NULL;

			//	EnterCriticalSection(&s_IDLQueLock);
			//	s_IDLQue.clear();
			//	LeaveCriticalSection(&s_IDLQueLock);
			//	DeleteCriticalSection(&s_IDLQueLock);
			//}
		}

		return;
	}


	BOOL TCP_CONTEXT::IsAddressValid(LPCVOID pMem)
	{
		BOOL bResult = HeapValidate(s_hHeap, 0, pMem);

		if (NULL == pMem)
		{
			bResult = FALSE;
		}

		return bResult;
	}

	//CTcpMng的相关实现
	CTcpMng::CTcpMng(void)
		: m_pCloseFun(NULL)
		, m_pCloseParam(NULL)
		, m_hSock(INVALID_SOCKET)
		, m_bThreadRun(TRUE)
	{
		if(0 == InterlockedExchangeAdd(&s_nCount, 1))
		{
			WSADATA wsData;
			WSAStartup(MAKEWORD(2, 2), &wsData);	
		}	

		InitializeCriticalSection(&m_RcvQueLock);
		InitializeCriticalSection(&m_InvalidLock);
		InitializeCriticalSection(&m_ContextLock);

		//为相关队列预留空间
		m_RcvDataQue.reserve(50000 * sizeof(void*));
		m_AcceptQue.reserve(200 * sizeof(void*));
		m_InvalidQue.reserve(20000 * sizeof(SOCKET));
		//m_hSock = INVALID_SOCKET;
		//m_bThreadRun = TRUE;
		//m_InvalidIter = m_InvalidQue.begin();
		//m_MapIter = m_ContextMap.begin();

		//启动后台线程, 线程放在StartServer中启动
		//_beginthreadex(NULL, 0, ThreadProc, this, 0, NULL);
	}

	CTcpMng::~CTcpMng(void)
	{
		CloseServer();

		DeleteCriticalSection(&m_RcvQueLock);
		DeleteCriticalSection(&m_InvalidLock);
		DeleteCriticalSection(&m_ContextLock);

		if (0 == InterlockedExchangeAdd(&s_nCount, -1))
		{
			WSACleanup();
		}
	}

	void CTcpMng::PushInInvalidQue(const SOCKET& s)
	{
		try
		{
			EnterCriticalSection(&m_InvalidLock);
			const DWORD QUE_SIZE = (DWORD)(m_InvalidQue.size());
			DWORD nNum = 0;

			for (nNum = 0; nNum < QUE_SIZE; nNum++)
			{
				if (s == m_InvalidQue[nNum])
				{
					break;
				}
			}
			//不在队列中时才插入
			if (nNum >= QUE_SIZE)
			{
				m_InvalidQue.push_back(s);
			}
			//如果队列之前为空，需要重新设置遍历器
			if (0 == QUE_SIZE)
			{
				m_InvalidIter = m_InvalidQue.begin();
			}
			LeaveCriticalSection(&m_InvalidLock);
		}
		catch(...)
		{
			LeaveCriticalSection(&m_InvalidLock);
		}
	}

	//void CTcpMng::CloseServer()
	//{
	//	if (INVALID_SOCKET == m_hSock)
	//	{
	//		return;
	//	}
	//
	//	try
	//	{
	//		//先将其相关的ACCEPT_CONTEXT的m_nPostCount的标志设为-1, ACCEPT_CONTEXT的释放操作由后台线程处理
	//		for(INT index = 0; index < (INT)(m_AcceptQue.size()); index++)
	//		{
	//			ACCEPT_CONTEXT* pAccContext = m_AcceptQue[index];
	//			if (NULL != pAccContext)
	//			{
	//				InterlockedExchange(&(pAccContext->m_nPostCount), -1);
	//			}
	//		}
	//
	//		closesocket(m_ListenSock);
	//		//等待一段时间以使网络线程有足够的时间完成未决的IO操作
	//		Sleep(200);
	//		//将该socket压入无效队列中
	//		PushInInvalidQue(m_ListenSock);
	//	}
	//	catch(...)
	//	{
	//
	//	}
	//	m_ListenSock = INVALID_SOCKET;
	//}

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

		m_MapIter = m_ContextMap.begin();

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
			//GUID guidGetAddr = WSAID_GETACCEPTEXSOCKADDRS;
			//if (NULL == s_pfGetAddrs)
			//{
			//	nRet = WSAIoctl(m_hSock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidGetAddr, sizeof(guidGetAddr)
			//		, &s_pfGetAddrs, sizeof(s_pfGetAddrs), &dwBytes, NULL, NULL);
			//}
			//if (NULL == s_pfGetAddrs)
			//{
			//	throw ((long)(__LINE__));
			//}

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
#define ACCEPT_NUM 10
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
				//accept发生错误, 关闭服务器(由后台线程处理)
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
			m_hSock = INVALID_SOCKET;
			_TRACE("\r\nExcept : %s--%ld", __FILE__, iErrCode);

			bResult = FALSE;
		}
		return bResult;
	}

	void CTcpMng::CloseServer()
	{
		closesocket(m_hSock);
		m_hSock = INVALID_SOCKET;

		m_bThreadRun = FALSE;
		//等待5秒钟, 使IO线程有足够的时间执行未决的操作
		Sleep(5000);

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

		EnterCriticalSection(&m_ContextLock);

		for (map<SOCKET, TCP_CONTEXT* >::iterator iterMap = m_ContextMap.begin(); m_ContextMap.end() != iterMap; iterMap++)
		{
			TCP_CONTEXT* pContext = iterMap->second;
			delete pContext;
			pContext = NULL;
		}
		m_ContextMap.clear();

		LeaveCriticalSection(&m_ContextLock);

		m_InvalidQue.clear();

	}

	void CALLBACK CTcpMng::IOCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{

		TCPMNG_CONTEXT* pContext = CONTAINING_RECORD(lpOverlapped, TCPMNG_CONTEXT, m_ol);
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
			ConnectCompletionProc(dwErrorCode, dwNumberOfBytesTransfered, lpOverlapped);
			break;

		default:
			break;
		}
	}

	//当AcceptEx操作完成所需要进行的处理
	void CTcpMng::AcceptCompletionProc(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		try
		{
			INT iErrCode = 0;
			DWORD nBytes = 0;
			ACCEPT_CONTEXT* pContext = CONTAINING_RECORD(lpOverlapped, ACCEPT_CONTEXT, m_ol);
			//若投递次数为-1，则说明listen socket已关闭, 不能再投递accept操作
			if (-1 == pContext->m_nPostCount)
			{
				closesocket(pContext->m_hRemoteSock);
				throw ((long)(__LINE__));
			}
			//有错误发生, 关闭客户端socket
			if (0 != dwErrorCode)
			{
				closesocket(pContext->m_hRemoteSock);
			}
			else
			{
				//为到来的连接申请TCP_CONTEXT，投递WSARecv操作, 并将其放入到m_ContextMap中
				//关闭系统缓存，使用自己的缓存以防止数据的复制操作
				INT nZero = 0;
				//IP_ADDR* pClientAddr;
				//IP_ADDR* pLocalAddr;
				//INT nClientLen;
				//INT nLocalLen;

				setsockopt(pContext->m_hRemoteSock, SOL_SOCKET, SO_SNDBUF, (char*)&nZero, sizeof(nZero));
				setsockopt(pContext->m_hRemoteSock, SOL_SOCKET, SO_RCVBUF, (CHAR*)&nZero, sizeof(nZero));
				setsockopt(pContext->m_hRemoteSock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&(pContext->m_hSock), sizeof(pContext->m_hSock));
				
				//s_pfGetAddrs(pContext->m_pBuf, 0, 0, sizeof(sockaddr_in) +16
				//	, NULL, NULL, (LPSOCKADDR*)&pClientAddr, &nClientLen);

				TCP_CONTEXT* pTcpContext = new TCP_CONTEXT(pContext->m_pMng);
				//系统可能已经没有足够的内存可以使用
				if (NULL == pTcpContext)
				{
					closesocket(pContext->m_hRemoteSock);
				}
				//发送和接收缓存亦不能空
				else if (NULL == pTcpContext->m_pRcvContext || NULL == pTcpContext->m_pSendContext)
				{
					closesocket(pContext->m_hRemoteSock);
					delete pTcpContext;
					pTcpContext = NULL;
				}
				else
				{
					//投递读操作
					InterlockedExchange(&(pTcpContext->m_pRcvContext->m_nPostCount), 1);
					pTcpContext->m_pRcvContext->m_hSock = pContext->m_hRemoteSock;
					pTcpContext->m_pRcvContext->m_nOperation = OP_READ;

					//pTcpContext->m_pSendContext->m_hSock = pContext->m_hRemoteSock;

					//绑定到完成端口失败
					if (FALSE == BindIoCompletionCallback((HANDLE)(pTcpContext->m_pRcvContext->m_hSock), IOCompletionRoutine, 0))
					{
						closesocket(pContext->m_hRemoteSock);
						delete pTcpContext;
						pTcpContext = NULL;
					}
					//绑定成功
					else
					{					
						DWORD nFlag = 0;					
						WSABUF RcvBuf;
						RcvBuf.buf = pTcpContext->m_pRcvContext->m_pBuf;
						RcvBuf.len = TCPMNG_CONTEXT::BUF_SIZE;
						iErrCode = WSARecv(pTcpContext->m_pRcvContext->m_hSock, &RcvBuf, 1
							, &nBytes, &nFlag, &(pTcpContext->m_pRcvContext->m_ol), NULL);

						//投递成功
						if (0 == iErrCode || WSA_IO_PENDING == WSAGetLastError())
						{
							pContext->m_pMng->PushInContextMap(pTcpContext->m_pRcvContext->m_hSock, pTcpContext);
						}
						//投递失败
						else
						{
							closesocket(pContext->m_hRemoteSock);
							InterlockedExchange(&(pTcpContext->m_pRcvContext->m_nPostCount), 0);
							delete pTcpContext;
							pTcpContext = NULL;
						}
					}
				}
			}
			//投递新的Accept操作
			SOCKET clientSock = INVALID_SOCKET; 
			for (INT index = 0; index < 10; index++)
			{
				clientSock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
				//只要有一次成功便退出
				if (INVALID_SOCKET != clientSock)
				{
					break;
				}
			}

			ULONG ul = 1;
			ioctlsocket(clientSock, FIONBIO, &ul);

			//系统当前没有可用的socket进行accept操作, 则将其PostCount置为0xfafafafa
			if (INVALID_SOCKET == clientSock)
			{
				InterlockedExchange(&(pContext->m_nPostCount), 0xfafafafa);
				throw ((long)(__LINE__));
			}
			pContext->m_hRemoteSock = clientSock;
			iErrCode = s_pfAcceptEx(pContext->m_hSock, clientSock, pContext->m_pBuf, 0
				, sizeof(sockaddr_in) +16, sizeof(sockaddr_in) +16, &nBytes, &(pContext->m_ol));
			//accept发生错误
			if (FALSE == iErrCode && ERROR_IO_PENDING != WSAGetLastError())
			{
				closesocket(clientSock);
				closesocket(pContext->m_hSock);
				pContext->m_pMng->m_hSock = INVALID_SOCKET;
				InterlockedExchange(&(pContext->m_nPostCount), -1);
				throw ((long)(__LINE__));
			}
		}
		catch (const long& iErrCode)
		{
			_TRACE("\r\nExcept : %s--%ld", __FILE__, iErrCode);
		}
	}

	//当ConnectEx操作完成所需要进行的处理
	void CTcpMng::ConnectCompletionProc(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
#if 0
		CONNECT_CONTEXT* pConnContext = CONTAINING_RECORD(lpOverlapped, CONNECT_CONTEXT, m_ol);
		TCP_CONTEXT* pTcpContext = new TCP_CONTEXT(pConnContext->m_pMng);
		LPCONNECT_ROUTINE lpfun = pConnContext->m_pConProc;
		LPVOID pParam = pConnContext->m_pParam;
		assert(lpfun);
		INT nRet = 0;

		try
		{
			//有错误发生, 关闭socket, 连接队列的清除在析够时进行
			if (0 != dwErrorCode)
			{
				throw ((long)(__LINE__));
			}

			if (NULL == pTcpContext || NULL == pTcpContext->m_pRcvContext || NULL == pTcpContext->m_pSendContext)
			{
				throw ((long)(__LINE__));
			}

			//在该socket上投递一个读操作，并将其放入到m_ContextMap中
			WSABUF RcvBuf;
			DWORD dwBytes;
			DWORD dwFlag = 0;

			InterlockedExchange(&(pTcpContext->m_pRcvContext->m_nPostCount), 1);
			pTcpContext->m_pRcvContext->m_hSock = pConnContext->m_hSock;
			pTcpContext->m_pSendContext->m_hSock = pConnContext->m_hSock;

			pTcpContext->m_pRcvContext->m_nOperation = OP_READ;

			RcvBuf.buf = pTcpContext->m_pRcvContext->m_pBuf;
			RcvBuf.len = TCPMNG_CONTEXT::BUF_SIZE;
			nRet = WSARecv(pConnContext->m_hSock, &RcvBuf, 1, &dwBytes, &dwFlag, &(pTcpContext->m_pRcvContext->m_ol), NULL);
			//接收操作失败
			if (SOCKET_ERROR == nRet && ERROR_IO_PENDING != WSAGetLastError())
			{
				InterlockedExchange(&(pTcpContext->m_pRcvContext->m_nPostCount), 0);
				throw ((long)(__LINE__));
			}

			pConnContext->m_pMng->PushInContextMap(pConnContext->m_hSock, pTcpContext);
			//调用其回调函数
			if (lpfun)
			{
				lpfun(pParam, pConnContext->m_hSock);
			}
		}
		catch (const long& iErrCode)
		{
			closesocket(pConnContext->m_hSock);
			if (pTcpContext)
			{
				delete pTcpContext;
				pTcpContext = NULL;
			}

			if (lpfun)
			{
				lpfun(pParam, INVALID_SOCKET);
			}
			_TRACE("\r\nExcept : %s--%ld", __FILE__, iErrCode);
		}

		//释放CONNECT_CONTEXT
		delete pConnContext;
		pConnContext = NULL;
#endif
	}

	//当WSARecv操作完成所需要进行的处理
	void CTcpMng::RecvCompletionProc(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		TCPMNG_CONTEXT* pRcvContext = CONTAINING_RECORD(lpOverlapped, TCPMNG_CONTEXT, m_ol);
		DWORD dwFlag = 0;

		try
		{
			//有错误产生, 关闭socket，并将其放入到无效的scket队列中
			if (0 != dwErrorCode || 0 == dwNumberOfBytesTransfered)
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

			WSABUF RcvBuf;

#ifdef _XML_NET_	//处理XML流
			TCP_RCV_DATA* pRcvData = new TCP_RCV_DATA(pRcvContext->m_hSock, pRcvContext->m_pBuf, dwNumberOfBytesTransfered);
			//表明没有足够的内存可使用
			if (NULL == pRcvData)
			{					
				throw ((long)(__LINE__));
			}
			pRcvContext->m_pMng->PushInRecvQue(pRcvData);

			pRcvContext->m_nDataLen = 0;
			RcvBuf.buf = pRcvContext->m_pBuf;
			RcvBuf.len = TCPMNG_CONTEXT::BUF_SIZE;

#else				//处理二进制数据流

			//解析包头信息中的应接受的数据包的长度
			pRcvContext->m_nDataLen += dwNumberOfBytesTransfered;
			//WORD nAllLen = *((WORD*)(pRcvContext->m_pBuf));		//此长度为数据的长度+sizeof(WORD)
			PACKET_HEAD* pHeadInfo = (PACKET_HEAD*)(pRcvContext->m_pBuf);
			//WSABUF RcvBuf;
			//数据长度合法才进行处理
			if ((pHeadInfo->nCurrentLen <= TCPMNG_CONTEXT::BUF_SIZE)  
				&& ((WORD)(pRcvContext->m_nDataLen) <= pHeadInfo->nCurrentLen + sizeof(PACKET_HEAD)))
			{
				//所有数据已接收完,将其放入接收队列中
				if ((WORD)(pRcvContext->m_nDataLen) == pHeadInfo->nCurrentLen + sizeof(PACKET_HEAD))
				{
					TCP_RCV_DATA* pRcvData = new TCP_RCV_DATA(pRcvContext->m_pBuf, pRcvContext->m_nDataLen);
					//表明没有足够的内存可使用
					if (NULL == pRcvData)
					{					
						throw ((long)(__LINE__));
					}
					pRcvContext->m_pMng->PushInRecvQue(pRcvData);

					pRcvContext->m_nDataLen = 0;
					RcvBuf.buf = pRcvContext->m_pBuf;
					RcvBuf.len = TCPMNG_CONTEXT::BUF_SIZE;
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
				RcvBuf.len = TCPMNG_CONTEXT::BUF_SIZE;
			}
#endif	//#ifdef _XML_NET_

			//继续投递读操作
			//DWORD dwBytes = 0;
			INT iErrCode = WSARecv(pRcvContext->m_hSock, &RcvBuf, 1, NULL, &dwFlag, &(pRcvContext->m_ol), NULL);
			//Recv操作错误
			if (SOCKET_ERROR == iErrCode && WSA_IO_PENDING != WSAGetLastError())
			{
				throw ((long)(__LINE__));
			}
		}
		catch (const long& iErrCode)
		{
			InterlockedExchange(&(pRcvContext->m_nPostCount), 0);
			closesocket(pRcvContext->m_hSock);
			pRcvContext->m_pMng->PushInInvalidQue(pRcvContext->m_hSock);
			_TRACE("\r\nExcept : %s--%ld", __FILE__, iErrCode);
		}
	}

	//当WSASend操作完成所需要进行的处理
	void CTcpMng::SendCompletionProc(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		TCPMNG_CONTEXT* pSendContext = CONTAINING_RECORD(lpOverlapped, TCPMNG_CONTEXT, m_ol);

		try
		{

			//若有错误产生将其放入无效的socket队列中
			if (0 != dwErrorCode || 0 == dwNumberOfBytesTransfered)
			{
				throw ((long)(__LINE__));
			}

#ifndef _XML_NET_
			pSendContext->m_nDataLen += dwNumberOfBytesTransfered;
			PACKET_HEAD* pHeadInfo = (PACKET_HEAD*)(pSendContext->m_pBuf);
#endif	//#ifndef _XML_NET_

			BOOL bSend = FALSE;		//是否需要投递发送操作
			WSABUF SendBuf;
			//DWORD dwBytes = 0;

			//修改交互时间
			pSendContext->m_pMng->ModifyInteractiveTime(pSendContext->m_hSock);

#ifndef _XML_NET_
			//数据没有发送完还需继续投递发送操作
			if ((pHeadInfo->nCurrentLen +sizeof(PACKET_HEAD) <= TCPMNG_CONTEXT::BUF_SIZE) 
				&& (pSendContext->m_nDataLen < pHeadInfo->nCurrentLen +sizeof(PACKET_HEAD)))
			{
				bSend = TRUE;
				SendBuf.buf = pSendContext->m_pBuf +pSendContext->m_nDataLen;
				SendBuf.len = pHeadInfo->nCurrentLen - pSendContext->m_nDataLen +sizeof(PACKET_HEAD);
			}
			//数据已发送完，可以继续投递新的发送操作
			else
			{
#endif	//#ifndef _XML_NET_

				//从队列中删除上次所投递的发送数据
				TCP_SEND_DATA* pSendData = pSendContext->m_pMng->GetSendData(pSendContext->m_hSock);			

				if (NULL != pSendData)
				{
					bSend = TRUE;
					
					pSendContext->m_nDataLen = 0;	//nLen; 还没有送到网络处理, 所以处理的数据长度为0
					memcpy(pSendContext->m_pBuf, pSendData->m_pData, pSendData->m_nLen);

					SendBuf.buf = pSendContext->m_pBuf;
					SendBuf.len = pSendData->m_nLen;

					delete pSendData;
					pSendData = NULL;
				}
				//没有需要发送的数据
				else
				{
					bSend = FALSE;
					InterlockedExchange(&(pSendContext->m_nPostCount), 0);
				}
#ifndef _XML_NET_
			}
#endif	//#ifndef _XML_NET_

			if (TRUE == bSend)
			{
				INT iErrCode = WSASend(pSendContext->m_hSock, &SendBuf, 1, NULL, 0, &(pSendContext->m_ol), NULL);

				//投递失败
				if (SOCKET_ERROR == iErrCode && ERROR_IO_PENDING != WSAGetLastError())
				{				
					throw ((long)(__LINE__));
				}
			}
		}
		catch (const long& iErrCode)
		{
			InterlockedExchange(&(pSendContext->m_nPostCount), 0);
			closesocket(pSendContext->m_hSock);
			pSendContext->m_pMng->PushInInvalidQue(pSendContext->m_hSock);
			_TRACE("\r\nExcept : %s--%ld", __FILE__, iErrCode);
		}
	}

	TCP_SEND_DATA* CTcpMng::GetSendData(const SOCKET &sock)
	{
		TCP_SEND_DATA* pSendData = NULL;

		EnterCriticalSection(&m_ContextLock);

		map<SOCKET, TCP_CONTEXT* >::iterator itermap = m_ContextMap.find(sock);

		if (m_ContextMap.end() != itermap)
		{
			TCP_CONTEXT* pContext = itermap->second;

			if (pContext)
			{
				if (FALSE == pContext->m_SendDataQue.empty())
				{
					pSendData = pContext->m_SendDataQue.front();
					pContext->m_SendDataQue.pop();
				}
			}
			else
			{
				delete pContext;
				pContext = NULL;
				m_ContextMap.erase(itermap);
			}
		}

		LeaveCriticalSection(&m_ContextLock);

		return pSendData;
	}

	//对accept socket处理有-1 和 0xfafafafa两种形式
	UINT WINAPI CTcpMng::ThreadProc(LPVOID lpParam)
	{	
		CTcpMng* pThis = (CTcpMng*)lpParam;
		const DWORD MAX_ERGOD = 200;		//每次最多遍历200队列元素

		while (pThis->m_bThreadRun)
		{
			//从无效队列中取出无效的socket的进行处理
			EnterCriticalSection(&(pThis->m_InvalidLock));
		
			const DWORD QUE_SIZE = (DWORD)(pThis->m_InvalidQue.size());
			DWORD nCount = 0;
			//_TRACE("\r\n%s:%ld QUE_SIZE = %ld", __FILE__, __LINE__, QUE_SIZE);
			
			//如果已遍历到队列的尾部, 则从头开始重新遍历
			if (pThis->m_InvalidQue.end() == pThis->m_InvalidIter)
			{
				pThis->m_InvalidIter = pThis->m_InvalidQue.begin();
			}

			//每次最多处理200个
			for (nCount = 0; ((nCount < MAX_ERGOD) && (pThis->m_InvalidQue.end() != pThis->m_InvalidIter)); nCount++)
			//for (vector<SOCKET>::iterator sock_iter = pThis->m_InvalidQue.begin(); 
			//	pThis->m_InvalidQue.end() != sock_iter; )
			{	

				SOCKET sInvalid = *(pThis->m_InvalidIter);

				//若在TCP_CONTEXT 的map中则需要检查当前是否还有未决的IO操作, 当没有未决的IO操作时则可以进行释放操作
				EnterCriticalSection(&(pThis->m_ContextLock));

				//map <SOCKET, TCP_CONTEXT* >::iterator iterTcp = pThis->m_ContextMap.find(sInvalid);
				pThis->m_MapIter = pThis->m_ContextMap.find(sInvalid);

				if (pThis->m_MapIter != pThis->m_ContextMap.end())
				{
					//如果下一个要进行遍历元素需要跳过
					//if (iterTcp == pThis->m_MapIter)
					//{
					//	pThis->m_MapIter++;
					//}

					TCP_CONTEXT* pTcpContext = pThis->m_MapIter->second;

					if ((NULL != pTcpContext)
						&& (pTcpContext->m_pRcvContext->m_nPostCount <= 0)
						&& (pTcpContext->m_pSendContext->m_nPostCount <= 0))
					{
						pThis->m_ContextMap.erase(pThis->m_MapIter);					
						pThis->m_InvalidQue.erase(pThis->m_InvalidIter);
						delete pTcpContext;
						pTcpContext = NULL;

						//通知上层该socket已关闭
						if (pThis->m_pCloseFun)
						{
							pThis->m_pCloseFun(pThis->m_pCloseParam, sInvalid);
						}
					}
					else if (NULL == pTcpContext)
					{
						pThis->m_ContextMap.erase(pThis->m_MapIter);					
						pThis->m_InvalidQue.erase(pThis->m_InvalidIter);
					}
					//继续遍历下一个
					else
					{
						pThis->m_InvalidIter++;
					}
				}
				//非法的socket
				else
				{
					pThis->m_InvalidQue.erase(pThis->m_InvalidIter);
				}

				LeaveCriticalSection(&(pThis->m_ContextLock));

				//检查是否在accept 队列中

				//ACCEPT_CONTEXT* pAccContext = NULL;
				//for (vector<ACCEPT_CONTEXT* >::iterator iter = pThis->m_AcceptQue.begin(); iter != pThis->m_AcceptQue.end(); iter++)
				//{
				//	pAccContext = *iter;
				//	if (NULL != pAccContext && pAccContext->m_hSock == sInvalid)
				//	{
				//		pThis->m_AcceptQue.erase(iter);
				//		pThis->m_InvalidQue.erase(sInvalid);
				//		delete pAccContext;
				//		pAccContext = NULL;
				//	}
				//}
			}

			LeaveCriticalSection(&(pThis->m_InvalidLock));

			//检测每个有效的socket上次的交互时间,如果超过十秒钟没有交互则断开其连接
			EnterCriticalSection(&(pThis->m_ContextLock));

			DWORD dwNow = GetTickCount();
			const DWORD TIME_OUT = 10000;
			//const DWORD CONTEXT_NUM = (DWORD)(pThis->m_ContextMap.size());

			if (pThis->m_ContextMap.end() == pThis->m_MapIter)
			{
				pThis->m_MapIter = pThis->m_ContextMap.begin();
			}

			for (nCount = 0; (pThis->m_ContextMap.end() != pThis->m_MapIter && nCount < MAX_ERGOD); nCount++)
			//for (map<SOCKET, TCP_CONTEXT*>::iterator map_iter = pThis->m_ContextMap.begin();
			//	pThis->m_ContextMap.end() != map_iter; map_iter++)
			{
				//if (pThis->m_ContextMap.end() == pThis->m_MapIter)
				//{
				//	break;
				//}

				SOCKET sock = pThis->m_MapIter->first;
				TCP_CONTEXT* pTcpContext = pThis->m_MapIter->second;

				if (pTcpContext)
				{
					if (dwNow - pTcpContext->m_nInteractiveTime >= TIME_OUT)
					{
						closesocket(sock);
						pThis->PushInInvalidQue(sock);						
					}
					pThis->m_MapIter++;
				}
				else
				{
					pThis->m_ContextMap.erase(pThis->m_MapIter);
				}	
			}

			LeaveCriticalSection(&(pThis->m_ContextLock));

#if 0
			//检查accept队列中postcount为0xfafafafa的socket为其投递新的Accept操作
			//const DWORD ACCEPT_COUNT = (DWORD)(pThis->m_AcceptQue.size());

			//for (nCount = 0; nCount < ACCEPT_COUNT; nCount++)
			//{
			//	ACCEPT_CONTEXT* pAccContext = pThis->m_AcceptQue[nCount];
			//	if (0xfafafafa == pAccContext->m_nPostCount)
			//	{
			//		SOCKET sockClient = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
			//		//没有可用socket, 清除ACCEPT_CONTEXT
			//		//if (INVALID_SOCKET == sockClient)
			//		//{
			//		//	pThis->m_AcceptQue.erase(iter);
			//		//	delete pAccContext;
			//		//	pAccContext = NULL;
			//		//}
			//		//投递accept操作
			//		//else
			//		if (INVALID_SOCKET != sockClient)
			//		{
			//			ULONG ul = 1;
			//			ioctlsocket(sockClient, FIONBIO, &ul);

			//			DWORD dwNum;
			//			InterlockedExchange(&(pAccContext->m_nPostCount), 1);
			//			pAccContext->m_hRemoteSock = sockClient;
			//			int iErrCode = s_pfAcceptEx(pAccContext->m_hSock, sockClient, pAccContext->m_pBuf, 0
			//				, sizeof(sockaddr_in) +16, sizeof(sockaddr_in) +16, &dwNum, &(pAccContext->m_ol));
			//			//accept发生错误
			//			if (FALSE == iErrCode && ERROR_IO_PENDING != WSAGetLastError())
			//			{
			//				closesocket(sockClient);
			//				closesocket(pAccContext->m_hSock);
			//				pThis->m_hSock = INVALID_SOCKET;
			//				InterlockedExchange(&(pAccContext->m_nPostCount), 0);
			//			}
			//		}
			//	}
			//}
#endif

			//_TRACE("\r\n%s : %ld 1", __FILE__, __LINE__);
			Sleep(100);
		}

		return 0;
	}

	void CTcpMng::DelTcpContext(const SOCKET& s)
	{
		try
		{
			EnterCriticalSection(&m_ContextLock);
			m_ContextMap.erase(s);
			LeaveCriticalSection(&m_ContextLock);
		}
		catch (...)
		{
			LeaveCriticalSection(&m_ContextLock);
		}
	}

	void CTcpMng::PushInContextMap(const SOCKET&s, TCP_CONTEXT* const pTcpContext)
	{
		try
		{
			EnterCriticalSection(&m_ContextLock);
			bool bEmpty = m_ContextMap.empty();
			m_ContextMap[s] = pTcpContext;

			//如果队列开始为空需要重置遍历器
			if (bEmpty)
			{
				m_MapIter = m_ContextMap.begin();
			}

			LeaveCriticalSection(&m_ContextLock);
		}
		catch (...)
		{
			LeaveCriticalSection(&m_ContextLock);
		}
	}

	void CTcpMng::PushInRecvQue(TCP_RCV_DATA* const pRcvData)
	{
		try
		{		
			assert(pRcvData);
			EnterCriticalSection(&m_RcvQueLock);
			m_RcvDataQue.push_back(pRcvData);
			LeaveCriticalSection(&m_RcvQueLock);
		}
		catch (...)
		{
			LeaveCriticalSection(&m_RcvQueLock);
		}
	}

	TCP_RCV_DATA* CTcpMng::GetRcvData(DWORD* const pQueLen)
	{
		TCP_RCV_DATA* pRcvData = NULL;

		try
		{
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
		}
		catch (...)
		{
			LeaveCriticalSection(&m_RcvQueLock);
		}
		return pRcvData;
	}

	BOOL CTcpMng::Connect(const CHAR* szIP, INT nPort, LPCONNECT_ROUTINE lpConProc, LPVOID pParam)
	{
		BOOL bResult = TRUE;
#if 0
		assert(lpConProc);
		closesocket(m_hSock);

		m_hSock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		CONNECT_CONTEXT* pConnContext = new CONNECT_CONTEXT(lpConProc, pParam, this);

		try 
		{
			if (INVALID_SOCKET == m_hSock)
			{
				throw ((long)(__LINE__));
			}
			if (NULL == pConnContext)
			{
				throw ((long)(__LINE__));
			}

			//加载ConnectEx函数
			GUID guidProc =  WSAID_CONNECTEX;
			DWORD dwBytes;
			INT nRet;		

			if (NULL == s_pfConnectEx)
			{
				nRet = WSAIoctl(m_hSock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidProc, sizeof(guidProc)
					, &s_pfConnectEx, sizeof(s_pfConnectEx), &dwBytes, NULL, NULL);
			}
			if (NULL == s_pfConnectEx || SOCKET_ERROR == nRet)
			{
				throw ((long)(__LINE__));
			}

			ULONG ul = 1;
			ioctlsocket(m_hSock, FIONBIO, &ul);

			//为到来的连接申请TCP_CONTEXT，投递WSARecv操作, 并将其放入到m_ContextMap中
			//关闭系统缓存，使用自己的缓存以防止数据的复制操作
			INT nZero = 0;
			setsockopt(m_hSock, SOL_SOCKET, SO_SNDBUF, (char*)&nZero, sizeof(nZero));
			setsockopt(m_hSock, SOL_SOCKET, SO_RCVBUF, (CHAR*)&nZero, sizeof(nZero));

			//将该socket绑定到完成端口上
			if (FALSE == BindIoCompletionCallback((HANDLE)m_hSock, IOCompletionRoutine, 0))
			{
				throw ((long)(__LINE__));
			}

			//先进行绑定
			IP_ADDR locaAddr("0.0.0.0", 0);
			if (SOCKET_ERROR == bind(m_hSock, (sockaddr*)&locaAddr, sizeof(locaAddr)))
			{
				throw ((long)(__LINE__));
			}
			//执行连接操作
			sockaddr_in SerAddr;
			SerAddr.sin_family = AF_INET;
			SerAddr.sin_port = htons(nPort);
			SerAddr.sin_addr.s_addr = inet_addr(szIP);

			pConnContext->m_hSock = m_hSock;
			pConnContext->m_nOperation = OP_CONNECT;

			nRet = s_pfConnectEx(m_hSock, (sockaddr*)&SerAddr, sizeof(SerAddr), NULL, 0, NULL, &(pConnContext->m_ol));
			//连接发生错误
			if (SOCKET_ERROR == nRet && ERROR_IO_PENDING != WSAGetLastError())
			{
				throw ((long)(__LINE__));
			}		
		}
		catch (const long& iErrCode)
		{
			bResult = FALSE;
			if (INVALID_SOCKET != m_hSock)
			{
				closesocket(m_hSock);
				m_hSock = INVALID_SOCKET;
			}

			delete pConnContext;
			pConnContext = NULL;
			_TRACE("\r\nExcept : %s--%ld", __FILE__, iErrCode);
		}
#endif

		return bResult;
	}

	BOOL CTcpMng::SendData(const SOCKET& sock, const CHAR* szData, INT nDataLen)
	{
#ifdef _XML_NET_
		//数据长度非法
		if ((nDataLen > TCPMNG_CONTEXT::BUF_SIZE) || (NULL == szData))
		{
			return FALSE;
		}
#else
		//数据长度非法
		if ((nDataLen > TCPMNG_CONTEXT::BUF_SIZE) || (NULL == szData) || (nDataLen < sizeof(PACKET_HEAD)))
		{
			return FALSE;
		}
#endif	//#ifdef _XML_NET_

		BOOL bResult = TRUE;

		assert(szData);

		try 
		{
			EnterCriticalSection(&m_ContextLock);

			map<SOCKET, TCP_CONTEXT* >::iterator iterMap = m_ContextMap.find(sock);
			BOOL bSend = FALSE;			//是否需要投递发送操作
			WSABUF SendBuf;
			DWORD dwBytes;
			//WORD nAllLen = 0;

			if (m_ContextMap.end() != iterMap)
			{
				TCP_CONTEXT* pContext = iterMap->second;
				if (pContext)
				{
					//发送队列为空, 并且postcount为0, 则投递发送操作
					if (pContext->m_SendDataQue.empty() && 0 == pContext->m_pSendContext->m_nPostCount)
					{
						bSend = TRUE;
						InterlockedExchange(&(pContext->m_pSendContext->m_nPostCount), 1);
						//nAllLen = (WORD)(nDataLen + sizeof(WORD));
						pContext->m_pSendContext->m_hSock = sock;
						pContext->m_pSendContext->m_nOperation = OP_WRITE;					
						pContext->m_pSendContext->m_nDataLen = 0;	//nLen; 还没有送到网络处理, 所以处理的数据长度为0
						//memcpy(pContext->m_pSendContext->m_pBuf, &nAllLen, sizeof(nAllLen));
						memcpy(pContext->m_pSendContext->m_pBuf, szData, nDataLen);

						SendBuf.buf = pContext->m_pSendContext->m_pBuf;
						SendBuf.len = nDataLen;
					}
					//将数据压入队列中, 并从队列头中取出一个数据进行投递
					else if (0 == pContext->m_pSendContext->m_nPostCount)
					{
						bSend = TRUE;
						TCP_SEND_DATA* pSendData = new TCP_SEND_DATA(szData, nDataLen);

						if (pSendData)
						{
							pContext->m_SendDataQue.push(pSendData);
						}

						pSendData = pContext->m_SendDataQue.front();
						pContext->m_SendDataQue.pop();

						InterlockedExchange(&(pContext->m_pSendContext->m_nPostCount), 1);
						pContext->m_pSendContext->m_nDataLen = 0;
						pContext->m_pSendContext->m_hSock = sock;
						pContext->m_pSendContext->m_nOperation = OP_WRITE;
						//memcpy(pContext->m_pSendContext->m_pBuf, &nAllLen, sizeof(nAllLen));
						memcpy(pContext->m_pSendContext->m_pBuf, pSendData->m_pData, pSendData->m_nLen);

						SendBuf.buf = pContext->m_pSendContext->m_pBuf;
						SendBuf.len = pSendData->m_nLen;

						delete pSendData;		//pSendData = pContext->m_SendDataQue.front();
						pSendData = NULL;
					}
					//将数据压入到队列中即可
					else
					{
						bSend = FALSE;

						TCP_SEND_DATA* pSendData = new TCP_SEND_DATA(szData, nDataLen);
						if (pSendData)
						{
							pContext->m_SendDataQue.push(pSendData);
						}
					}

					if (bSend)
					{
						INT iErr = WSASend(pContext->m_pSendContext->m_hSock, &SendBuf, 1, &dwBytes, 0, &(pContext->m_pSendContext->m_ol), NULL);

						if (SOCKET_ERROR == iErr && ERROR_IO_PENDING != WSAGetLastError())
						{
							InterlockedExchange(&(pContext->m_pSendContext->m_nPostCount), 0);
							closesocket(sock);
							PushInInvalidQue(sock);
						}
					}
				}
				else
				{
					closesocket(sock);
					PushInInvalidQue(sock);
				}
			}
			LeaveCriticalSection(&m_ContextLock);
		}
		catch (...)
		{
			LeaveCriticalSection(&m_ContextLock);
		}

		return bResult;
	}

	TCP_CONTEXT* CTcpMng::GetTcpContext(const SOCKET& s)
	{
		TCP_CONTEXT* pContext = NULL;
		map<SOCKET, TCP_CONTEXT*>:: iterator iter;
		try
		{
			EnterCriticalSection(&m_ContextLock);
			iter = m_ContextMap.find(s);

			if (iter != m_ContextMap.end())
			{
				pContext = iter->second;
			}
			LeaveCriticalSection(&m_ContextLock);
		}
		catch (...)
		{
			LeaveCriticalSection(&m_ContextLock);
			pContext = NULL;
		}

		return pContext;
	}

	void CTcpMng::ModifyInteractiveTime(const SOCKET& s)
	{
		TCP_CONTEXT* pContext = NULL;
		map<SOCKET, TCP_CONTEXT* >::iterator iter;

		EnterCriticalSection(&m_ContextLock);
		iter = m_ContextMap.find(s);

		if (m_ContextMap.end() != iter)
		{
			pContext = iter->second;
			pContext->m_nInteractiveTime = GetTickCount();
		}
		LeaveCriticalSection(&m_ContextLock);
	}

	void CTcpMng::InitReource()
	{
		//TCPMNG_CONTEXT
		TCPMNG_CONTEXT::s_hHeap = HeapCreate(0, TCPMNG_CONTEXT::BUF_SIZE, TCPMNG_CONTEXT::HEAP_SIZE);
		InitializeCriticalSection(&(TCPMNG_CONTEXT::s_IDLQueLock));
		TCPMNG_CONTEXT::s_IDLQue.reserve(TCPMNG_CONTEXT::MAX_IDL_DATA * sizeof(TCPMNG_CONTEXT*));
		_TRACE("\r\n%s:%ld TCPMNG_CONTEXT::s_hHeap = 0x%x sizeof(TCPMNG_CONTEXT*) = %ld"
			, __FILE__, __LINE__, TCPMNG_CONTEXT::s_hHeap, sizeof(TCPMNG_CONTEXT*));

		//CONNECT_CONTEXT
		CONNECT_CONTEXT::s_hHeap = HeapCreate(0, CONNECT_CONTEXT::BUF_SIZE, CONNECT_CONTEXT::HEAP_SIZE);
		InitializeCriticalSection(&(CONNECT_CONTEXT::s_IDLQueLock));

		//ACCEPT_CONTEXT
		ACCEPT_CONTEXT::s_hHeap = HeapCreate(0, ACCEPT_CONTEXT::BUF_SIZE, ACCEPT_CONTEXT::HEAP_SIZE);
		InitializeCriticalSection(&(ACCEPT_CONTEXT::s_IDLQueLock));
		ACCEPT_CONTEXT::s_IDLQue.reserve(500 * sizeof(ACCEPT_CONTEXT*));
		_TRACE("\r\n%s:%ld nACCEPT_CONTEXT::s_hHeap = 0x%x", __FILE__, __LINE__, ACCEPT_CONTEXT::s_hHeap);

		//TCP_RCV_DATA
		TCP_RCV_DATA::s_hHeap = HeapCreate(0, 0, TCP_RCV_DATA::HEAP_SIZE);
		TCP_RCV_DATA::s_DataHeap = HeapCreate(0, 0, TCP_RCV_DATA::DATA_HEAP_SIZE);
		InitializeCriticalSection(&(TCP_RCV_DATA::s_IDLQueLock));
		TCP_RCV_DATA::s_IDLQue.reserve(TCP_RCV_DATA::MAX_IDL_DATA * sizeof(TCP_RCV_DATA*));

		_TRACE("\r\n%s:%ld TCP_RCV_DATA::s_hHeap = 0x%x", __FILE__, __LINE__, TCP_RCV_DATA::s_hHeap);
		_TRACE("\r\n%s:%ld TCP_RCV_DATA::s_DataHeap = 0x%x", __FILE__, __LINE__, TCP_RCV_DATA::s_DataHeap);

		//TCP_SEND_DATA
		TCP_SEND_DATA::s_hHeap = HeapCreate(0, 0, TCP_SEND_DATA::SEND_DATA_HEAP_SIZE);
		TCP_SEND_DATA::s_DataHeap = HeapCreate(0, 0, TCP_SEND_DATA::DATA_HEAP_SIZE);
		InitializeCriticalSection(&(TCP_SEND_DATA::s_IDLQueLock));
		TCP_SEND_DATA::s_IDLQue.reserve(TCP_SEND_DATA::MAX_IDL_DATA * sizeof(TCP_SEND_DATA*));

		_TRACE("\r\n%s:%ld TCP_SEND_DATA::s_hHeap = 0x%x", __FILE__, __LINE__, TCP_SEND_DATA::s_hHeap);
		_TRACE("\r\n%s:%ld TCP_SEND_DATA::s_DataHeap = 0x%x", __FILE__, __LINE__, TCP_SEND_DATA::s_DataHeap);

		//TCP_CONNECT
		TCP_CONTEXT::s_hHeap = HeapCreate(0, 0, TCP_CONTEXT::TCP_CONTEXT_HEAP_SIZE);
		InitializeCriticalSection(&(TCP_CONTEXT::s_IDLQueLock));
		TCP_CONTEXT::s_IDLQue.reserve(TCP_CONTEXT::MAX_IDL_DATA * sizeof(TCP_CONTEXT*));
	}

	void CTcpMng::ReleaseReource()
	{
		//TCPMNG_CONTEXT
		HeapDestroy(TCPMNG_CONTEXT::s_hHeap);
		TCPMNG_CONTEXT::s_hHeap = NULL;

		EnterCriticalSection(&(TCPMNG_CONTEXT::s_IDLQueLock));
		TCPMNG_CONTEXT::s_IDLQue.clear();
		LeaveCriticalSection(&(TCPMNG_CONTEXT::s_IDLQueLock));
		DeleteCriticalSection(&(TCPMNG_CONTEXT::s_IDLQueLock));

		//CONNECT_CONTEXT
		HeapDestroy(CONNECT_CONTEXT::s_hHeap);
		CONNECT_CONTEXT::s_hHeap = NULL;

		EnterCriticalSection(&(CONNECT_CONTEXT::s_IDLQueLock));
		CONNECT_CONTEXT::s_IDLQue.clear();
		LeaveCriticalSection(&(CONNECT_CONTEXT::s_IDLQueLock));

		DeleteCriticalSection(&(CONNECT_CONTEXT::s_IDLQueLock));

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

		//TCP_SEND_DATA
		HeapDestroy(TCP_SEND_DATA::s_hHeap);
		TCP_SEND_DATA::s_hHeap = NULL;

		HeapDestroy(TCP_SEND_DATA::s_DataHeap);
		TCP_SEND_DATA::s_DataHeap = NULL;

		EnterCriticalSection(&(TCP_SEND_DATA::s_IDLQueLock));
		TCP_SEND_DATA::s_IDLQue.clear();
		LeaveCriticalSection(&(TCP_SEND_DATA::s_IDLQueLock));
		DeleteCriticalSection(&(TCP_SEND_DATA::s_IDLQueLock));

		//TCP_CONTEXT
		HeapDestroy(TCP_CONTEXT::s_hHeap);
		TCP_CONTEXT::s_hHeap = NULL;

		EnterCriticalSection(&(TCP_CONTEXT::s_IDLQueLock));
		TCP_CONTEXT::s_IDLQue.clear();
		LeaveCriticalSection(&(TCP_CONTEXT::s_IDLQueLock));
		DeleteCriticalSection(&(TCP_CONTEXT::s_IDLQueLock));
	}
}