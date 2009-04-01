#ifndef _MY_CLIENT_H
#define _MY_CLIENT_H
#include "ClientNet.h"
using namespace HelpMng;

//客户端类
class MyClient
{
public:
	MyClient() {}
	virtual ~MyClient();

	/************************************************
	* Desc : 初始化客户端
	************************************************/
	BOOL Init(
		DWORD nId				//该客户端的ID
		, const char* szSerIp	//服务器的IP地址
		, int nPort				//服务器的端口
		, BOOL bType			//tcp or udp
		);
protected:
	ClientNet* m_pNet;			//处理TCP的网络传输模块
	DWORD m_nId;				//该客户端的ID
	DWORD m_nCount;				//客户端的报文计数器


	/************************************************
	* Desc : TCP的读数据处理函数
	************************************************/
	static int CALLBACK ReadProc(
		LPVOID pParam
		, const char* szData
		, int nLen
		);

	/************************************************
	* Desc : UDP的读数据处理函数
	************************************************/
	static void CALLBACK ReadFromProc(
		LPVOID pParam
		, const IP_ADDR& peer_addr
		, const char* szData
		, int nLen
		);

	/************************************************
	* Desc : 连接成功后的处理函数
	************************************************/
	static void CALLBACK ConnectPrco(LPVOID pParam);

	/************************************************
	* Desc : 网络的错处理函数
	************************************************/
	static void CALLBACK ErrorProc(LPVOID pParam, int nOpt);
private:
};

#endif		//#define _MY_CLIENT_H