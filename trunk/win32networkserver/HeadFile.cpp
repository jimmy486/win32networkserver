#include "HeadFile.h"
#include <assert.h>
#include <string>
#include <vector>

using namespace std;

namespace HelpMng
{

	HANDLE NET_CONTEXT::s_hDataHeap = NULL;
	vector<char * > NET_CONTEXT::s_IDLQue;
	CRITICAL_SECTION NET_CONTEXT::s_IDLQueLock;
	DWORD NET_CONTEXT::S_PAGE_SIZE = NET_CONTEXT::GetSysMemPageSize();

	void NET_CONTEXT::InitReource()
	{
		s_hDataHeap = HeapCreate(0, S_PAGE_SIZE, 1024 * 1024 * 500);
		InitializeCriticalSection(&s_IDLQueLock);
		s_IDLQue.reserve(5000 * sizeof(void*));		
	}

	void NET_CONTEXT::ReleaseReource()
	{
		HeapDestroy(s_hDataHeap);
		s_hDataHeap = NULL;

		EnterCriticalSection(&(s_IDLQueLock));
		s_IDLQue.clear();
		LeaveCriticalSection(&(s_IDLQueLock));
		DeleteCriticalSection(&(s_IDLQueLock));
	}

	NET_CONTEXT::NET_CONTEXT()
		: m_hSock(INVALID_SOCKET)
		, m_pBuf(NULL)
		, m_nOperation(0)
	{
		assert(0 != S_PAGE_SIZE);
		//为pBuf申请虚拟内存
		//m_pBuf = (CHAR*)VirtualAlloc(NULL, S_PAGE_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		assert(s_hDataHeap);

		EnterCriticalSection(&s_IDLQueLock);

		vector<char* >::iterator iter = s_IDLQue.begin();
		if (s_IDLQue.end() != iter)
		{
			m_pBuf = *iter;
			s_IDLQue.erase(iter);
		}
		else
		{
			m_pBuf = (char *)HeapAlloc(s_hDataHeap, HEAP_ZERO_MEMORY | HEAP_NO_SERIALIZE, S_PAGE_SIZE);
		}

		LeaveCriticalSection(&s_IDLQueLock);

		assert(m_pBuf);
		memset(&m_ol, 0, sizeof(m_ol));
	}

	NET_CONTEXT::~NET_CONTEXT()
	{
		if (m_pBuf)
		{
			EnterCriticalSection(&s_IDLQueLock);
			const DWORD QUE_SIZE = (DWORD)(s_IDLQue.size());

			if (QUE_SIZE <= 5000)
			{
				s_IDLQue.push_back(m_pBuf);
			}
			else
			{
				HeapFree(s_hDataHeap, HEAP_NO_SERIALIZE, m_pBuf);
			}
			LeaveCriticalSection(&s_IDLQueLock);
		}
		m_pBuf = NULL;
	}

	void* NET_CONTEXT:: operator new(size_t nSize)
	{
		void* pContext = NULL;

		//try
		//{
		//	//还没有对象被申请过，需要先创建堆
		//	if (0 == InterlockedExchangeAdd(&s_nCount, 1))
		//	{			
		//		s_hHeap = HeapCreate(0, BUF_SIZE, HEAP_SIZE);
		//	}
		//	if (NULL == s_hHeap)
		//	{
		//		THROW_LINE;
		//	}

		//	//申请堆内存
		//	pContext = HeapAlloc(s_hHeap, HEAP_ZERO_MEMORY, nSize);
		//	if (NULL == pContext)
		//	{
		//		THROW_LINE;
		//	}
		//}
		//catch(...)
		//{
		//	InterlockedExchangeAdd(&s_nCount, -1);
		//	pContext = NULL;
		//}

		return pContext;
	}

	void NET_CONTEXT:: operator delete(void* p)
	{
		//if (IsAddressValid(p))
		//{
		//	HeapFree(s_hHeap, 0, p);
		//	InterlockedExchangeAdd(&s_nCount, -1);

		//	//已经没有对象再被释放需要将堆释放
		//	if (0 >= s_nCount)
		//	{
		//		HeapDestroy(s_hHeap);
		//		s_hHeap = NULL;
		//	}
		//}

		return;
	}

	DWORD NET_CONTEXT::GetSysMemPageSize()
	{
		SYSTEM_INFO sys_info;
		GetSystemInfo(&sys_info);
		return sys_info.dwPageSize;
	}
	//BOOL NET_CONTEXT::IsAddressValid(LPCVOID lpMem)
	//{
	//	BOOL bResult = HeapValidate(s_hHeap, 0, lpMem);

	//	if (NULL == lpMem)
	//	{
	//		bResult = FALSE;
	//	}	

	//	return bResult;
	//}


	//IP_ADDR的实现
	IP_ADDR::IP_ADDR(const char* szIP, INT nPort)
	{
		sin_family = AF_INET;
		sin_port = htons(nPort);
		sin_addr.s_addr = inet_addr(szIP);
	}

	IP_ADDR::IP_ADDR(const IP_ADDR& other)
	{
		if (this != &other)
		{
			sin_family = other.sin_family;
			sin_port = other.sin_port;
			sin_addr.s_addr = other.sin_addr.s_addr;
			memcpy(sin_zero, other.sin_zero, sizeof(sin_zero));
		}
	}

	IP_ADDR& IP_ADDR::operator =(const IP_ADDR& other)
	{
		if (this != &other)
		{
			sin_family = other.sin_family;
			sin_port = other.sin_port;
			sin_addr.s_addr = other.sin_addr.s_addr;
			memcpy(sin_zero, other.sin_zero, sizeof(sin_zero));
		}

		return *this;
	}

	IP_ADDR& IP_ADDR::operator =(const sockaddr_in& other)
	{
		if (this != &other)
		{
			sin_family = other.sin_family;
			sin_port = other.sin_port;
			sin_addr.s_addr = other.sin_addr.s_addr;
			memcpy(sin_zero, other.sin_zero, sizeof(sin_zero));
		}

		return *this;
	}

	bool IP_ADDR::operator ==(const IP_ADDR &other)const
	{
		bool bResult = FALSE;
		if (sin_family == other.sin_family && sin_port == other.sin_port && sin_addr.s_addr == other.sin_addr.s_addr)
		{
			bResult = TRUE;
		}

		return bResult;
	}

	bool IP_ADDR::operator ==(const sockaddr_in& other)const
	{
		bool bResult = FALSE;
		if (sin_family == other.sin_family && sin_port == other.sin_port && sin_addr.s_addr == other.sin_addr.s_addr)
		{
			bResult = TRUE;
		}

		return bResult;
	}

	bool IP_ADDR::operator <(const IP_ADDR& other)const
	{
		bool bResult = FALSE;
		if (sin_port < other.sin_port && sin_addr.s_addr < other.sin_addr.s_addr)
		{
			bResult = TRUE;
		}

		return bResult;
	}

	bool IP_ADDR::operator <(const sockaddr_in &other) const
	{
		bool bResult = FALSE;
		if (sin_port < other.sin_port && sin_addr.s_addr < other.sin_addr.s_addr)
		{
			bResult = TRUE;
		}

		return bResult;
	}

	void GetAddress(const IP_ADDR& addr, void *pSzIp , int& nPort)
	{
		string *pIp = (string *)pSzIp;
		*pIp	= inet_ntoa(addr.sin_addr);
		nPort = ntohs(addr.sin_port);
	}

	BOOL GetLocalIps(void *pIpList)
	{
		BOOL bRet = TRUE;
		char szName[MAX_PATH] = { 0 };
		if (gethostname(szName, sizeof(szName -1)))
		{
			return FALSE;
		}

		HOSTENT *pHost = gethostbyname(szName);
		if (NULL == pHost)
		{
			return FALSE;
		}

		vector<string> *plist = (vector<string> *)pIpList;
		string szIp;
		for (int index = 0; ; index++)
		{
			szIp = inet_ntoa(*(IN_ADDR *)pHost->h_addr_list[index]);
			(*plist).push_back(szIp);

			if (pHost->h_addr_list[index] + pHost->h_length >= pHost->h_name )
			{
				break;
			}
		}

		return TRUE;
	}
}
