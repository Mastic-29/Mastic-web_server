// -----------------------------------------------------------------------*/
//
//文件名：CWinCGI.cpp
//创建人：MasticZhang
//创建时间：2017/7/20 13:46
//功能描述：处理执行CGI脚本
//
// -----------------------------------------------------------------------*/

#include "WinCGI.h"
#include <atlconv.h>
#include <wchar.h>

WinCGI::WinCGI()
	:hStdOutRead(nullptr)
	, hStdInRead(nullptr)
	, hStdOutWrite(nullptr)
	, hStdInWrite(nullptr)
{
	memset(&siStartupInfo, 0, sizeof(siStartupInfo));
	memset(&piProcInfo, 0, sizeof(piProcInfo));
}

WinCGI::~WinCGI()
{
	__Reset();
}



// -----------------------------------------------------------------------*/
//函数方法：__Reset
//作用：重置类成员
// -----------------------------------------------------------------------*/
void WinCGI::__Reset() {
	if (hStdOutRead) {
		CloseHandle(hStdOutRead);
		hStdOutRead = nullptr;
	}

	if (hStdInRead) {
		CloseHandle(hStdInRead);
		hStdInRead = nullptr;
	}

	if (hStdOutWrite) {
		CloseHandle(hStdOutWrite);
		hStdOutWrite = nullptr;
	}

	if (hStdInWrite) {
		CloseHandle(hStdInWrite);
		hStdInWrite = nullptr;
	}

	if (piProcInfo.hProcess) {
		CloseHandle(piProcInfo.hProcess);
		piProcInfo.hProcess = nullptr;
	}

	if (piProcInfo.hThread) {
		CloseHandle(piProcInfo.hThread);
		piProcInfo.hThread = nullptr;
	}
}



// -----------------------------------------------------------------------*/
//函数方法：Exec
//作用：处理CGI主要程序
// -----------------------------------------------------------------------*/
BOOL WinCGI::Exec(const char* path, const char* query_string) {
	BOOL bRet = FALSE;

	do {

		if (!path) break;

		__Reset();

		//创建管道
		SECURITY_ATTRIBUTES sa{ sizeof(SECURITY_ATTRIBUTES),0,TRUE };
		if (!CreatePipe(&hStdInRead, &hStdInWrite, &sa, 0)) break;
		if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0))break;

		ZeroMemory(&siStartupInfo, sizeof(siStartupInfo));
		siStartupInfo.cb = sizeof(siStartupInfo);
		GetStartupInfo(&siStartupInfo);//获得进程的启动信息
		
		siStartupInfo.dwFlags |= STARTF_USESHOWWINDOW; //使用 wShowWindow
		siStartupInfo.dwFlags |= STARTF_USESTDHANDLES; //使用hStdInput hStdOutput
		siStartupInfo.wShowWindow = SW_HIDE;

		siStartupInfo.hStdInput = hStdInRead; //重定向子进程的输入
		siStartupInfo.hStdOutput = hStdOutWrite;//重定向子进程的输出


		WCHAR wcCmd[1024]{ 0 };
		USES_CONVERSION;
		if (0 == _stricmp(path, "htdocs/cgipy") && query_string) {
			swprintf_s(wcCmd, _countof(wcCmd) - 1, L"\"python.exe\"  htdocs\\cgi\\%s", A2W(query_string));
		}
		else if (0 == _stricmp(path, "htdocs/cgibat") && query_string) {
			swprintf_s(wcCmd, _countof(wcCmd) - 1, L"\"cmd.exe\"  /c htdocs\\cgi\\%s", A2W(query_string));//执行结束关闭窗口
		}
		else if (0 == _stricmp(path, "htdocs/cgipost")) {
			swprintf_s(wcCmd, _countof(wcCmd) - 1, L"\"python.exe\"  htdocs\\cgi\\%s", L"cgipost.py");
		}
		else {
			break;
		}

		if (!CreateProcess(
			nullptr,
			wcCmd,
			nullptr,
			nullptr,
			TRUE,
			CREATE_UNICODE_ENVIRONMENT,
			nullptr,
			nullptr,
			&siStartupInfo,
			&piProcInfo)) {
			break;
		}
			
		bRet = TRUE;
	} while (0);

	return bRet;
}




// ---------------------------------------------------------------------- - */
//函数方法：Wait
//作用：等待通知（5000）
// -----------------------------------------------------------------------*/
DWORD WinCGI::Wait() {

	return WaitForSingleObject(piProcInfo.hProcess, 5000);
}



// ---------------------------------------------------------------------- - */
//函数方法：Write
//作用：写操作
// -----------------------------------------------------------------------*/
BOOL WinCGI::Write(PBYTE in_buffer, DWORD dwsize) {
	if (!in_buffer || 0 == dwsize) {
		return FALSE;
	}

	DWORD process_exit_code = 0;
	GetExitCodeProcess(piProcInfo.hProcess, &process_exit_code);
	if (process_exit_code == STILL_ACTIVE) {
		return FALSE;
	}

	DWORD dwWritten = 0;
	return WriteFile(hStdInWrite, in_buffer, dwsize, &dwWritten, nullptr);
}



// ---------------------------------------------------------------------- - */
//函数方法：Read
//作用：读操作
// -----------------------------------------------------------------------*/
BOOL WinCGI::Read(PBYTE out_buffer, DWORD dwsize) {
	DWORD dwRead = 0;
	return ReadFile(hStdOutRead, out_buffer, dwsize, &dwRead, nullptr);
}