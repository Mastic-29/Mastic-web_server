#pragma once

// -----------------------------------------------------------------------*/
//
//文件名：CWinCGI.h
//创建人：MasticZhang
//创建时间：2017/7/20 13:04
//功能描述：处理执行CGI脚本
//
// -----------------------------------------------------------------------*/

#ifndef WINCGI_H_
#define WINCGI_H_

#include <windows.h>

class WinCGI
{
public:
	WinCGI();
	~WinCGI();
public:
	BOOL Exec(const char* path, const char* query_string);
	BOOL Write(PBYTE in_buffer, DWORD dwsize);
	BOOL Read(PBYTE out_buffer, DWORD dwsize);

	DWORD Wait();
private:
	HANDLE hStdInRead;
	HANDLE hStdInWrite;

	HANDLE hStdOutRead;
	HANDLE hStdOutWrite;

	STARTUPINFO siStartupInfo;
	PROCESS_INFORMATION piProcInfo;
private:
	void __Reset();
};


#endif // WINCGI_H_
