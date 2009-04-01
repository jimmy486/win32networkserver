#ifndef _UDP_MNG_H
#define _UDP_MNG_H

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

#include<queue>
#include<vector>
#include<map>

using namespace std;

namespace HelpMng
{

	class CUdpMng;

	class UDP_CONTEXT : protected NET_CONTEXT
	{
		friend class CUdpMng;
	protected:
		IP_ADDR m_RemoteAddr;			//对端地址
		CUdpMng* const m_pMng;			//与UDP_CONTEXT相关的CUdpMng对象实例
		enum
		{
			HEAP_SIZE = 1024 * 1024 * 5,
			MAX_IDL_DATA = 100000,
		};

	public:
		UDP_CONTEXT(CUdpMng* pMng)
			: m_pMng(pMng) {}

		~UDP_CONTEXT() {}

		void* operator new(size_t nSize);
		void operator delete(void* p);

	private:
		static vector<UDP_CONTEXT* > s_IDLQue;
		static CRITICAL_SECTION s_IDLQueLock;
		static HANDLE s_hHeap;	

		//判断该类的某个指针对象是否有效
		static BOOL IsAddressValid(LPCVOID lpMem);
	};

	class DLLENTRY UDP_RCV_DATA
	{
		friend class CUdpMng;
	public:
		CHAR* m_pData;				//数据缓冲区
		INT m_nLen;					//数据的长度
		IP_ADDR m_PeerAddr;			//发送报文的地址

		UDP_RCV_DATA(const CHAR* szBuf, int nLen, const IP_ADDR& PeerAddr);
		~UDP_RCV_DATA();

		void* operator new(size_t nSize);
		void operator delete(void* p);

		//判断RCV_DATA的某个对象指针是否有效
		static BOOL IsAddressValid(LPCVOID pMem);
		//判断m_pData是否有效
		BOOL IsDataVailid();

		enum
		{
			RCV_HEAP_SIZE = 1024 * 1024 *7,		//s_Heap堆的大小
			DATA_HEAP_SIZE = 50 * 1024* 1024,	//s_DataHeap堆的大小
			MAX_IDL_DATA = 250000,
		};

	private:
		static vector<UDP_RCV_DATA* > s_IDLQue;
		static CRITICAL_SECTION s_IDLQueLock;
		static HANDLE s_DataHeap;		//数据缓冲区的堆
		static HANDLE s_Heap;			//RCV_DATA的堆
	};

	class DLLENTRY CUdpMng
	{
	public:	
		CUdpMng();
		~CUdpMng(void);

		/************************************************************************
		* Desc : 初始化静态资源，在申请TCP实例对象之前应先调用该函数, 否则程序无法正常运行
		************************************************************************/
		static void InitReource();

		/************************************************************************
		* Desc : 在释放TCP实例以后, 掉用该函数释放相关静态资源
		************************************************************************/
		static void ReleaseReource();

		//用指定本地地址和端口进行初始化
		BOOL Init(const CHAR* szIp = "0.0.0.0", INT nPort = 0);

		//从数据队列的头部获取一个接收数据, pCount不为null时返回队列的长度
		UDP_RCV_DATA* GetRcvDataFromQue(DWORD* pCount);

		//向对端发送数据
		BOOL SendData(const IP_ADDR& PeerAddr, const CHAR* szData, INT nLen);
	protected:	
		SOCKET m_hSock;
		vector<UDP_RCV_DATA* > m_RcvDataQue;				//接收数据队列
		CRITICAL_SECTION m_RcvDataLock;						//访问m_RcvDataQue的互斥锁

		//IO线程池处理函数
		static void CALLBACK IOCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);
		static void ReadCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);
		static void SendCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);

		//将数据压入到数据包队列中
		void PushInRcvDataQue(const CHAR* szBuf, INT nLen, const IP_ADDR& addr);

		//投递nNum个Recv操作
		void PostRecv(INT nNum);
	};
}

#endif			//#ifndef _UDP_MNG_H