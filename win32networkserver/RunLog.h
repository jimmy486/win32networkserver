#ifndef _RUN_LOG_
#define _RUN_LOG_

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

namespace HelpMng
{
	//本模块主要记录相关的运行日志信息,对本类成员的访问是线程安全的
	//该类会自动在应用程序目录下创建一个log目录; 并生成一个以当天日期
	//为名的log文件, 每个文件的可写入容量为2M左右, 当大于2M时会新建一个
	//文件; 使用该模块时请在stdafx.h文件中添加或修改如下宏:
	//#define WINVER 0x0400   #define _WIN32_WINNT 0x0500
	//#define _WIN32_WINDOWS 0x0410
	class DLLENTRY RunLog
	{
	public:
		static RunLog* _GetObject();
		static void _Destroy();
		static const char *GetModulePath();

		void WriteLog(const char* szFormat, ...);
		void SetSaveFiles(LONG nDays);

		/************************************************************************
		* Desc : 是否将日志打印到屏幕上
		************************************************************************/
		void SetPrintScreen(BOOL bPrint = TRUE);

	protected:
		RunLog(void);
		~RunLog(void);
		BOOL CreateMemoryFile();
		inline void SetFileRealSize();
		//后台线程处理函数
		static UINT WINAPI ThreadProc(void *lpParam);
		
		enum { FILE_BUFF_SIZE = 1024 };			//文件预留的缓存大小
		enum { FILE_MAX_SIZE = 1024* 1024 +FILE_BUFF_SIZE };	//日志文件的最大长度
		enum { TIMEQUEUE_PERIOD = 1000 *60 *60 *24 };	//线程组件的执行时间间隔为24个小时

		static RunLog* s_RunLog;		//全局唯一日志对象	
		static BOOL s_bThreadRun;		//是否允许后台线程继续运行
		static char s_szModulePath[512];

		CHAR* m_pDataBuff;				//指向文件的缓冲区指针		
		HANDLE m_hMutex;				//多线程下资源的访问锁
		HANDLE m_hThread;				//线程的句柄
		UINT m_nFileCount;				//当前以创建的文件个数,用于创建新文件用
		LONG m_nDays;					//可以保存多少天的文件
		BOOL m_bInit;					//初始化操作是否成功
		BOOL m_bPrintScreen;			//是否将日志打印到屏幕上
		ULONG m_ulCurrSize;				//当前已写入的字节总数
		SYSTEMTIME m_tCurrTime;			//当前要写入文件的时间
		SYSTEMTIME m_tLastTime;			//前一天的时间
	};

#ifndef _TRACE
#define _TRACE (RunLog::_GetObject()->WriteLog)
#endif

	//用于记录一些调试信息
#define TRACELOG(x) ((RunLog::_GetObject())->WriteLog("%s L%ld %s", __FILE__, __LINE__, (x)))

	//用于调试一些函数信息,使用方法: TRACEFUN("funName", y++)
#define TRACEFUN(x, y) ((RunLog::_GetObject())->WriteLog("%s L%ld %s__%ld", __FILE__, __LINE__, (x), (y)))

#define TRACE1NUM(x, y, Param) ((RunLog::_GetObject())->WriteLog("%s L%ld %s__%ld PARAM = 0x%x", __FILE__, __LINE__, (x), (y), (Param)))

#define TRACE1STR(x, y, Param) ((RunLog::_GetObject())->WriteLog("%s L%ld %s__%ld STR_PARAM = %s", __FILE__, __LINE__, (x), (y), (Param)))

	//用于进行调试时记录相关信息
#define TRACEDEBUG1STR(x) (TRACE(_T("\r\n%s %s L%ld STR = %s"), CTime::GetCurrentTime().Format("%Y-%m-%d %H:%M:%S"), __FILE__, __LINE__, x)) 
#define TRACEDEBUG1NUM(x) (TRACE(_T("\r\n%s %s L%ld PARAM = %ld"), CTime::GetCurrentTime().Format("%Y-%m-%d %H:%M:%S"), __FILE__, __LINE__, x))

}

#endif