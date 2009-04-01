#include "BufMng.h"
#include <assert.h>

namespace HelpMng
{
	unsigned long BufMng::PAGE_SIZE = 4096;

	char *BufMng::_new(int nSize /* = 1  */)
	{
		char *pBuf = NULL;
		const DWORD ALLOC_SIZE = (((nSize -1) /PAGE_SIZE +1) *PAGE_SIZE == 0 ? PAGE_SIZE : ((nSize -1) /PAGE_SIZE +1) *PAGE_SIZE);
		pBuf = (char *)VirtualAlloc(NULL, ALLOC_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

		return pBuf;
	}

	void BufMng::_delete(void* p)
	{
		if (p)
		{
			VirtualFree(p, 0, MEM_RELEASE);
		}
	}

	BufData::BufData()
	{
		pBufData = NULL;
	}

	BufData::BufData(char *szBuf)
	{
		assert(szBuf);
		pBufData = szBuf;
	}

	BufData::~BufData()
	{
		if (pBufData)
		{
			BufMng::_delete(pBufData);
		}
	}

	BufData &BufData::operator =(char *szBuf)
	{
		assert(szBuf);
		if (pBufData != szBuf)
		{
			if (pBufData)
			{
				BufMng::_delete(pBufData);
			}

			pBufData = szBuf;
		}

		return *this;
	}
}