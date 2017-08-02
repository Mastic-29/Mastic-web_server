#pragma once

// -----------------------------------------------------------------------*/
//
//�ļ�����CWinCGI.h
//�����ˣ�MasticZhang
//����ʱ�䣺2017/7/20 13:04
//��������������ִ��CGI�ű�
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
