#ifndef _TCP_SER_H_
#define _TCP_SER_H_
#include "HeadFile.h"

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
	class TcpServer;

	class TCP_CONTEXT : public NET_CONTEXT
	{
		friend class TcpServer;
	protected:
		DWORD m_nDataLen;		//TCP模式下为累计接收或发送的数据长度

		TCP_CONTEXT()
			: m_nDataLen(0)
		{

		}
		virtual ~TCP_CONTEXT() {}

		void* operator new(size_t nSize);
		void operator delete(void* p);

		enum
		{
			E_TCP_HEAP_SIZE = 1024 * 1024* 10,
			MAX_IDL_DATA = 20000,
		};
	private:
		static vector<TCP_CONTEXT* > s_IDLQue;		//无效的数据队列
		static CRITICAL_SECTION s_IDLQueLock;		//访问s_IDLQue的互斥锁
		static HANDLE s_hHeap;	//NET_CONTEXT自己的堆内存
	};

	/************************************************************************
	* Name: class ACCEPT_CONTEXT
	* Desc: 本类主要处理listen socket的网络缓冲区
	************************************************************************/
	class ACCEPT_CONTEXT : public NET_CONTEXT
	{
		friend class TcpServer;
	protected:
		SOCKET m_hRemoteSock;			//连接到服务器的SOCKET

		ACCEPT_CONTEXT()
			: m_hRemoteSock(INVALID_SOCKET)
		{

		}

		virtual ~ACCEPT_CONTEXT() {}

		void* operator new(size_t nSize);
		void operator delete(void* p);
	private:
		static vector<ACCEPT_CONTEXT* > s_IDLQue;		//无效的数据队列
		static CRITICAL_SECTION s_IDLQueLock;			//访问s_IDLQue的互斥锁
		static HANDLE s_hHeap;	//NET_CONTEXT自己的堆内存, 其容量为800K, 既最多可申请20000万多个NET_CONTEXT变量
	};

	class DLLENTRY TCP_RCV_DATA
	{
		friend class TcpServer;
	public:
		SOCKET m_hSocket;		//收到数据的socket
		CHAR* m_pData;			//数据缓冲区, 优化时可采用虚拟内存的方式实现
		INT m_nLen;				//数据缓冲区的长度

		TCP_RCV_DATA(SOCKET hSock, const CHAR* pBuf, INT nLen);
		~TCP_RCV_DATA();

		void* operator new(size_t nSize);
		void operator delete(void* p);

		enum
		{
			HEAP_SIZE = 1024 *1024* 50,		//自定义堆的最大容量
			DATA_HEAP_SIZE = 1024 *1024 * 100,
			MAX_IDL_DATA = 100000,
		};

	private:
		static vector<TCP_RCV_DATA* > s_IDLQue;		//无效的数据队列
		static CRITICAL_SECTION s_IDLQueLock;		//访问s_IDLQue的互斥锁
		static HANDLE s_hHeap;		//Rcv_DATA的自定义堆
		static HANDLE s_DataHeap;	//数据缓冲区的自定义堆, 最大容量为10M
	};
	
	class DLLENTRY TcpServer
	{
	public:
		TcpServer();
		~TcpServer();

		/************************************************************************
		* Desc : 初始化静态资源，在申请TCP实例对象之前应先调用该函数, 否则程序无法正常运行
		************************************************************************/
		static void InitReource();

		/************************************************************************
		* Desc : 在释放TCP实例以后, 掉用该函数释放相关静态资源
		************************************************************************/
		static void ReleaseReource();

		/****************************************************
		* Name : StartServer()
		* Desc : 启动服务
		****************************************************/
		BOOL StartServer(
			const char *szIp	//要启动服务的本地地址, NULL 则采用默认地址
			, INT nPort	//要启动服务的端口
			, LPCLOSE_ROUTINE pCloseFun		//某个socket关闭的回调函数
			, LPVOID pParam					//回调函数的pParam参数
			);

		/****************************************************
		* Name : CloseServer()
		* Desc : 关闭服务
		****************************************************/
		void CloseServer();

		/****************************************************
		* Name : SendData()
		* Desc : 将nDataLen长度的数据szData从指定的网络接口sock上发出去数据的顺序问题有上层控制
		* 注：系统不负责对szData的释放, 释放操作由用户自己完成. 当操作返回以后便可进行释放操作
		 * nDataLen的长度不要超过4096
		****************************************************/
		BOOL SendData(SOCKET hSock, const CHAR* szData, INT nDataLen);

		/****************************************************
		* Name : GetRcvData()
		* Desc : 从接收数据对列中获取一个接收数据包; pQueLen 不为空时将传出数据队列的长度.	
		* 注：当用户使用完数据包后应将其释放掉, 以防止内存的泄漏
		****************************************************/
		TCP_RCV_DATA* GetRcvData(
			DWORD* const pQueLen
			);
	protected:
		enum
		{
			LISTEN_EVENTS = 2,					//监听socket的事件个数
			MAX_ACCEPT = 50,
			_SOCK_NO_RECV = 0xf0000000,		//socket已连接但未接收数据
			_SOCK_RECV = 0xf0000001				//socket已连接并且也接收了数据
		};

		vector<TCP_RCV_DATA* > m_RcvDataQue;			//接受数据缓冲区队列
		CRITICAL_SECTION m_RcvQueLock;					//访问s_RcvDataQue的互斥锁

		vector<SOCKET> m_SocketQue;							//连接到服务器的客户端socket队列
		CRITICAL_SECTION m_SockQueLock;				//访问m_SocketQue的互斥锁

		LPCLOSE_ROUTINE m_pCloseFun;					//网络关闭的回调通知函数
		LPVOID m_pCloseParam;							//传递给m_pCloseFun的参数

		SOCKET m_hSock;									//负责监听的socket
		long volatile m_bThreadRun;								//是否允许后台线程是否继续运行
		long volatile m_nAcceptCount;				//accept的投递计数器
		BOOL m_bSerRun;									//服务是否运行

		//用于投递accept的事件对象
		HANDLE m_ListenEvents[LISTEN_EVENTS];
		HANDLE *m_pThreads;				//线程数组
		HANDLE m_hCompletion;					//完成端口句柄

		static LPFN_ACCEPTEX s_pfAcceptEx;				//AcceptEx函数地址
		static LPFN_GETACCEPTEXSOCKADDRS s_pfGetAddrs;	//GetAcceptExSockaddrs函数的地址


		/****************************************************
		* Name : PushInRecvQue()
		* Desc : 将接受到的数据放入接受队列中
		****************************************************/
		void PushInRecvQue(TCP_RCV_DATA* const pRcvData);

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
		* Name : WorkThread()
		* Desc : I/O 后台管理线程
		****************************************************/
		static UINT WINAPI WorkThread(LPVOID lpParam);

		/****************************************************
		* Name : AideThread()
		* Desc : 后台辅助线程, 该线程暂时无用
		****************************************************/
		static UINT WINAPI AideThread(LPVOID lpParam);
	};
}

#endif