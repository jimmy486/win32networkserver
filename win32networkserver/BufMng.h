#ifndef _BUF_MNG_H
#define _BUF_MNG_H

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

//本模块主要处理缓冲区的管理
namespace HelpMng
{

	//该类不需要有实例对象
	class DLLENTRY BufMng
	{	
	public:
		/************************************************************************
		* Desc : 申请一块新的缓冲区
		* Return : 申请成功返回相应的地址, 失败则返回NULL
		************************************************************************/
		static char* _new(
			int nSize = 1		//要申请的缓冲区容量
			);

		/************************************************************************
		* Desc : 当缓冲区用完需要进行释放
		* Return : 
		************************************************************************/
		static void _delete(void* p);

	private:
		BufMng() {}
		~BufMng() {}

		enum
		{
			BUF_CAP = 1024 * 1024 *10,		//缓冲区的总容量
		};

		static unsigned long PAGE_SIZE;		//一个内存页的大小
	};

	//配合BufMng的使用
	class DLLENTRY BufData
	{
	public:
		BufData();
		~BufData();
		
		/************************************************************************
		* Desc : szBuf 须由BufMng生成
		************************************************************************/
		BufData(char *szBuf);

		/************************************************************************
		* Desc : 赋值操作
		************************************************************************/
		BufData &operator = (char *szBuf);

        
		char *pBufData;					//由BufMng生成的数据空间
	};
}

#endif			//#ifndef _BUF_MNG_H