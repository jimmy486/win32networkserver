#ifndef _CLIENT_NET_H
#define _CLIENT_NET_H

#include "HeadFile.h"
#include <vector>
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

using namespace std;

namespace HelpMng
{
	class DLLENTRY ClientNet;

	/************************************************************************
	* Desc : 当网络模块收到数据时会调用该函数处理接收到的数据; TCP方式有效
	* Return : 还需要从网络上接收多少个字节的数据, 返回0表示此次数据已接收完成
	************************************************************************/
	typedef int (CALLBACK *LPONREAD)(
		LPVOID pParam			//该参数由用户通过SetCallbackParam函数设置
		//以下参数由网络模块提供
		, const char* szData	//从网络上接收到的数据缓冲区, 用户不能进行释放操作								
		, int nLen				//数据缓冲区的长度
		);

	/************************************************************************
	* Desc : UDP方式数据接收处理
	* Return : 
	************************************************************************/
	typedef void (CALLBACK *LPONREADFROM)(
		LPVOID pParam				//该参数由用户提供
		//以下参数由网络模块提供
		, const IP_ADDR& PeerAddr	//发送数据的对方地址
		, const char* szData		//数据缓冲区
		, int nLen					//数据缓冲区长度
		);

	/************************************************************************
	* Desc : 当监听到有客户端连接到本服务器时的回调函数
	* Return : 
	************************************************************************/
	typedef void (CALLBACK *LPONACCEPT)(
		LPVOID pParam				//该参数由用户提供
		//以下参数由网络模块提供
		, const SOCKET& sockClient	//连接到本机的客户端socket
		, const IP_ADDR& AddrClient	//连接到本机的客户端地址
		);

	/************************************************************************
	* Desc : 当检测到网络错误,或socket关闭等错误时会回调该函数
	* Return : 
	************************************************************************/
	typedef void (CALLBACK *LPONERROR)(
		LPVOID pParam				//该参书由用户提供
		, int nOperation			//网络模块当前正在进行的操作
		);

	//客户端的网络Context类
	class CLIENT_CONTEXT : public NET_CONTEXT
	{
		friend class ClientNet;
		//friend class Sock5Relay;
	public:
		ClientNet* const m_pClient;		//与此context相关的代理类
		SOCKET m_ClientSock;			//连接到服务器的socket, 用于Accept操作
		IP_ADDR m_PeerIP;				//对端IP地址, 只对UDP方式有效
		volatile LONG m_nPostCount;		//1表明正在进行读写操作, 0表明socket空闲, 若为-1则表明监听socket关闭
		WORD m_nAllLen;					//要处理的数据总长度, TCP有效
		WORD m_nCurrentLen;				//当前已处理的数据长度, TCP有效

		CLIENT_CONTEXT(ClientNet* pClient) 
			: m_pClient(pClient)
			, m_nAllLen (0)
			, m_nCurrentLen (0){};

		virtual ~CLIENT_CONTEXT() {};

		void* operator new (size_t nSize);
		void operator delete(void* p);

		enum
		{
			HEAP_SIZE = 1024 *1024,
			MAX_IDL_NUM = 5000,			//空闲队列的最大长度
		};

	private:
		static HANDLE s_hHeap;			//NET_CONTEXT自己的堆内存, 其容量为800K, 既最多可申请20000万多个NET_CONTEXT变量
		static vector<CLIENT_CONTEXT*> s_IDLQue;	//空闲的CLIENT_TCP_CONTEXT队列
		static CRITICAL_SECTION s_IDLQueLock;			//访问s_IDLQue的数据队列

		//判断该类的某个指针对象是否有效
		static BOOL IsAddressValid(LPCVOID lpMem);
	};

	//发送数据的队列缓冲区
	class CLIENT_SEND_DATA
	{
		friend class ClientNet;
	public:
		CHAR* m_pData;			//数据缓冲区
		INT m_nLen;				//要发送的数据长度
		IP_ADDR m_PeerIP;		//报文的目的地, 只对UDP方式有效

		CLIENT_SEND_DATA(const CHAR* szData, INT nLen);
		CLIENT_SEND_DATA(const CHAR* szData, INT nLen, const IP_ADDR& peer_ip);
		~CLIENT_SEND_DATA();

		void* operator new(size_t nSize);
		void operator delete(void* p);

		static BOOL IsAddressValid(LPCVOID pMem);
		BOOL IsDataValid();

		enum
		{	
			RELAY_HEAP_SIZE = 400 *1024,
			DATA_HEAP_SIZE = 10 *1024* 1024,
			MAX_IDL_NUM = 10000,
		};

	private:
		static HANDLE s_DataHeap;
		static HANDLE s_hHeap;
		static vector<CLIENT_SEND_DATA*> s_IDLQue;		//空闲的CLIENT_SEND_DATA队列
		static CRITICAL_SECTION s_IDLQueLock;				//访问s_IDLQue的数据队列
	};
	
	/************************************************************************
	*Name : class ClientNet
	*Desc : 本类主要用于客户端的网络程序开发
	************************************************************************/
	class DLLENTRY ClientNet
	{

	public:

		/************************************************************************
		* Desc : 初始化静态资源，在申请TCP实例对象之前应先调用该函数, 否则程序无法正常运行
		************************************************************************/
		static void InitReource();

		/************************************************************************
		* Desc : 在释放TCP实例以后, 掉用该函数释放相关静态资源
		************************************************************************/
		static void ReleaseReource();

		ClientNet(
			BOOL bUseCallback		//是否采用回调函数的方式, 否则采用虚函数继承的方式处理
			);
		//通过一个有效的socket创建一个ClientNet对象, 当服务器端监听到一个客户端连接上时调用此函数进行创建
		//即在OnAccept()方法监听到一个连接到来时应该为新到来的socket申请一个新的网络模块对象
		//若初始化失败则GetState()返回STATE_SOCK_CLOSE
		ClientNet(
			const SOCKET& SockClient
			, BOOL bUseCallback		//是否采用回调函数的方式, 否则采用虚函数继承的方式处理
			, LPVOID pParam			//bUseCallback = true时有效
			, LPONREAD pOnRead		//bUseCallback = true时有效
			, LPONERROR pOnError	//bUseCallback = true时有效
			);
		virtual ~ClientNet(void);

		//初始化代理, 后期将不再使用
		//nNetType 取值SOCK_STREAM 或 SOCK_DGRAM
		//virtual BOOL Init(INT nNetType, const CHAR* szSerIP, INT nSerPort);

		//在指定的端口上进行监听, 用于TCP
		BOOL Listen(
			INT nPort				//要监听的本地端口
			, BOOL bReuse			//是否进行地址重用
			, const char* szLocalIp	//要进行重用的本地地址, bReuse=TRUE时有效
			);

		//绑定到指定的端口上, 用于UDP
		BOOL Bind(			
			INT nPort		//本地端口
			, BOOL bReuse	//是否采用地址重用
			, const char* szLocalIp //要进行重用的本地地址, bReuse=TRUE时有效
			);

		//连接到指定的服务器上, 此处采用异步连接
		BOOL Connect(
			const CHAR* szHost	//远程服务器地址
			, INT nPort			//要连接的服务器端口
			, BOOL bReuse		//是否采用地址重用
			, const char* szLocalIp //要进行重用的本地地址, bReuse=TRUE时有效			
			, int nLocalPort	//要重用的本地端口, bReuse=TRUE时有效			
			);

		//关闭网络接口
		void Close();

		//TCP方式的发送数据
		BOOL Send(const CHAR* szData, INT nLen);

		//UDP方式的数据发送
		BOOL SendTo(const IP_ADDR& PeerAddr, const CHAR* szData, INT nLen);

		/************************************************************************
		* Desc : 设置TCP模式的回调函数参数,当采用了回调函数的方式时才应该进行此设置
		*	否则设置无效; 该操作应该在Listen()或Connect()方法之前调用
		* Return : 
		************************************************************************/
		void SetTCPCallback(
			LPVOID pParam
			, LPONACCEPT pOnAccept		//Listen类型的socket有效, connect类型的socket为NULL
			, LPONCONNECT pOnConnect	//connect类型的socket有效, 否则为NULL
			, LPONREAD pOnRead			//connect类型的socket有效, 否则为NULL
			, LPONERROR pOnError
			);

		/************************************************************************
		* Desc : 设置UDP模式的回调函数参数, 当采用回调函数的参数时才应该进行此设置
		*	否则设置无效; 该操作应该在bind()方法之前调用
		* Return : 
		************************************************************************/
		void SetUDPCallback(
			LPVOID pParam
			, LPONREADFROM pOnReadFrom
			, LPONERROR pOnError
			);

		/************************************************************************
		* Desc : 获取本端地址和端口信息, 只对listen和connect型的socket有效; 对端连接到
		*	本端的地址信息由OnAccept函数返回
		************************************************************************/
		void GetLocalAddr(string& szIp, int& nPort);

		void* operator new(size_t nSize);
		void operator delete(void* p);

		//相关错误码定义
		enum
		{
			ERR_SOCKET_TYPE = 0x10000000,		//socket类型错误
			ERR_SOCKET_INVALID,					//无效的socket
			ERR_INIT,							//初始化失败
		};

	protected:
		typedef vector<CLIENT_SEND_DATA* > SEND_QUEUE;

		SOCKET m_hSock;
		const BOOL c_bUseCallback;			//是否采用回调函数的方式处理
		void* m_pParam;						//回调函数参数, c_bUseCallback = true时有效
		LPONREAD m_pOnRead;
		LPONREADFROM m_pOnReadFrom;
		LPONCONNECT m_pOnConnect;
		LPONACCEPT m_pOnAccept;
		LPONERROR m_pOnError;

		//volatile LONG m_nState;			//状态参数
		CLIENT_CONTEXT* m_pRcvContext;		//接收数据或Accept的Context
		CLIENT_CONTEXT* m_pSendContext;		//发送数据或连接服务器的Context
		SEND_QUEUE m_SendDataQue;	//发送数据的缓冲区队列
		CRITICAL_SECTION m_SendDataLock;			// 访问m_SendDataQue的互斥锁
		static LPFN_CONNECTEX s_pfConnectEx;		//ConnectEx函数的地址
		static LPFN_ACCEPTEX s_pfAcceptEx;			//AcceptEx函数的地址
		static LPFN_GETACCEPTEXSOCKADDRS s_pfGetAddrs;	//GetAcceptExSockaddrs函数的地址		

		//IO处理函数
		static void CALLBACK IOCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);
		static void ReadCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);
		static void SendCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);
		static void AcceptCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);

		//当网络上收到数据时会调用该方法, 用户不能释放szData, 其为系统缓存
		//返回还需接受多长的数据. 若返回0表示一个完整的数据包已接收完; 可以进行下一个数据包的接收
		//该方法用于TCP的接收, 子类须进行相应的实现
		virtual INT OnRead(const CHAR* szData, INT nDataLen);
		
		//UDP方式的数据接收, 说明同OnRead()
		virtual void OnReadFrom(const IP_ADDR& PeerAddr, const CHAR* szData, INT nDataLen);
		
		//当连接成功后调用该方法
		virtual void OnConnect();

		//当检测到有连接的客户, 会调用该方法
		virtual void OnAccept(const SOCKET& SockClient, const IP_ADDR& AddrClient);

		//当检测到连接断开或网络已关闭时调用该方法
		virtual void OnError(
			int nOperation			//当前正在进行的操作, 连接/监听/接收/发送 或其他错误码, 参见其枚举类型
			);

		//投递读操作
		BOOL PostRecv(CLIENT_CONTEXT* const pContext, const SOCKET& sock);
		inline BOOL PostRecv();

		//从发送数据缓冲区队列中取出一个有效的发送数据, 若队列为空返回NULL
		CLIENT_SEND_DATA* GetSendData();

		//清除发送数据队列
		void ClearSendQue();

		/************************************************************************
		* Desc : 获取socket的类型, tcp/udp
		* Return : SOCK_STREAM, SOCK_DGRAM
		************************************************************************/
		inline int GetSockType();
        
	private:
		static HANDLE s_hHeap;			//用于申请对象实例的堆
		static vector<ClientNet*> s_IDLQue;		//空闲的ClientNet队列
		static CRITICAL_SECTION s_IDLQueLock;	//访问s_IDLQue数据队列互斥锁

		static BOOL IsAddressValid(LPCVOID pMem);

		enum
		{
			E_BUF_SIZE = 4096,
			E_HEAP_SIZE = 300 *1024,		 //对象实例的容量为500K
			E_MAX_IDL_NUM = 2000,			 //空闲实例队列的最大长度
		};
	};
}

#endif			//#ifndef _CLIENT_NET_H