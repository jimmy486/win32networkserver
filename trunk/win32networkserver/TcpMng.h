#ifndef _TCP_MNG_H
#define _TCP_MNG_H

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

#include <vector>
#include <map>

using namespace std;

namespace HelpMng
{
	class CTcpMng;
	class SOCKET_CONTEXT;
    
	typedef map<SOCKET, SOCKET_CONTEXT*> CSocketMap;

	class TCP_CONTEXT : public NET_CONTEXT
	{
		friend class CTcpMng;
	protected:
		DWORD m_nDataLen;		//TCP模式下为累计接收或发送的数据长度
		CTcpMng* const m_pMng;

		TCP_CONTEXT(CTcpMng* pMng)
			: m_pMng(pMng)
			, m_nDataLen(0)
		{

		}
		virtual ~TCP_CONTEXT() {}

		void* operator new(size_t nSize);
		void operator delete(void* p);

		enum
		{
			E_TCP_HEAP_SIZE = 1024 * 1024* 2,
			MAX_IDL_DATA = 40000,
		};
	private:
		static vector<TCP_CONTEXT* > s_IDLQue;		//无效的数据队列
		static CRITICAL_SECTION s_IDLQueLock;		//访问s_IDLQue的互斥锁
		static HANDLE s_hHeap;	//NET_CONTEXT自己的堆内存, 其容量为800K, 既最多可申请20000万多个NET_CONTEXT变量

		//判断该类的某个指针对象是否有效
		static BOOL IsAddressValid(LPCVOID lpMem);
	};

	/************************************************************************
	* Name: class ACCEPT_CONTEXT
	* Desc: 本类主要处理listen socket的网络缓冲区
	************************************************************************/
	class ACCEPT_CONTEXT : public NET_CONTEXT
	{
		friend class CTcpMng;

	protected:
		SOCKET m_hRemoteSock;			//连接到服务器的SOCKET
		CTcpMng *const m_pMng;

		ACCEPT_CONTEXT(CTcpMng* pMng)
			: m_pMng(pMng) 
			, m_hRemoteSock(INVALID_SOCKET)
		{

		}

		virtual ~ACCEPT_CONTEXT() {}

		void* operator new(size_t nSize);
		void operator delete(void* p);
	private:
		static vector<ACCEPT_CONTEXT* > s_IDLQue;		//无效的数据队列
		static CRITICAL_SECTION s_IDLQueLock;			//访问s_IDLQue的互斥锁
		static HANDLE s_hHeap;	//NET_CONTEXT自己的堆内存, 其容量为800K, 既最多可申请20000万多个NET_CONTEXT变量

		//判断该类的某个指针对象是否有效
		static BOOL IsAddressValid(LPCVOID lpMem);
	};

	class DLLENTRY TCP_RCV_DATA
	{
		friend class CTcpMng;

	public:
		SOCKET_CONTEXT *m_pSocket;	//收到数据的socket
		CHAR* m_pData;			//数据缓冲区, 优化时可采用虚拟内存的方式实现
		INT m_nLen;				//数据缓冲区的长度

		TCP_RCV_DATA(SOCKET_CONTEXT *pSocket, const CHAR* pBuf, INT nLen);
		~TCP_RCV_DATA();

		void* operator new(size_t nSize);
		void operator delete(void* p);

		static BOOL IsAddressValid(LPCVOID pMem);
		BOOL IsDataValid();

		enum
		{
			HEAP_SIZE = 1024 *1024* 5,		//自定义堆的最大容量
			DATA_HEAP_SIZE = 1024 *1024 * 50,
			MAX_IDL_DATA = 50000,
		};

	private:
		static vector<TCP_RCV_DATA* > s_IDLQue;		//无效的数据队列
		static CRITICAL_SECTION s_IDLQueLock;		//访问s_IDLQue的互斥锁
		static HANDLE s_hHeap;		//Rcv_DATA的自定义堆, 最多可以处理20万个TCP_RCV_DATA对象
		static HANDLE s_DataHeap;	//数据缓冲区的自定义堆, 最大容量为10M
	};
	
	class DLLENTRY SOCKET_CONTEXT
	{
		friend class CTcpMng;
	public:
		const IP_ADDR &GetPeerAddr()const
		{
			return m_Addr;
		}

	protected:
		IP_ADDR m_Addr;				//该socket所对应的对端地址
		DWORD m_nInteractiveTime;	//socket的上次交互时间
		SOCKET m_hSocket;			//相关的网络接口

		SOCKET_CONTEXT(const IP_ADDR& peer_addr, const SOCKET &sock);
		~SOCKET_CONTEXT() {}

		void* operator new(size_t nSize);
		void operator delete(void* p);

		enum
		{
			HEAP_SIZE = 500 *1024,
			MAX_IDL_SIZE = 20000,
		};
	private:
		static vector<SOCKET_CONTEXT*> s_IDLQue;
		static CRITICAL_SECTION s_IDLQueLock;
		static HANDLE s_hHeap;			//自定义数据堆
	};

	class DLLENTRY CTcpMng
	{
	public:
		CTcpMng(void);
		~CTcpMng(void);


		/************************************************************************
		* Desc : 初始化静态资源，在申请TCP实例对象之前应先调用该函数, 否则程序无法正常运行
		************************************************************************/
		static void InitReource();

		/************************************************************************
		* Desc : 在释放TCP实例以后, 掉用该函数释放相关静态资源
		************************************************************************/
		static void ReleaseReource();

		//启动一个TCP服务, 服务器启动成功返回TRUE, 否则返回FALSE
		BOOL StartServer(
			INT nPort
			, LPCLOSE_ROUTINE pCloseFun		//某个socket关闭的回调函数
			, LPVOID pParam					//回调函数的pParam参数
			);

		//关闭服务
		void CloseServer();
		//将nDataLen长度的数据szData从指定的网络接口sock上发出去数据的顺序问题有上层控制
		//注：系统不负责对szData的释放, 释放操作由用户自己完成. 当操作返回以后便可进行释放操作
		// nDataLen的长度不要超过4094
		BOOL SendData(SOCKET_CONTEXT *const pSocket, const CHAR* szData, INT nDataLen);

		//从接收数据对列中获取一个接收数据包; pQueLen 不为空时将传出数据队列的长度.	
		//注：当用户使用完数据包后应将其释放掉, 以防止内存的泄漏
		TCP_RCV_DATA* GetRcvData(
			DWORD* const pQueLen
			, const SOCKET_CONTEXT *pSocket = NULL		//要在哪个socket上获取数据 只对字符串格式的数据有效
			);

	protected:
		vector<TCP_RCV_DATA* > m_RcvDataQue;			//接受数据缓冲区队列
		CRITICAL_SECTION m_RcvQueLock;					//访问s_RcvDataQue的互斥锁

		vector<ACCEPT_CONTEXT* > m_AcceptQue;			//监听SOCKET的网络监听队列

		//CSendMap m_SendDataMap;							//发送数据的MAP
		//CRITICAL_SECTION m_SendDataMapLock;				//访问m_SendDataMapLock的互斥锁

		CSocketMap m_SocketMap;							//客户端的socket相关map
		CSocketMap::iterator m_SockIter;				//访问m_SocketMap的遍历器
		CRITICAL_SECTION m_SockMapLock;					//访问m_SocketMap的互斥锁

		LPCLOSE_ROUTINE m_pCloseFun;					//网络关闭的回调通知函数
		LPVOID m_pCloseParam;							//传递给m_pCloseFun的参数
		static LPFN_ACCEPTEX s_pfAcceptEx;				//AcceptEx函数地址
		static LPFN_CONNECTEX s_pfConnectEx;			//ConnectEx函数的地址
		static LPFN_GETACCEPTEXSOCKADDRS s_pfGetAddrs;	//GetAcceptExSockaddrs函数的地址

		SOCKET m_hSock;									//负责监听的socket
		BOOL m_bThreadRun;								//是否允许后台线程是否继续运行

		//将数据压入接收数据队列中
		void PushInRecvQue(TCP_RCV_DATA* const pRcvData);

		//当收到或发送交互报文需要修改交互时间
		void ModifyInteractiveTime(const SOCKET& s);

		//将新连接到的socket压入到socket map中
		inline void PushInSocketMap(const SOCKET& s, SOCKET_CONTEXT *pSocket);

		//从socket队列中删除相关的socket的上下文
		void DelSocketContext(const SOCKET &s);

		//IO线程池处理函数
		static void CALLBACK IOCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);

		//当AcceptEx操作完成所需要进行的处理
		static void AcceptCompletionProc(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);

		//当WSARecv操作完成所需要进行的处理
		static void RecvCompletionProc(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);

		//当WSASend操作完成所需要进行的处理
		static void SendCompletionProc(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);

		//后台线程处理函数
		static UINT WINAPI ThreadProc(LPVOID lpParam);

		//获取一个有效的socket的上下文
		SOCKET_CONTEXT *GetSocketContext(const SOCKET &sock);
	};
}

#endif			//#ifndef _TCP_MNG_H
