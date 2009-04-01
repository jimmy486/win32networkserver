#ifndef _TCP_SER_EX_H
#define _TCP_SER_EX_H

#include "HeadFile.h"
#include "NetWork.h"

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

#pragma warning (disable:4251)

#include <vector>

using namespace std;

namespace HelpMng
{
	class TcpSerEx;
	void ReleaseTcpSerEx();

	class TCP_CONTEXT_EX : public NET_CONTEXT
	{
		friend class TcpSerEx;
	protected:
		DWORD m_nDataLen;		//TCP模式下为累计接收或发送的数据长度

		TCP_CONTEXT_EX()
			: m_nDataLen(0)
		{

		}
		virtual ~TCP_CONTEXT_EX() {}

		void* operator new(size_t nSize);
		void operator delete(void* p);

		enum
		{
			E_TCP_HEAP_SIZE = 1024 * 1024* 10,
			MAX_IDL_DATA = 20000,
		};
	private:
		static vector<TCP_CONTEXT_EX* > s_IDLQue;		//无效的数据队列
		static CRITICAL_SECTION s_IDLQueLock;		//访问s_IDLQue的互斥锁
		static HANDLE s_hHeap;	//NET_CONTEXT自己的堆内存
	};

	/************************************************************************
	* Name: class ACCEPT_CONTEXT
	* Desc: 本类主要处理listen socket的网络缓冲区
	************************************************************************/
	class ACCEPT_CONTEXT_EX : public NET_CONTEXT
	{
		friend class TcpSerEx;
	protected:
		SOCKET m_hRemoteSock;			//连接到服务器的SOCKET

		ACCEPT_CONTEXT_EX()
			: m_hRemoteSock(INVALID_SOCKET)
		{
		}

		virtual ~ACCEPT_CONTEXT_EX() {}

		void* operator new(size_t nSize);
		void operator delete(void* p);
	private:
		static vector<ACCEPT_CONTEXT_EX* > s_IDLQue;		//无效的数据队列
		static CRITICAL_SECTION s_IDLQueLock;			//访问s_IDLQue的互斥锁
		static HANDLE s_hHeap;	//NET_CONTEXT自己的堆内存, 其容量为800K, 既最多可申请20000万多个NET_CONTEXT变量
	};

	class DLLENTRY TCP_RCV_DATA_EX
	{
		friend class TcpSerEx;
	public:
		SOCKET m_hSocket;		//收到数据的socket
		CHAR* m_pData;			//数据缓冲区
		INT m_nLen;				//数据缓冲区的长度

		TCP_RCV_DATA_EX(SOCKET hSock, const CHAR* pBuf, INT nLen);
		~TCP_RCV_DATA_EX();

		void* operator new(size_t nSize);
		void operator delete(void* p);

		enum
		{
			HEAP_SIZE = 1024 *1024* 50,		//自定义堆的最大容量
			DATA_HEAP_SIZE = 1024 *1024 * 100,
			MAX_IDL_DATA = 100000,
		};

	private:
		static vector<TCP_RCV_DATA_EX* > s_IDLQue;		//无效的数据队列
		static CRITICAL_SECTION s_IDLQueLock;		//访问s_IDLQue的互斥锁
		static HANDLE s_hHeap;		//Rcv_DATA的自定义堆
		static HANDLE s_DataHeap;	//数据缓冲区的自定义堆, 最大容量为10M
	};

	class DLLENTRY TcpSerEx : public NetWork
	{
		friend void ReleaseTcpSerEx();
	public:
		/****************************************************
		* Name : InitReource()
		* Desc : 对相关全局及静态资源进行初始化
		*	该方法由DllMain()方法调用, 开发者不能调用
		*	该方法; 否则会造成内存泄露
		****************************************************/
		static void InitReource();

		TcpSerEx(
			LPCLOSE_ROUTINE pCloseFun		//某个socket关闭的回调函数
			, void *pParam					//回调函数的pParam参数
			);
		virtual ~TcpSerEx();

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
		* Desc : TCP模式的发送数据
		****************************************************/
		virtual BOOL SendData(const char * szData, int nDataLen, SOCKET hSock = INVALID_SOCKET);

		/****************************************************
		* Name : GetRecvData()
		* Desc : 从接收队列中取出一个接收数据
		****************************************************/
		virtual void *GetRecvData(DWORD* const pQueLen);

	protected:
		enum
		{
			LISTEN_EVENTS = 2,	                          //监听socket的事件个数
			THREAD_NUM = 2,							//辅助线程的数目
			MAX_ACCEPT = 50,
			_SOCK_NO_RECV = 0xf0000000,       //socket已连接但未接收数据
			_SOCK_RECV = 0xf0000001               //socket已连接并且也接收了数据
		};

		vector<TCP_RCV_DATA_EX * > m_RcvDataQue;        //接受数据缓冲区队列
		CRITICAL_SECTION m_RcvQueLock;					//访问s_RcvDataQue的互斥锁

		vector<SOCKET> m_SocketQue;							//连接到服务器的客户端socket队列
		CRITICAL_SECTION m_SockQueLock;				//访问m_SocketQue的互斥锁

		LPCLOSE_ROUTINE m_pCloseFun;					//网络关闭的回调通知函数
		LPVOID m_pCloseParam;							//传递给m_pCloseFun的参数

		long volatile m_bThreadRun;								//是否允许后台线程是否继续运行
		long volatile m_nReadCount;				//读操作计数器
		long volatile m_nAcceptCount;				//accept的投递计数器
		BOOL m_bSerRun;									//服务是否运行

		//用于投递accept的事件对象
		HANDLE m_ListenEvents[LISTEN_EVENTS];
		HANDLE m_Threads[THREAD_NUM];	               //辅助线程和监听线程

		static LPFN_ACCEPTEX s_pfAcceptEx;				//AcceptEx函数地址
		static LPFN_GETACCEPTEXSOCKADDRS s_pfGetAddrs;	//GetAcceptExSockaddrs函数的地址

		/****************************************************
		* Name : IOCompletionProc()
		* Desc : IO完成后的回调函数
		****************************************************/
		virtual void IOCompletionProc(BOOL bSuccess, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);

		/****************************************************
		* Name : AcceptCompletionProc()
		* Desc : 当有客户端连接到服务器时调用此方法
		****************************************************/
		void AcceptCompletionProc(BOOL bSuccess, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);

		/****************************************************
		* Name : RecvCompletionProc()
		* Desc : 网络接口上读到数据时调用此方法
		****************************************************/
		void RecvCompletionProc(BOOL bSuccess, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);

		/****************************************************
		* Name : SendCompletionProc()
		* Desc : 发送操作完成时调用此方法
		****************************************************/
		void SendCompletionProc(BOOL bSuccess, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);

		/****************************************************
		* Name : ListenThread()
		* Desc : 网络监听线程
		****************************************************/
		static UINT WINAPI ListenThread(LPVOID lpParam);

		/****************************************************
		* Name : AideThread()
		* Desc : 后台辅助线程, 该线程暂时无用
		****************************************************/
		static UINT WINAPI AideThread(LPVOID lpParam);
	private:
		static BOOL s_bInit;				//是否初始化完成

		/****************************************************
		* Name : ReleaseReource()
		* Desc : 对相关全局和静态资源进行释放
		****************************************************/
		static void ReleaseReource();
	};
}

#endif		//#ifndef _TCP_SER_EX_H