#pragma once
#include <string>
#include "ClientNet.h"

using namespace std;

namespace HelpMng
{	
	//当代理完成后的回调函数
	typedef void (CALLBACK *LPRELAY_RESULT)(
		LPVOID pParam			//
		, int nErrCode			//0 = 代理成功; 非0为错误码
		);

	class Sock5Relay
	{
	public:	

		/************************************************************************
		*Name : Sock5Relay()
		*Desc : 构造一个TCP方式的sock5代理
		*Param : szUser[IN] 登录代理服务器的用户名, 不为空时采用认证方式, 否则采用非认证方式
		*	szPwd[IN] 登录到代理服务器的密码
		*	szDstIP[IN] 最终要连接的远程IP地址
		*	nDstPort[IN] 最终要连接的远程端口
		************************************************************************/
		Sock5Relay(
			const string& szUser
			, const string& szPwd
			, const CHAR* szDstIp
			, INT nDstPort
			, LPONREAD pReadFun			//代理成功以后的数据读取回调函数
			, LPONERROR pErrorFun		//代理的错误处理函数
			, LPRELAY_RESULT pResultFun	//代理结果的回调函数
			, LPVOID pParam				//传给回调函数的参数
			);

		/************************************************************************
		*Name : Sock5Relay()
		*Desc : 构造一个UDP方式的sock5代理
		*Param : nLocalPort[IN] 要进行本地帮定的端口
		*	pNetWork[IN] 要处理协议的网络接口, 若为空则自动申请新的网络接口
		************************************************************************/
		Sock5Relay(
			const string& szUser
			, const string& szPwd
			, INT nLocalPort
			, LPONREADFROM pReadFromFun		//数据读取处理函数
			, LPONERROR pErrorFun			//数据错误处理函数
			, LPRELAY_RESULT pResultFun		//代理结果处理函数
			, LPVOID pParam					//传给回调函数的参数
			);

		//Sock5Relay(const string& szUser, const string& szPwd);
		virtual ~Sock5Relay(void);

		/************************************************
		* Desc : 开始进行sock5代理
		************************************************/
		BOOL Start(			
			const char* szRelayIp			//代理服务器的地址
			, int nSerPort = 1080			//代理服务器端口
			);

		BOOL Send(const char* szData, int nLen);

		BOOL SendTo(const IP_ADDR& peer_addr, const char* szData, int nLen);

		enum 
		{
			STATE_SUCCESS = 0,				//代理成功
			STATE_INIT,
			STATE_CONNECTING,				//正在连接代理服务器
			STATE_STEP_1,					//代理第一步
			STATE_STEP_1_AUTH,				//代理第一步的认证方式, 采用认证方式时有效
			STATE_STEP_2,
			STATE_FAILE,					//代理失败
		};

	protected:        

		/************************************************************************
		* Desc : 从代理服务器读取数据的处理函数, 若正在进行代理则需要在函数中进行
		*	代理协议的解析, 若代理已成功则只需要调用相应的回调函数即可
		* Return : 还需要读取多少字节的数据, 若此次数据以读取完毕, 返回0
		************************************************************************/
		int CALLBACK ReadProc(LPVOID pParam, const char* szData, int nLen);

		/************************************************************************
		* Desc : UDP代理的读数据函数, 只要代理成功后该函数才有效
		************************************************************************/
		void CALLBACK ReadFromProc(LPVOID pParam, const IP_ADDR& peer_addr, const char* szData, int nLen);

		/************************************************************************
		* Desc : 连接代理服务器成功后的回调函数
		************************************************************************/
		void CALLBACK ConnectProc(LPVOID pParam);

		/************************************************************************
		* Desc : 错误处理函数
		************************************************************************/
		void CALLBACK ErrorProc(LPVOID pParam, int nOpt);

		/************************************************************************
		*Name : ParseUdpData()
		*Desc : 当收到代理服务器发送来的UDP数据包时, 需要先调用该函数进行解析
		*	该函数主要解析UDP数据包头获取对端的真实地址和对端实际发送的数据
		*Param : szBuf[IN] 代理服务器发送来的数据
		*	nBufLen[IN] 数据的长度
		*	PeerAddr[OUT] 实际发送数据包的对端地址
		*	nDataLen[OUT] 对端发送数据的长度
		*Return : 对端的数据地址
		************************************************************************/
		const CHAR* ParseUdpData(const CHAR* szBuf, INT nBufLen, IP_ADDR& PeerAddr, INT& nDataLen);

		string m_szUser;			//登录到服务器的用户名
		string m_szPwd;				//登录到服务器的密码
		BOOL m_bAuth;				//是否需要认证
		int m_nState;				//代理的当前步骤
		CHAR m_szSerIP[16];			//服务器IP地址	
		INT m_nSerPort;				//服务器端口
		INT m_nNetType;				//网络模型, TCP/UDP
		CHAR m_szDstIP[16];			//最终要连接的服务器IP地址
		INT m_nDstPort;				//
		INT m_nLocalPort;			//要帮定的本地端口
		IP_ADDR m_SerAddr;			//UDP方式代理成功以后与UDP socket通信的服务器地址和端口信息
		ClientNet* m_pRelayNet;		//与代理服务器通信的网络接口, 若采用TCP代理方式该接口即TCP的网络接口
		ClientNet* m_pUdpNet;		//当采用UDP方式代理时, 该接口用于UDP通信
		LPONREAD m_pReadProc;		//从目的服务器的读取数据处理函数
		LPONREADFROM m_pReadFromProc;
		LPONERROR m_pErrorProc;
		LPRELAY_RESULT m_pResultProc;
		LPVOID const m_pParam;
	};

}