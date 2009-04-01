#ifndef _HEAD_FILE_H
#define _HEAD_FILE_H

#ifndef WINVER				// 允许使用特定于 Windows XP 或更高版本的功能。
#define WINVER 0x0501		// 将此值更改为相应的值，以适用于 Windows 的其他版本。
#endif

#ifndef _WIN32_WINNT		// 允许使用特定于 Windows XP 或更高版本的功能。
#define _WIN32_WINNT 0x0501	// 将此值更改为相应的值，以适用于 Windows 的其他版本。
#endif						

#ifndef _WIN32_WINDOWS		// 允许使用特定于 Windows 98 或更高版本的功能。
#define _WIN32_WINDOWS 0x0410 // 将此值更改为适当的值，以指定将 Windows Me 或更高版本作为目标。
#endif

#ifndef _WIN32_IE			// 允许使用特定于 IE 6.0 或更高版本的功能。
#define _WIN32_IE 0x0600	// 将此值更改为相应的值，以适用于 IE 的其他版本。
#endif

#ifdef WIN32

#include <WinSock2.h>
#include <MSWSock.h>
#include <vector>

#ifdef _DLL_EXPORTS_
#define DLLENTRY __declspec(dllexport)
//#pragma comment(lib, "ws2_32.lib")

#else
#define DLLENTRY __declspec(dllimport)
#endif		//#ifdef _DLL_EXPORTS_
#else
#define DLLENTRY  
#endif		//#ifdef WIN32

#define THROW_LINE (throw ((long)(__LINE__)))

using namespace std;
namespace HelpMng
{
	enum 
	{
		OP_ACCEPT = 1,			//要在网络上进行或已完成accept操作
		OP_CONNECT,				//要在网络上进行或已完成connect操作
		OP_READ,				//在网络上进行或已完成READ操作
		OP_WRITE,				//在网络上进行或已完成write操作
	};

	class NET_CONTEXT;
	class IP_ADDR;

	//用户连接操作的回调函数, 用户只需传递lpThreadParameter参数即可, 
	//sock参数将由系统传递, nState 表明连接状态
	//注：该函数中不要有过于复杂的运算或操作, 因为该函数是由IO线程来调用的
	//	如果过于复杂将影响IO线程的执行
	typedef void (WINAPI *PCONNECT_ROUTINE)(
		LPVOID lpThreadParameter
		, SOCKET hSock	
		);
	typedef PCONNECT_ROUTINE LPCONNECT_ROUTINE;

	//当检测网络关闭时通知函数，用户只需要传递pParam参数即可
	typedef void (WINAPI *PCLOSE_ROUTINE)(
		LPVOID pParam					//网络关闭的参数
		, SOCKET hSock
		, int nOpt					//socket关闭时所进行的操作
		);
	typedef PCLOSE_ROUTINE LPCLOSE_ROUTINE;

	/************************************************************************
	* Desc : 当用户连接服务器成功后的回调函数, TCP方式有效
	* Return : 
	************************************************************************/
	typedef void (CALLBACK *LPONCONNECT)(LPVOID pParam);

	/************************************************************************
	* Name: class NET_CONTEXT
	* Desc: 本类主要用于处理网络的读写操作的缓冲区, 该类不能直接进行实例化
	************************************************************************/
	class NET_CONTEXT 
	{
	public:
		WSAOVERLAPPED m_ol;
		SOCKET m_hSock;			//相关网络接口
		CHAR* m_pBuf;			//接受或发送数据的缓冲区, 采用虚拟内存方式实现; 数据的实际长度存放在BUF的头两个字节中
		INT m_nOperation;		//网络接口上要进行的操作, 可以是OP_ACCEPT, OP_CONNECT, OP_READ, OP_WRITE		
		//volatile LONG m_nPostCount;		//1表明正在进行读写操作, 0表明socket空闲, 若为-1则表明监听socket关闭

		static DWORD S_PAGE_SIZE;		//当前系统的内存页大小

		/****************************************************
		* Name : GetSysMemPageSize()
		* Desc : 获取当前系统内存页的大小
		****************************************************/
		static DWORD GetSysMemPageSize();
		
		NET_CONTEXT();
		virtual ~NET_CONTEXT();


		/****************************************************
		* Name : InitReource()
		* Desc : 对相关全局及静态资源进行初始化
		*	该方法由DllMain()方法调用, 开发者不能调用
		*	该方法; 否则会造成内存泄露
		****************************************************/
		static void InitReource();

		/****************************************************
		* Name : ReleaseReource()
		* Desc : 对相关全局和静态资源进行释放
		*	该方法由DllMain()方法调用, 开发者不能调用
		*	方法.
		****************************************************/
		static void ReleaseReource();

	private:
		void* operator new (size_t nSize);
		void operator delete(void* p);
		static HANDLE s_hDataHeap;	
		static vector<char * > s_IDLQue;		//无效的数据队列
		static CRITICAL_SECTION s_IDLQueLock;		//访问s_IDLQue的互斥锁
	};


	/************************************************************************
	* Name: class IP_ADDR
	* Desc: 该类主要用于map操作
	************************************************************************/
	class DLLENTRY IP_ADDR : public sockaddr_in
	{
	public:	
		IP_ADDR() { sin_family = AF_INET; }
		IP_ADDR(const char* szIP, INT nPort);
		IP_ADDR(const IP_ADDR& other);
		~IP_ADDR() {};

		IP_ADDR& operator =(const IP_ADDR& other);
		IP_ADDR& operator =(const sockaddr_in& other);
		bool operator ==(const IP_ADDR& other)const;
		bool operator ==(const sockaddr_in& other)const;
		bool operator <(const IP_ADDR& other)const;
		bool operator <(const sockaddr_in& other)const;
	};

	/************************************************
	* Desc : 数据包头的定义
	* 格式如下：
	* |<------16------>|<------16------>|		
	* +----------------+-----------------+
	* |				数据包总长度		  |
	* +-----------------------------------+
	* |				数据包的序列号		 |
	* +----------------+------------------+
	* |当前数据包的长度|  数据包的类型  |
	* +----------------+------------------+
	* |				  数据				     |
	* .									.
	* .								    .
	* .								    .
	* +----------------+----------------+
	* 注: 序列号从0开始; 当前收到的数据包的序列号 + 当前数据包的长度 = 要接收的下一个数据包的序列号
	* 若数据包的序列号 + 当前数据包的长度 = 数据包的总长度, 说明该类型的数据包已接收完毕
	* 数据包的长度不包括包头的长度
	************************************************/
	struct PACKET_HEAD
	{
		ULONG nTotalLen;			//数据包的总长度
		ULONG nSerialNum;			//数据包的序列号
		WORD nCurrentLen;			//当前数据包的长度
		WORD nType;					//数据包的类型
	};


#ifdef __cplusplus
	extern "C" 
	{
#endif

	/************************************************************************
	* Desc : 从给定的addr中获取ip地址和端口
	************************************************************************/
	void DLLENTRY GetAddress(
		const IP_ADDR& addr
		, void *pSzIp										// string 类型
		, int& nPort
		);

	/************************************************************************
	* Desc : 获取本地IP地址列表
	************************************************************************/
	BOOL DLLENTRY GetLocalIps(
		void *pIpList										//vector<string>类型 
		);

#ifdef __cplusplus
	}
#endif		//#ifdef __cplusplus

}

#endif //#ifndef _HEAD_FILE_H