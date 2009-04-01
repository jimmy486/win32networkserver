#ifndef _TCP_CLIENT_H_
#define _TCP_CLIENT_H_

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
	void ReleaseTcpClient();

	class TCP_CLIENT_EX : public NET_CONTEXT
	{
		friend class TcpClient;
	protected:
		DWORD m_nDataLen;		//TCP模式下为累计接收或发送的数据长度

		TCP_CLIENT_EX()
			: m_nDataLen(0)
		{
		}
		virtual ~TCP_CLIENT_EX() {}

		void* operator new(size_t nSize);
		void operator delete(void* p);

		enum
		{
			E_TCP_HEAP_SIZE = 1024 * 1024* 10,
			MAX_IDL_DATA = 20000,
		};
	private:
		static vector<TCP_CLIENT_EX* > s_IDLQue;		//无效的数据队列
		static CRITICAL_SECTION s_IDLQueLock;		//访问s_IDLQue的互斥锁
		static HANDLE s_hHeap;	//NET_CONTEXT自己的堆内存
	};

	class DLLENTRY TCP_CLIENT_RCV_EX
	{
		friend class TcpClient;
	public:
		CHAR* m_pData;			//数据缓冲区, 优化时可采用虚拟内存的方式实现
		INT m_nLen;				//数据缓冲区的长度

		TCP_CLIENT_RCV_EX(const CHAR* pBuf, INT nLen);
		~TCP_CLIENT_RCV_EX();

		void* operator new(size_t nSize);
		void operator delete(void* p);

		enum
		{
			HEAP_SIZE = 1024 *1024* 50,		//自定义堆的最大容量
			DATA_HEAP_SIZE = 1024 *1024 * 100,
			MAX_IDL_DATA = 100000,
		};

	private:
		static vector<TCP_CLIENT_RCV_EX* > s_IDLQue;		//无效的数据队列
		static CRITICAL_SECTION s_IDLQueLock;		//访问s_IDLQue的互斥锁
		static HANDLE s_hHeap;		//Rcv_DATA的自定义堆
		static HANDLE s_DataHeap;	//数据缓冲区的自定义堆, 最大容量为10M
	};

	class DLLENTRY TcpClient : public NetWork
	{
		friend void ReleaseTcpClient();

	public:
		/****************************************************
		* Name : InitReource()
		* Desc : 对相关全局及静态资源进行初始化
		*	该方法由DllMain()方法调用, 开发者不能调用
		*	该方法; 否则会造成内存泄露
		****************************************************/
		static void InitReource();

		//* 注 : 请不要回调函数中做耗时操作, 否则会导致I/O线程阻塞
		TcpClient(
			LPCLOSE_ROUTINE pCloseFun		//某个socket关闭的回调函数
			, void *pParam					//回调函数的pParam参数
			, LPONCONNECT pConnFun		//连接成功的回调函数
			, void *pConnParam			//pConnFun 的参数
			);
		virtual ~TcpClient();

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

		/****************************************************
		* Name : Connect()
		* Desc : 连接到指定的服务器, 客户端网络模块需实现该函数
		****************************************************/
		virtual BOOL Connect(
			const char *szIp
			, int nPort
			, const char *szData = NULL     //连接成功后发送的第一份数据
			, int nLen = 0                            //数据的长度
			);

		/*************************************************
		* Name : IsConnected()
		* Desc : 客户端连接是否正常
		***********************************************/
		BOOL IsConnected()const { return m_bConnected; }

		void* operator new(size_t nSize);
		void operator delete(void* p);

		void *operator new(size_t nSize, const char *, int)
		{
			return operator new(nSize);
		}

		void operator delete(void *p, const char *, int)
		{
			operator delete(p);
		}
	protected:
		vector<TCP_CLIENT_RCV_EX* > m_RcvDataQue;        //接受数据缓冲区队列
		CRITICAL_SECTION m_RcvQueLock;					//访问s_RcvDataQue的互斥锁

		LPCLOSE_ROUTINE m_pCloseFun;					//网络关闭的回调通知函数
		LPVOID m_pCloseParam;							//传递给m_pCloseFun的参数

		LPONCONNECT m_pConnFun;               //连接成功的回调函数
		void *m_pConnParam;

		BOOL m_bConnected;					//客户端连接是否正常

		/****************************************************
		* Name : IOCompletionProc()
		* Desc : IO完成后的回调函数
		****************************************************/
		virtual void IOCompletionProc(BOOL bSuccess, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped) ;

		/****************************************************
		* Name : OnConnect()
		* Desc : 连接的回调函数
		****************************************************/
		void OnConnect(BOOL bSuccess, DWORD dwTrans, LPOVERLAPPED lpOverlapped);

		/****************************************************
		* Name : OnRead() 
		* Desc : 读操作完成的回调函数
		****************************************************/
		void OnRead(BOOL bSuccess, DWORD dwTrans, LPOVERLAPPED lpOverlapped);

	private:
		static HANDLE s_hHeap;			//用于申请对象实例的堆
		static vector<TcpClient*> s_IDLQue;		//空闲的TcpClient队列
		static CRITICAL_SECTION s_IDLQueLock;	//访问s_IDLQue数据队列互斥锁
		static LPFN_CONNECTEX s_pfConnectEx;		//ConnectEx函数的地址
		static BOOL s_bInit;					//是否已经初始化

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

#endif				//#ifndef _TCP_CLIENT_H_