#ifndef _UDP_SER_EX_H
#define _UDP_SER_EX_H
#include "HeadFile.h"
#include "NetWork.h"

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
	void ReleaseUdpSerEx();

	class UDP_CONTEXT_EX : protected NET_CONTEXT
	{
		friend class UdpSerEx;
	protected:
		IP_ADDR m_RemoteAddr;			//对端地址

		enum
		{
			HEAP_SIZE = 1024 * 1024 * 5,
			MAX_IDL_DATA = 10000,
		};

	public:
		UDP_CONTEXT_EX() {}
		virtual ~UDP_CONTEXT_EX() {}

		void* operator new(size_t nSize);
		void operator delete(void* p);

	private:
		static vector<UDP_CONTEXT_EX* > s_IDLQue;
		static CRITICAL_SECTION s_IDLQueLock;
		static HANDLE s_hHeap;	
	};

	class DLLENTRY UDP_RCV_DATA_EX
	{
		friend class UdpSerEx;
	public:
		CHAR* m_pData;				//数据缓冲区
		INT m_nLen;					//数据的长度
		IP_ADDR m_PeerAddr;			//发送报文的地址

		UDP_RCV_DATA_EX(const CHAR* szBuf, int nLen, const IP_ADDR& PeerAddr);
		~UDP_RCV_DATA_EX();

		void* operator new(size_t nSize);
		void operator delete(void* p);

		enum
		{
			RCV_HEAP_SIZE = 1024 * 1024 *50,		//s_Heap堆的大小
			DATA_HEAP_SIZE = 100 * 1024* 1024,	//s_DataHeap堆的大小
			MAX_IDL_DATA = 250000,
		};

	private:
		static vector<UDP_RCV_DATA_EX* > s_IDLQue;
		static CRITICAL_SECTION s_IDLQueLock;
		static HANDLE s_DataHeap;		//数据缓冲区的堆
		static HANDLE s_Heap;			//RCV_DATA的堆
	};

	//客户端的UDP网络层也使用该类实现
	class DLLENTRY UdpSerEx : public NetWork
	{
		friend void ReleaseUdpSerEx();
	public:
		/****************************************************
		* Name : InitReource()
		* Desc : 对相关全局及静态资源进行初始化
		*	该方法由DllMain()方法调用, 开发者不能调用
		*	该方法; 否则会造成内存泄露
		****************************************************/
		static void InitReource();

		//	* 注: 请不要在pCloseFun中做耗时的操作, 否则会使IO线程阻塞
		UdpSerEx(
			LPCLOSE_ROUTINE pCloseFun		//某个socket关闭的回调函数
			, void *pParam					//回调函数的pParam参数
			, BOOL bClient = FALSE	//该UDP网络模块是否为客户端网络模块
			);
		virtual ~UdpSerEx();

		/****************************************************
		* Name : Init()
		* Desc : 对网络模块进行初始化	
		****************************************************/
		virtual BOOL Init(
			const char *szIp	//要启动服务的本地地址, NULL 则采用默认地址
			, int nPort	//要启动服务的端口
			);

		/****************************************************
		* Name : Close()
		* Desc : 关闭网络模块
		****************************************************/
		virtual void Close();

		/****************************************************
		* Name : SendData()
		* Desc : UDP模式的数据发送
		****************************************************/
		virtual BOOL SendData(const IP_ADDR& PeerAddr, const char * szData, int nLen);

		/****************************************************
		* Name : GetRecvData()
		* Desc : 从接收队列中取出一个接收数据
		****************************************************/
		virtual void *GetRecvData(DWORD* const pQueLen);

		void * operator new(size_t nSize);
		void operator delete(void* p);		

		void *operator new(size_t nSize, const char *, long)
		{
			return operator new(nSize);
		}
		void operator delete(void *p, const char *, long)
		{
			operator delete(p);
		}

		void *operator new[](size_t nSize)
		{
			return operator new(nSize);
		}
		void operator delete[](void *p)
		{
			operator delete(p);
		}

	protected:
		BOOL m_bSerRun;											//服务器是否正在运行
		const BOOL c_bClient;												//该UDP网络实例是否为客户端
		long volatile m_lReadCount;								//读操作计数器
		vector<UDP_RCV_DATA_EX* > m_RcvDataQue;				//接收数据队列
		CRITICAL_SECTION m_RcvDataLock;						//访问m_RcvDataQue的互斥锁

		LPCLOSE_ROUTINE m_pCloseProc;                         //socket关闭通知函数
		void *m_pCloseParam;

		void ReadCompletion(BOOL bSuccess, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);

		/****************************************************
		* Name : IOCompletionProc()
		* Desc : IO完成后的回调函数
		****************************************************/
		virtual void IOCompletionProc(BOOL bSuccess, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);

	private:
		static HANDLE s_hHeap;			//用于申请对象实例的堆
		static vector<UdpSerEx*> s_IDLQue;		//空闲的UdpSerEx队列
		static CRITICAL_SECTION s_IDLQueLock;	//访问s_IDLQue数据队列互斥锁
		static BOOL s_bInit;			//是否已经初始化

		enum
		{
			E_BUF_SIZE = 4096,
			E_HEAP_SIZE = 800 *1024,		 //对象实例的容量
			E_MAX_IDL_NUM = 2000,			 //空闲实例队列的最大长度
		};

		/****************************************************
		* Name : ReleaseReource()
		* Desc : 对相关全局和静态资源进行释放
		****************************************************/
		static void ReleaseReource();
	};
}

#endif			//#ifndef _UDP_SER_EX_H