#include "DataType.h"
#include ".\runlog.h"
#include <direct.h>
#include <assert.h>
#include <atlstr.h>
#include <atltime.h>

namespace HelpMng
{

	RunLog* RunLog::s_RunLog = NULL;
	BOOL RunLog::s_bThreadRun = FALSE;
	char RunLog::s_szModulePath[512] = { 0 };


	RunLog::RunLog(void)
		: m_hThread(NULL)
	{	
		m_bPrintScreen = FALSE;
		m_bInit = TRUE;
		m_pDataBuff = NULL;
		m_nFileCount = 0;
		m_ulCurrSize = 0;
		m_nDays = 1;
 		GetLocalTime(&m_tCurrTime);
		GetLocalTime(&m_tLastTime);
		CString strPath = GetModulePath();
		strPath += "RunLog";

		_mkdir(strPath);		//建一个log目录
		m_hMutex = CreateMutex(NULL, FALSE, NULL);
		if (NULL == m_hMutex)
		{
			m_bInit = FALSE;
		}

		//创建内存映射文件
		m_bInit = CreateMemoryFile();
	}

	RunLog::~RunLog(void)
	{
		CloseHandle(m_hMutex);
		if (NULL != m_pDataBuff)
		{
			UnmapViewOfFile(m_pDataBuff);
			m_pDataBuff = NULL;
		}

		InterlockedExchange((volatile long *)&s_bThreadRun, 0);
	}

	UINT WINAPI RunLog::ThreadProc(LPVOID lpParam)
	{
		while (InterlockedExchangeAdd((volatile long *)&s_bThreadRun, 0))
		{
			LONG nDays = s_RunLog->m_nDays;
			if (nDays < 1)
			{
				nDays = 1;
			}
			//const DWORD TIME_TICK = nDays *24 *60 *60 *1000;

			CString strFindFile(s_RunLog->GetModulePath());
			strFindFile += "RunLog\\*.LOG";

			CTime tmSave = CTime::GetCurrentTime();
			CTimeSpan tmSpan(nDays, 0, 0, 0);
			tmSave -= tmSpan;

			WIN32_FIND_DATA FindData;
			HANDLE hFind = FindFirstFile(strFindFile, &FindData);
			if (INVALID_HANDLE_VALUE != hFind)
			{
				do 
				{
					CTime tmFile(FindData.ftLastWriteTime);
					if (tmSave > tmFile)
					{
						CString strDelFile(s_RunLog->GetModulePath());
						strDelFile += "RunLog\\";
						strDelFile += FindData.cFileName;

						DeleteFile(strDelFile);
					}
				} while(FindNextFile(hFind, &FindData));	

				FindClose(hFind);
			}

			Sleep(TIMEQUEUE_PERIOD);
		}

		return 0;
	}

	//===============================================
	//Name : CreateMemoryFile()
	//Desc : 创建一个内存映射文件,每天最多建200个文件
	//Return : TRUE=创建成功; FALSE=创建失败
	//===============================================
	BOOL RunLog::CreateMemoryFile()
	{	
		TCHAR szFile[MAX_PATH] = { 0 };
		HANDLE hFile;
		HANDLE hMapFile; 
		BOOL bOpenSuccess = FALSE;
		ULONG nBytesRead = 0;
		CString strPath;

		if (m_tLastTime.wDay != m_tCurrTime.wDay)
		{
			m_nFileCount = 0;
			GetLocalTime(&m_tLastTime);
		}	
		//删除已经映射的MAP
		if (NULL != m_pDataBuff)
		{
			UnmapViewOfFile(m_pDataBuff);
			m_pDataBuff = NULL;
		}
		strPath = GetModulePath();
		strPath += "RunLog";

		do{	
			m_nFileCount++;
			bOpenSuccess = FALSE;
			m_ulCurrSize = 0;

			sprintf(szFile, "%s\\%u%02u%02u%03d.LOG", strPath, m_tCurrTime.wYear, 
				m_tCurrTime.wMonth, m_tCurrTime.wDay, m_nFileCount);
			//先查看文件的实际大小是否超过3M, 若小于3M则打开文家继续使用, 否创建一个新的文件
			hFile = CreateFile(szFile, GENERIC_WRITE| GENERIC_READ, FILE_SHARE_READ |FILE_SHARE_WRITE
				, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (INVALID_HANDLE_VALUE == hFile)
			{	//若文件打开失败则新建一个文件
				hFile = CreateFile(szFile, GENERIC_WRITE| GENERIC_READ, FILE_SHARE_READ |FILE_SHARE_WRITE
					, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
			}
			else
			{	//打开文件成功,查看文件是否超过3M
				ReadFile(hFile, &m_ulCurrSize, sizeof(m_ulCurrSize), &nBytesRead, NULL);
				if (m_ulCurrSize < (FILE_MAX_SIZE - FILE_BUFF_SIZE))
				{
					bOpenSuccess = TRUE;
				}
				else
				{	//需要创建一个新的文件
					CloseHandle(hFile);
					hFile = INVALID_HANDLE_VALUE;
				}
			}
			if (TRUE == bOpenSuccess)
			{
				break;
			}
			if (m_nFileCount >= 200)
			{	//若200次创建失败,则说明无法创建
				return FALSE;
			}
		} while (INVALID_HANDLE_VALUE == hFile);	

		hMapFile = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, FILE_MAX_SIZE, NULL);
		if (NULL == hMapFile)
		{
			CloseHandle(hFile);
			return FALSE;
		}

		//将MAP定位到文件的结尾处		
		if (0 == m_ulCurrSize)
		{
			m_ulCurrSize = sizeof(m_ulCurrSize);
		}	
		m_pDataBuff = (char* )MapViewOfFile(hMapFile, FILE_MAP_WRITE, 0, 0, 0);
		SetFileRealSize();
		CloseHandle(hFile);
		CloseHandle(hMapFile);
		if (NULL == m_pDataBuff)
		{
			return FALSE;
		}

		return TRUE;
	}

	//===============================================
	//Name : _GetObject()
	//Desc : 获取日志类的唯一对象指针
	//===============================================
	RunLog* RunLog::_GetObject()
	{
		if (NULL == s_RunLog)
		{
			s_RunLog = new RunLog();
		}
		if (TRUE != s_RunLog->m_bInit && NULL != s_RunLog)
		{
			delete s_RunLog;
			s_RunLog = NULL;
		}
		return s_RunLog;
	}

	//===============================================
	//Name : Destroy()
	//Desc : 销毁日志类的全局唯一对象
	//===============================================
	void RunLog::_Destroy()
	{
		if (NULL != s_RunLog)
		{
			delete s_RunLog;
			s_RunLog = NULL;
		}
	}

	//===============================================
	//Name : WriteLog()
	//Desc : 将相关信息写入日志文件中,首先判断文件的大小
	//	是否已超过2M, 若超过2M则创建一个新的文件
	//===============================================
	void RunLog::WriteLog(const char* szFormat, ...)
	{
		DWORD dwFlag = WaitForSingleObject(m_hMutex, 500);
		if (WAIT_OBJECT_0 != dwFlag)
		{	//若没有等到信号则退出本次写日志操作
			return;
		}
		GetLocalTime(&m_tCurrTime);	
		//查看文件是否已超过3M, 若是则创建一个新的文件
		if (m_ulCurrSize > (FILE_MAX_SIZE -FILE_BUFF_SIZE))
		{
			if(FALSE == CreateMemoryFile())
			{
				ReleaseMutex(m_hMutex);
				return;
			}
		}
		if (NULL == m_pDataBuff)
		{
			ReleaseMutex(m_hMutex);
			return;
		}
		//将相应的数据写入到文件缓冲区中
		ULONG ulCount = 0;
		ulCount = sprintf((m_pDataBuff +m_ulCurrSize), "\r\n%u-%02u-%02u %02u:%02u:%02u:%03u", 
			m_tCurrTime.wYear, m_tCurrTime.wMonth, m_tCurrTime.wDay, 
			m_tCurrTime.wHour, m_tCurrTime.wMinute, m_tCurrTime.wSecond, m_tCurrTime.wMilliseconds);
		m_ulCurrSize += (ulCount +1);

		ulCount = vsprintf((m_pDataBuff +m_ulCurrSize), szFormat, (char* )(&szFormat +1));
		//若需要打印到屏幕上则打印到屏幕上
		if (m_bPrintScreen)
		{
			printf("\r\n%s", m_pDataBuff +m_ulCurrSize);
		}

		m_ulCurrSize += (ulCount +1);

		SetFileRealSize();
		ReleaseMutex(m_hMutex);
	}

	//===============================================
	//Name : GetModulePath()
	//Desc : 获得调用日志类的可执行文件路径
	//===============================================
	const char *RunLog::GetModulePath()
	{	
		if (0 == s_szModulePath[0])
		{
			DWORD uPathLen = GetModuleFileName(NULL, s_szModulePath, sizeof(s_szModulePath) -1);
			while (uPathLen > 0)
			{
				if ('\\' == s_szModulePath[uPathLen])
				{
					s_szModulePath[uPathLen +1] = NULL;
					break;
				}
				uPathLen --;
			}	
		}
		return s_szModulePath;
	}


	//===============================================
	//Name : SetFileRealSize()
	//Desc : 将文件的实际长度写入文件
	//===============================================
	void RunLog::SetFileRealSize()
	{
		assert(NULL != m_pDataBuff);
		if (NULL == m_pDataBuff)
		{
			return;
		}
		memcpy(m_pDataBuff, &m_ulCurrSize, sizeof(m_ulCurrSize));
	}

	//===============================================
	//Name : SetSaveFiles()
	//Desc : 设置可以保存多少天的文件, 如nDays=1; 则只保存当天
	//		的日志文件, nDays=10,则保存近10天的日志文件.
	//		注: 不能删除当天所建的日志文件, 该函数会自动启动一个
	//		定时器组件并将其放入线程池中进行自动管理.
	//===============================================
	void RunLog::SetSaveFiles(LONG nDays)
	{
		//若果后台线程没有创建则创建之
		if (NULL == m_hThread)
		{
			//创建后台线程
			s_bThreadRun = TRUE;
			m_hThread = (HANDLE)_beginthreadex(NULL, 0, ThreadProc, this, 0, NULL);
		}
		//设置可保存的天数
		if (nDays <= 1 || nDays >= 365)
		{
			InterlockedExchange(&m_nDays, 1);			
		}
		else
		{
			InterlockedExchange(&m_nDays, nDays);
		}
	}

	void RunLog::SetPrintScreen(BOOL bPrint /* = TRUE */)
	{
		m_bPrintScreen = bPrint;
	}

}
//===============================================
//Name :
//Desc :
//===============================================