#ifndef _MY_CLIENT_H
#define _MY_CLIENT_H
#include "ClientNet.h"
using namespace HelpMng;

//客户端类
class MyClient
{
public:
	MyClient() {}
	MyClient(const SOCKET& s, DWORD nId);
	virtual ~MyClient();

protected:
	ClientNet* m_pNet;			//处理TCP的网络传输模块
	DWORD m_nId;				//该客户端的ID


	/************************************************
	* Desc : TCP的读数据处理函数
	************************************************/
	static int CALLBACK ReadProc(
		LPVOID pParam
		, const char* szData
		, int nLen
		);

	/************************************************
	* Desc : 网络的错处理函数
	************************************************/
	static void CALLBACK ErrorProc(LPVOID pParam, int nOpt);
private:
};

#endif		//#define _MY_CLIENT_H