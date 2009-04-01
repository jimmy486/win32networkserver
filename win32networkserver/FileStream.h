#ifndef _FILE_STREAM_H
#define _FILE_STREAM_H

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

	/************************************************
	该类主要用于处理文件流的输入输出, 当处理输入性文件时
	必须指定要映射的文件的大小
	************************************************/

	class DLLENTRY CFileStream
	{
	public:
		enum
		{
			MAP_READ,				//只读映射
			MAP_WRITE,				//写方式映射
			MAP_READ_WRITE,			//读写方式映射
		};

		//===============================================
		//Name : CFileStream()
		//Desc : 为指定的文件创建一个指定大小的文件流, 当流的
		//	的指定大小为0时则流的大小和所指定的文件的大小相同
		//Param : strFile[IN] 要映射的文件
		//	lSize[IN] 流的容量, 为0 表示和映射的文件的大小相同
		//	bEndFlag[IN] 文件流是否包含结束字段; 当包含结束字段时
		//	文件流的起始4个字节将被预留出来不进行处理. 该字段由
		//	类本身维护, 结束字段即流的实际大小
		//===============================================
		CFileStream(const char* szFile, ULONG lSize = 0, BOOL bEndFlag = FALSE);
		CFileStream() 			
			: m_lCurrPos(0)
			, m_lEndPos(0)
			, m_nState(0)
			, m_pBuff(NULL)
			, m_hFile(NULL)
			, C_STARTPOS(0)
			, m_lMaxSize(0)
		{
		}

		~CFileStream(void);

		UINT GetState()const { return m_nState; }

		//===============================================
		//Name : GetBuff()
		//Desc : 获取流的缓冲区
		//===============================================
		CHAR* GetBuff()const { return (m_pBuff +C_STARTPOS); }

		//===============================================
		//Name : SeekBegin()
		//Desc : 定位到流的起始位置处
		//===============================================
		void SeekBegin() { m_lCurrPos = C_STARTPOS; }

		//===============================================
		//Name : SeekEnd()
		//Desc : 定位到流的结束位置
		//===============================================
		ULONG SeekEnd() { m_lCurrPos = m_lEndPos; return m_lCurrPos; }

		//===============================================
		//Name : Seek()
		//Desc : 定位流到指定的位置
		//===============================================
		void Seek(ULONG ulPos);

		/************************************************************************
		* Desc : 若流的当前存放的是字符串, 则跳过该字符串
		* Return : 返回所跳过的字符串的长度, 该长度包括字符串的结束符
		************************************************************************/
		DWORD SkipString();

		/************************************************************************
		* Desc : 设置流的当前位置指针跳过一定的位置
		************************************************************************/
		void Skip(DWORD nLen);

		//===============================================
		//Name : GetCurrBuff()
		//Desc : 获取当前位置处的缓冲区
		//===============================================
		CHAR* GetCurrBuff()const { return (m_pBuff + m_lCurrPos); }

		//===============================================
		//Name : GetSize()
		//Desc : 获取流的当前实际容量, 只针对有结束标志的流有效
		//===============================================
		ULONG GetSize()const { return (m_lEndPos - C_STARTPOS); }

		/************************************************************************
		* Desc : 获取文件流的最大长度
		************************************************************************/
		DWORD GetFileLen()const { return m_lMaxSize; }

		//===============================================
		//Name : FillBuff()
		//Desc : 将指定长度的缓冲区数据填入到文件的指定位置中
		// 如果lPos = -1; 则在文件的当前位置进行填入
		//===============================================
		void FillBuff(LONG lPos, LPVOID pBuf, UINT nSize);

		/************************************************************************
		* Desc : 将文件当前位置起的内容填到指定的缓冲区中
		* Return : 操作成功返回true, 否则返回false
		************************************************************************/
		BOOL SetBuf(void *pBuf, int nSize);

		CFileStream& operator << (const INT& i);
		CFileStream& operator << (const LONG& l);
		CFileStream& operator << (const UINT& ui);
		CFileStream& operator << (const ULONG& ul);
		CFileStream& operator << (const CHAR* szSTR);
		CFileStream& operator << (const double& d);

		CFileStream& operator >> (INT& i);
		CFileStream& operator >> (LONG& l);
		CFileStream& operator >> (UINT& ui);
		CFileStream& operator >> (ULONG& ul);
		CFileStream& operator >> (string& str);
		CFileStream& operator >> (const char *&szStr);
		CFileStream& operator >> (double& d);

		//===============================================
		//Name : 对指定位置数据生成32位的CRC校验码
		//Desc : 生成的CRC校验码
		//===============================================
		DWORD MakeCrc32();

		//===============================================
		//Name : MapFile()
		//Desc : 对文件进行映射
		//===============================================
		BOOL MapFile(
			const char* szFile
			, ULONG lSize = 0
			, BOOL bEndFlag = FALSE
			, int nMapType = MAP_READ_WRITE			//映射方式
			);

		/************************************************************************
		* Desc : 取消对文件的映射
		************************************************************************/
		void UnMapFile();

		//===============================================
		//Name : MakeCrc32()
		//Desc : 对特定的数据生成32位的crc校验码
		//===============================================
		static DWORD MakeCrc32(const char *pszData, int nLen);

		/************************************************************************
		* Desc : 对文件的缓冲区进行清零操作
		************************************************************************/
		void ZeroBuf();

		enum
		{
			STATE_NORMAL,					//流的当前状态正常
			STATE_OPEN_FILE_FAIL,			//打开文件失败
			STATE_STREAM_SIZE_ERROR,		//流的容量有误
			STATE_END_STREAM,				//已到达流的尾部
			STATE_INCREMENT_FIAL,			//扩容失败
		};

	protected:
		//===============================================
		//Name : SetEndPos()
		//Desc : 设置流的结束位置, 当每次进行文件输入操作以后都需设置次标志
		// 只针对有结束标志字段的流有效
		//===============================================
		inline void SetEndPos();

		//===============================================
		//Name : IncrementFile()
		//Desc : 当文件的输入空间不够时, 对文件进行扩容
		//Param : nSize[IN] : 要扩容的字节数
		//		bType[IN] : 是否按固定字节进行扩容; bType = TRUE
		//		按指定字节进行扩容, 即当nSize < FILE_INCREMENT时
		//		仍扩容nSize个字节, bType = FALSE 按固定字节进行扩容
		//		即当nSize < FILE_INCREMENT时仍扩容FILE_INCREMENT个字节
		//===============================================
		inline BOOL IncrementFile(UINT nSize = FILE_INCREMENT, BOOL bType = FALSE);

		enum
		{
			FILE_INCREMENT = 1024,			//文件增量, 当进行文件的输入操作达到文件的末尾时需对文件进行扩充
		};

		CHAR* m_pBuff;			//映射文家的缓冲区
		ULONG m_lCurrPos;		//文件流的当前位置		
		ULONG m_lEndPos;		//流的相对结束位置
		ULONG C_STARTPOS;		//流的起始位置
		ULONG m_lMaxSize;		//该文件的最大容量	
		UINT m_nState;			//流的状态
		HANDLE m_hFile;
	};

}

#endif  //#ifndef _FILE_STREAM_H