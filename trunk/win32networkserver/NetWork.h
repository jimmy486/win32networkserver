#ifndef _NET_WORK_H
#define _NET_WORK_H
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

namespace HelpMng
{

	/****************************************************
	* Name : ReleaseNetWork()
	* Desc : 释放class NetWork的相关资源
	****************************************************/
	void ReleaseNetWork();

	class DLLENTRY NetWork
	{
		friend void ReleaseNetWork(); 
	public:
		/****************************************************
		* Name : InitReource()
		* Desc : 对相关全局及静态资源进行初始化
		*	该方法由DllMain()方法调用, 开发者不能调用
		*	该方法; 否则会造成内存泄露
		****************************************************/
		static void InitReource();

		NetWork()
			: m_hSock(INVALID_SOCKET)
		{}
		virtual ~NetWork() {}

		/****************************************************
		* Name : Init()
		* Desc : 对网络模块进行初始化
		****************************************************/
		virtual BOOL Init(
			const char *szIp	//要启动服务的本地地址, NULL 则采用默认地址
			, int nPort	//要启动服务的端口
			) = 0;

		/****************************************************
		* Name : Close()
		* Desc : 关闭网络模块
		****************************************************/
		virtual void Close() = 0;

		/****************************************************
		* Name : SendData()
		* Desc : TCP模式的发送数据
		****************************************************/
		virtual BOOL SendData(const char * szData, int nDataLen, SOCKET hSock = INVALID_SOCKET);

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
		* Name : GetSocket()
		* Desc : 
		***********************************************/
		SOCKET GetSocket()const { return m_hSock; }
		
	protected:
		SOCKET m_hSock;

		static HANDLE s_hCompletion;                    //完成端口句柄
		static HANDLE *s_pThreads;                       //在完成端口上工作的线程

		static UINT WINAPI WorkThread(LPVOID lpParam);

		/****************************************************
		* Name : IOCompletionProc()
		* Desc : IO完成后的回调函数
		****************************************************/
		virtual void IOCompletionProc(BOOL bSuccess, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped) = 0;

	private:
		static BOOL s_bInit;                                   //是否已进行初始化工作

		/****************************************************
		* Name : ReleaseReource()
		* Desc : 对相关全局和静态资源进行释放
		****************************************************/
		static void ReleaseReource();
	};
}

#endif		//#ifndef _NET_WORK_H