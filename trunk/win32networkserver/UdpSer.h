#ifndef _UDP_SER_H_
#define _UDP_SER_H_
#include "HeadFile.h"

#pragma warning (disable:4251)

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

#include<vector>

using namespace std;

namespace HelpMng
{
	class UDP_CONTEXT : protected NET_CONTEXT
	{
		friend class UdpSer;
	protected:
		IP_ADDR m_RemoteAddr;			//对端地址

		enum
		{
			HEAP_SIZE = 1024 * 1024 * 5,
			MAX_IDL_DATA = 10000,
		};

	public:
		UDP_CONTEXT() {}
		virtual ~UDP_CONTEXT() {}

		void* operator new(size_t nSize);
		void operator delete(void* p);

	private:
		static vector<UDP_CONTEXT* > s_IDLQue;
		static CRITICAL_SECTION s_IDLQueLock;
		static HANDLE s_hHeap;	
	};

	class DLLENTRY UDP_RCV_DATA
	{
		friend class UdpSer;
	public:
		CHAR* m_pData;				//数据缓冲区
		INT m_nLen;					//数据的长度
		IP_ADDR m_PeerAddr;			//发送报文的地址

		UDP_RCV_DATA(const CHAR* szBuf, int nLen, const IP_ADDR& PeerAddr);
		~UDP_RCV_DATA();

		void* operator new(size_t nSize);
		void operator delete(void* p);

		enum
		{
			RCV_HEAP_SIZE = 1024 * 1024 *50,		//s_Heap堆的大小
			DATA_HEAP_SIZE = 100 * 1024* 1024,	//s_DataHeap堆的大小
			MAX_IDL_DATA = 250000,
		};

	private:
		static vector<UDP_RCV_DATA* > s_IDLQue;
		static CRITICAL_SECTION s_IDLQueLock;
		static HANDLE s_DataHeap;		//数据缓冲区的堆
		static HANDLE s_Heap;			//RCV_DATA的堆
	};

	class DLLENTRY UdpSer
	{
	public:
		UdpSer();
		~UdpSer();

		/************************************************************************
		* Desc : 初始化静态资源，在申请UDP实例对象之前应先调用该函数, 否则程序无法正常运行
		************************************************************************/
		static void InitReource();

		/************************************************************************
		* Desc : 在释放UDP实例以后, 掉用该函数释放相关静态资源
		************************************************************************/
		static void ReleaseReource();

		//用指定本地地址和端口进行初始化
		BOOL StartServer(const CHAR* szIp = "0.0.0.0", INT nPort = 0);

		//从数据队列的头部获取一个接收数据, pCount不为null时返回队列的长度
		UDP_RCV_DATA* GetRcvData(DWORD* pCount);

		//向对端发送数据
		BOOL SendData(const IP_ADDR& PeerAddr, const CHAR* szData, INT nLen);

		/****************************************************
		* Name : CloseServer()
		* Desc : 关闭服务器
		****************************************************/
		void CloseServer();

	protected:
		SOCKET m_hSock;
		vector<UDP_RCV_DATA* > m_RcvDataQue;				//接收数据队列
		CRITICAL_SECTION m_RcvDataLock;						//访问m_RcvDataQue的互斥锁
		long volatile m_bThreadRun;								//是否允许后台线程继续运行
		BOOL m_bSerRun;											//服务器是否正在运行

		HANDLE *m_pThreads;				//线程数组
		HANDLE m_hCompletion;					//完成端口句柄

		void ReadCompletion(BOOL bSuccess, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);

		/****************************************************
		* Name : WorkThread()
		* Desc : I/O 后台管理线程
		****************************************************/
		static UINT WINAPI WorkThread(LPVOID lpParam);
	};
}
#endif		//#ifndef _UDP_SER_H_