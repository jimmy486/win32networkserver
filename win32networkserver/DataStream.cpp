#include ".\datastream.h"

#ifdef WIN32
#include <Windows.h>
#include <atltrace.h>

#ifdef _DEBUG
#define _TRACE ATLTRACE
#else
#define _TRACE 
#endif		//#ifdef _DEBUG
#else
#define _TRACE 

#endif		//#ifdef WIN32

#define THROW_LINE (throw ((long)(__LINE__)))

namespace HelpMng
{
	const DWORD CDataStream::s_lBlockSize = CDataStream::GetBlockSize();
    
	CDataStream::CDataStream(void)
		: m_pBuf(NULL)
		, m_bIncrement(TRUE)
		, m_lMaxSize(0)
		, m_lEndPos(0)
		, m_lCurPos(0)
		, m_iState(STATE_NOERROR)
	{
		try
		{
#ifdef WIN32
			m_pBuf = (CHAR*)VirtualAlloc(NULL, s_lBlockSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
			m_pBuf = new char[s_lBlockSize];
#endif
			if (NULL == m_pBuf)
			{
				m_iState = STATE_ALLOC_FIAL;

				THROW_LINE;
			}		
			m_lMaxSize += s_lBlockSize;
		}
		catch (const long& iErrCode)
		{			
			_TRACE("Exp : %s--%ld", __FILE__, iErrCode);
			//TRACE1STR("CDataStream", 0, "申请虚拟内存失败...");
		}
	}

	CDataStream::CDataStream(LPVOID pBuf, LONG lMaxSize, long lEndPos)
		: m_pBuf(NULL)
		, m_lMaxSize(lMaxSize)
		, m_bIncrement(FALSE)
		, m_lCurPos(0)
		, m_lEndPos(lEndPos)
		, m_iState(STATE_NOERROR)
	{
		try
		{
			m_pBuf = (CHAR*)pBuf;
			if (NULL == m_pBuf)
			{
				m_iState = STATE_BUFF_NULL;
				THROW_LINE;
			}
		}
		catch (const long& iErrCode)
		{
			_TRACE("Exp : %s--%ld", __FILE__, iErrCode);
		}
	}

	CDataStream::~CDataStream(void)
	{
		//只有可扩充的流才允许释放
		if (m_pBuf && m_bIncrement)
		{
#ifdef WIN32
			VirtualFree(m_pBuf, 0, MEM_RELEASE);
#else
			delete []m_pBuf;
#endif
			m_pBuf = NULL;
		}
	}

	void CDataStream::ZeroBuf()
	{
		memset(m_pBuf, 0, m_lEndPos);
		m_lCurPos = 0;
		m_lEndPos = 0;
		m_iState = STATE_NOERROR;
	}

	//===============================================
	//Name : GetBlockSize()
	//Desc : 获取流的数据块的大小
	//===============================================
	DWORD CDataStream::GetBlockSize()
	{	
#ifdef WIN32
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		return sysInfo.dwPageSize;
#else
		return 4096;
#endif
	}

	BOOL CDataStream::Increment(LONG lSize)
	{
		BOOL bRet = TRUE;
		m_iState = STATE_NOERROR;

		if (FALSE == m_bIncrement)
		{
			//TRACE1STR("Increment", 0, "非扩展流不能扩容...");
			return FALSE;
		}

		try
		{
			INT iIncrementSize = 0;
			if (m_lMaxSize >= INCREMENT_MAX_SIZE)
			{
				m_iState = STATE_MEM_MAX;
				THROW_LINE;
			}
			if (lSize > (LONG)s_lBlockSize)
			{
				iIncrementSize = m_lMaxSize + (lSize / s_lBlockSize +1) *s_lBlockSize;
			}
			else
			{
				iIncrementSize = m_lMaxSize + s_lBlockSize;
			}

#ifdef WIN32
			CHAR* pData = (CHAR*)VirtualAlloc(NULL, iIncrementSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
			CHAR* pData = new char[iIncrementSize];
#endif
			if (NULL == pData)
			{
				m_iState = STATE_ALLOC_FIAL;
				THROW_LINE;
			}
			memcpy(pData, m_pBuf, m_lMaxSize);

#ifdef WIN32
			VirtualFree(m_pBuf, 0, MEM_RELEASE);
#else
			delete []m_pBuf;
#endif

			m_pBuf = pData;
			m_lMaxSize = iIncrementSize;
		}
		catch (const long& iErrCode)
		{
			bRet = FALSE;
			_TRACE("Exp : %s--%ld", __FILE__, iErrCode);
		}
		return bRet;
	}

	CDataStream& CDataStream:: operator << (CHAR c)
	{
		try
		{
			m_iState = STATE_NOERROR;
			if (m_lCurPos + (LONG)sizeof(c) > m_lMaxSize)
			{
				if (FALSE == Increment())
				{
					m_iState = STATE_INCREMENT_FAIL;
					THROW_LINE;
				}
			}

			memcpy(m_pBuf +m_lCurPos, &c, sizeof(c));
			m_lCurPos += sizeof(c);
			if (m_lCurPos > m_lEndPos)
			{
				m_lEndPos = m_lCurPos;
			}
		}
		catch (const long& iErrCode)
		{
			_TRACE("Exp : %s--%ld", __FILE__, iErrCode);
			//TRACE1NUM("operator << (CHAR c)", 0, m_iState);
		}

		return *this;
	}

	CDataStream& CDataStream::operator << (BYTE b)
	{
		return operator << ((CHAR)b);
	}

	CDataStream& CDataStream::operator << (bool b)
	{
		return operator << ((char)b);
	}

	CDataStream& CDataStream::operator << (WORD w)
	{
		try
		{
			m_iState = STATE_NOERROR;
			//检查释放需要对流进行扩容
			if (m_lCurPos + (LONG)sizeof(w) > m_lMaxSize)
			{
				if (FALSE == Increment())
				{
					m_iState = STATE_INCREMENT_FAIL;
					THROW_LINE;
				}
			}

			memcpy(m_pBuf +m_lCurPos, &w, sizeof(w));
			m_lCurPos += sizeof(w);
			if (m_lCurPos > m_lEndPos)
			{
				m_lEndPos = m_lCurPos;
			}
		}
		catch (const long& iErrCode)
		{
			_TRACE("Exp : %s--%ld", __FILE__, iErrCode);
			//TRACE1NUM("operator << (WORD w)", 0, m_iState);
		}

		return *this;

	}

	CDataStream& CDataStream::operator << (LONG l)
	{
		try
		{
			m_iState = STATE_NOERROR;
			//检查释放需要对流进行扩容
			if (m_lCurPos + (LONG)sizeof(l) > m_lMaxSize)
			{
				if (FALSE == Increment())
				{
					m_iState = STATE_INCREMENT_FAIL;
					THROW_LINE;
				}
			}

			memcpy(m_pBuf +m_lCurPos, &l, sizeof(l));
			m_lCurPos += sizeof(l);
			if (m_lCurPos > m_lEndPos)
			{
				m_lEndPos = m_lCurPos;
			}
		}
		catch (const long& iErrCode)
		{
			_TRACE("Exp : %s--%ld", __FILE__, iErrCode);
			//TRACE1NUM("operator << (LONG l)", 0, m_iState);
		}

		return *this;
	}

	CDataStream& CDataStream::operator << (INT i)
	{
		return operator << ((LONG)i);
	}

	CDataStream& CDataStream::operator << (UINT ui)
	{
		return operator << ((LONG)ui);
	}

	CDataStream& CDataStream::operator << (ULONG ul)
	{
		return operator << ((LONG)ul);
	}

	CDataStream& CDataStream::operator << (const CHAR* str)
	{
		try
		{
			m_iState = STATE_NOERROR;
			const LONG nStrLen = (LONG)strlen(str) + 1;
			//检查释放需要对流进行扩容
			if (m_lCurPos + nStrLen >= m_lMaxSize)
			{
				if (FALSE == Increment(nStrLen))
				{
					m_iState = STATE_INCREMENT_FAIL;
					THROW_LINE;
				}
			}

			strcpy(m_pBuf +m_lCurPos, str);
			m_lCurPos += nStrLen;
			if (m_lCurPos > m_lEndPos)
			{
				m_lEndPos = m_lCurPos;
			}
		}
		catch (const long& iErrCode)
		{
			_TRACE("Exp : %s--%ld", __FILE__, iErrCode);
			//TRACE1NUM("operator << (CHAR* sz)", 0, m_iState);
		}
		return *this;
	}

	CDataStream& CDataStream::operator << (double d)
	{
		try
		{
			m_iState = STATE_NOERROR;
			//检查释放需要对流进行扩容
			if (m_lCurPos + (LONG)sizeof(d) > m_lMaxSize)
			{
				if (FALSE == Increment())
				{
					m_iState = STATE_INCREMENT_FAIL;
					THROW_LINE;
				}
			}

			memcpy(m_pBuf +m_lCurPos, &d, sizeof(d));
			m_lCurPos += sizeof(d);
			if (m_lCurPos > m_lEndPos)
			{
				m_lEndPos = m_lCurPos;
			}
		}
		catch (const long& iErrCode)
		{
			_TRACE("Exp : %s--%ld", __FILE__, iErrCode);
			//TRACE1NUM("operator << (double d)", 0, m_iState);
		}

		return *this;
	}

	CDataStream& CDataStream::operator >> (CHAR& c)
	{
		try
		{
			m_iState = STATE_NOERROR;
			//判断是否已到达流的尾部
			if (m_lCurPos + (LONG)sizeof(c) > m_lEndPos)
			{
				m_iState = STATE_OUT_END;
				THROW_LINE;
			}
			memcpy(&c, m_pBuf +m_lCurPos, sizeof(c));
			m_lCurPos += sizeof(c);
		}
		catch (const long& iErrCode)
		{
			_TRACE("Exp : %s--%ld", __FILE__, iErrCode);
			//TRACE1NUM("operator >> (CHAR& c)", 0, m_iState);
		}

		return *this;
	}

	CDataStream& CDataStream::operator >> (BYTE& b)
	{
		return operator >> ((CHAR&)b);
	}

	CDataStream& CDataStream::operator >> (bool& b)
	{
		return operator >> ((CHAR&)b);
	}

	CDataStream& CDataStream::operator >> (WORD& w)
	{
		try
		{
			m_iState = STATE_NOERROR;
			//判断是否已到达流的尾部
			if (m_lCurPos + (LONG)sizeof(w) > m_lEndPos)
			{
				m_iState = STATE_OUT_END;
				THROW_LINE;
			}
			memcpy(&w, m_pBuf +m_lCurPos, sizeof(w));
			m_lCurPos += sizeof(w);
		}
		catch (const long& iErrCode)
		{
			_TRACE("Exp : %s--%ld", __FILE__, iErrCode);
			//TRACE1NUM("operator >> (WORD& )", 0, m_iState);
		}

		return *this;
	}

	CDataStream& CDataStream::operator >> (LONG& l)
	{
		try
		{
			m_iState = STATE_NOERROR;
			//判断是否已到达流的尾部
			if (m_lCurPos + (LONG)sizeof(l) > m_lEndPos)
			{
				m_iState = STATE_OUT_END;
				THROW_LINE;
			}
			memcpy(&l, m_pBuf +m_lCurPos, sizeof(l));
			m_lCurPos += sizeof(l);
		}
		catch (const long& iErrCode)
		{
			_TRACE("Exp : %s--%ld", __FILE__, iErrCode);
			//TRACE1NUM("operator >> (INT& i)", 0, m_iState);
		}

		return *this;
	}

	CDataStream& CDataStream::operator >> (INT& i)
	{
		return operator >> ((LONG&)i);
	}

	CDataStream& CDataStream::operator >> (UINT& ui)
	{
		return operator >> ((LONG&)ui);
	}

	CDataStream& CDataStream::operator >> (ULONG& ul)
	{
		return operator >> ((LONG&)ul);
	}

	CDataStream& CDataStream::operator >> (string &str)
	{
		try
		{
			m_iState = STATE_NOERROR;
			//判断是否已到达流的尾部
			if (m_lCurPos > m_lEndPos)
			{
				m_iState = STATE_OUT_END;
				THROW_LINE;
			}
			//str.Format("%s", m_pBuf +m_lCurPos);
			str = (CHAR*)(m_pBuf +m_lCurPos);
			m_lCurPos += (LONG)(str.length() +1);
		}
		catch (const long& iErrCode)
		{
			_TRACE("Exp : %s--%ld", __FILE__, iErrCode);
			//TRACE1NUM("operator >> (CString& str)", 0, m_iState);
		}

		return *this;
	}

	CDataStream &CDataStream::operator >>(const char *&szStr)
	{
		try
		{
			m_iState = STATE_NOERROR;
			//判断是否已到达流的尾部
			if (m_lCurPos > m_lEndPos)
			{
				m_iState = STATE_OUT_END;
				THROW_LINE;
			}
			//str.Format("%s", m_pBuf +m_lCurPos);
			//str = (CHAR*)(m_pBuf +m_lCurPos);
			szStr = m_pBuf +m_lCurPos;
			m_lCurPos += (LONG)(strlen(szStr) +1);
		}
		catch (const long& iErrCode)
		{
			_TRACE("Exp : %s--%ld", __FILE__, iErrCode);
			//TRACE1NUM("operator >> (CString& str)", 0, m_iState);
		}

		return *this;
	}

	CDataStream& CDataStream::operator >> (double& d)
	{
		try
		{
			m_iState = STATE_NOERROR;
			//判断是否已到达流的尾部
			if (m_lCurPos + (LONG)sizeof(d) > m_lEndPos)
			{
				m_iState = STATE_OUT_END;
				THROW_LINE;
			}
			memcpy(&d, m_pBuf +m_lCurPos, sizeof(d));
			m_lCurPos += sizeof(d);
		}
		catch (const long& iErrCode)
		{
			_TRACE("Exp : %s--%ld", __FILE__, iErrCode);
			//TRACE1NUM("operator >> (double& d)", 0, m_iState);
		}

		return *this;
	}

	BOOL CDataStream::FillBuf(void *pBuf, int nLen)
	{
		if (m_lCurPos + nLen > m_lEndPos)
		{
			return FALSE;
		}

		memcpy(pBuf, m_pBuf + m_lCurPos, nLen);
		m_lCurPos += nLen;

		return TRUE;
	}

	BOOL CDataStream::SetBuf(const char *pBuf, int nLen)
	{
		BOOL bRet = TRUE;
		try
		{
			m_iState = STATE_NOERROR;

			//检查释放需要对流进行扩容
			if (m_lCurPos + nLen > m_lMaxSize)
			{
				if (FALSE == Increment(nLen))
				{
					m_iState = STATE_INCREMENT_FAIL;
					THROW_LINE;
				}
			}

			memcpy(m_pBuf +m_lCurPos, pBuf, nLen);
			m_lCurPos += nLen;
			if (m_lCurPos > m_lEndPos)
			{
				m_lEndPos = m_lCurPos;
			}
		}
		catch (const long& iErrCode)
		{
			bRet = FALSE;
			_TRACE("Exp : %s--%ld", __FILE__, iErrCode);
			//TRACE1NUM("operator << (CHAR* sz)", 0, m_iState);
		}

		return bRet;
	}

	bool CDataStream::Reserve(long lSize)
	{
		bool bRet = true;

		try
		{
			m_iState = STATE_NOERROR;
			//检查释放需要对流进行扩容
			if (m_lCurPos + lSize > m_lMaxSize)
			{
				if (FALSE == Increment())
				{
					m_iState = STATE_INCREMENT_FAIL;
					THROW_LINE;
				}
			}

			memset(m_pBuf +m_lCurPos, 0, lSize);
			m_lCurPos += lSize;
			if (m_lCurPos > m_lEndPos)
			{
				m_lEndPos = m_lCurPos;
			}
		}
		catch (const long& iErrCode)
		{
			bRet = false;
			_TRACE("Exp : %s--%ld", __FILE__, iErrCode);
			//TRACE1NUM("operator << (WORD w)", 0, m_iState);
		}
		return bRet;
	}
}