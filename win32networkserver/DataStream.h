#ifndef _DATA_STREAM_H
#define _DATA_STREAM_H
#include <string>

#ifdef WIN32
#ifdef _DLL_EXPORTS_
#define DLLENTRY __declspec(dllexport)
#include <windows.h>
#else
#define DLLENTRY __declspec(dllimport)
#endif		//#ifdef _DLL_EXPORTS_
#else
#define DLLENTRY  
#endif		//#ifdef WIN32

using namespace std;

namespace HelpMng
{

	//===============================================
	//Name : class CDataStream
	//Desc : 本类采用虚拟内存的方式来实现流
	//===============================================

	class DLLENTRY CDataStream
	{
	public:
		//===============================================
		//Name : CDataStream()
		//Desc : 构造一个流, 该流的初始最大容量为1个内存页(4K/8K)的大小
		//	此种方式的流可以进行扩容, 每次的扩容容量一个内存页的大小
		//  当扩容后流的缓冲区基地址可能会发生改变
		//===============================================
		CDataStream(void);

		//===============================================
		//Name : CDataStream_AWE()
		//Desc : 构造一个指定缓冲的流, 此种方式构造的流不能进行扩容
		//===============================================
		CDataStream(LPVOID pBuf, LONG lMaxSize, long lEndPos = 0);

		//===============================================
		//Name : GetState()
		//Desc : 获取流的状态
		//===============================================
		INT GetState()const { return m_iState; }

		//===============================================
		//Name : GetMaxSize()
		//Desc : 获取流的最大容量
		//===============================================
		INT GetMaxSize()const { return m_lMaxSize; }

		//===============================================
		//Name : GetCurSize()
		//Desc : 返回流的当前容量
		//===============================================
		INT GetCurSize()const { return m_lEndPos; }

		//===============================================
		//Name : GetBuf()
		//Desc : 获取流的缓冲区的地址
		//===============================================
		char* GetBuf()const { return m_pBuf; }

		//===============================================
		//Name : ZeroBuf()
		//Desc : 对流的数据进行清0
		//===============================================
		void ZeroBuf();

		//===============================================
		//Name : SeekBegin()
		//Desc : 定位到流的起始位置
		//===============================================
		void SeekBegin() { m_lCurPos = 0; }

		//===============================================
		//Name : SeekEnd()
		//Desc : 定位到流的结束位置
		//===============================================
		void SeekEnd() { m_lCurPos = m_lEndPos; }

		//===============================================
		//Name : Seek()
		//Desc : 定位到流的某个位置
		//===============================================
		void Seek(LONG lPos)
		{
			if (lPos < 0)
			{
				lPos = 0;
			}
			if (lPos > m_lEndPos)
			{
				lPos = m_lEndPos;
			}
			m_lCurPos = lPos;
		}


		/************************************************************************
		* Desc : 在流中保留出一段区域, 并将流的位置定位到保留区域以后
		* Return : 保留成功返回true; 否则返回false
		************************************************************************/
		bool Reserve(long lSize);

		//===============================================
		//Name : operator <<
		//Desc : 相关的输入操作
		//===============================================
		CDataStream& operator << (CHAR c);
		CDataStream& operator << (BYTE b);
		CDataStream& operator << (INT i);
		CDataStream& operator << (LONG l);
		CDataStream& operator << (UINT ui);
		CDataStream& operator << (ULONG ul);
		CDataStream& operator << (const CHAR* szStr);
		CDataStream& operator << (double d);
		CDataStream& operator << (WORD w);
		CDataStream& operator << (bool b);

		//===============================================
		//Name : operator >>
		//Desc : 相关的输出操作
		//===============================================
		CDataStream& operator >> (CHAR& c);
		CDataStream& operator >> (BYTE& b);
		CDataStream& operator >> (INT& i);
		CDataStream& operator >> (LONG& l);
		CDataStream& operator >> (UINT& ui);
		CDataStream& operator >> (ULONG& ul);
		CDataStream& operator >> (string& str);
		CDataStream& operator >> (const char *&szStr);		//无需为其申请空间
		CDataStream& operator >> (double& d);
		CDataStream& operator >> (WORD& w);
		CDataStream& operator >> (bool& b);

		//将流中的数据填入相关的数据缓冲区
		BOOL FillBuf(void *pBuf, int nLen);

		//将缓冲区中的数据放入流中
		BOOL SetBuf(const char *pBuf, int nLen);

		~CDataStream(void);

		enum
		{
			STATE_NOERROR,			//流的状态正常
			STATE_ALLOC_FIAL,		//申请内存失败
			STATE_BUFF_NULL,		//流的缓冲区为空
			STATE_MEM_MAX,			//流的内存容量已达到最大, 无法进行扩容
			STATE_INCREMENT_FAIL,	//扩容失败
			STATE_OUT_END,			//已经到达流的尾部
			STATE_UNKNOW_ERROR,		//未知错误
		};

	protected:
		//===============================================
		//Name : GetBlockSize()
		//Desc : 获取流的数据块的大小
		//===============================================
		static DWORD GetBlockSize();

		//===============================================
		//Name : Increment()
		//Desc : 当进行输入操作，缓冲区数据不够时则进行扩容
		//===============================================
		BOOL Increment(LONG lSize = 0);

		enum
		{
			INCREMENT_MAX_SIZE = 4096 * 1024,	//流的最大扩充容量
		};

		CHAR* m_pBuf;						//数据流的缓冲区基址
		const BOOL m_bIncrement;			//是否对流进行扩容
		static const DWORD s_lBlockSize;	//每次申请的数据块的大小
		LONG m_lMaxSize;					//流的最大容量
		LONG m_lEndPos;						//流的相对结束位置
		LONG m_lCurPos;						//流的当前位置
		INT m_iState;						//流的当前状态
	};

}

#endif		//#ifndef _DATA_STREAM_H
