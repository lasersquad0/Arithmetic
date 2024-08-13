// TestTCPlugin.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <windows.h>
#include <strsafe.h>
#include "wcxhead.h"

void PrintWindowsErrorMessage(const TCHAR* lpszFunction)
{
	LPVOID lpMsgBuf;
	DWORD err = GetLastError();
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	std::wcout << (TCHAR*)lpMsgBuf << std::endl;

	//const TCHAR* lpszFunction = TEXT("FindResource");

	LPVOID lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));

	StringCchPrintf((LPTSTR)lpDisplayBuf, LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"), lpszFunction, err, lpMsgBuf);


	std::wcout << (TCHAR*)lpDisplayBuf << std::endl;

	LocalFree(lpDisplayBuf);
}

/*
int main()
{
    TCHAR fn[] = L"c:\\Users\\Andrey\\source\\repos\\TCPlugin\\x64\\Debug\\arpacker.wcx64";

    cout << "Loading library : " << fn << endl;
    HMODULE hm = LoadLibrary(fn);
	if (hm == nullptr)
	{
		PrintWindowsErrorMessage(L"LoadLibrary");
		return 1;
	}
	
	cout << "LoadLibrary SUCCESS" << endl;
	//cout << "Unloading library" << endl;
	//if (!FreeLibrary(hm))
	//{
	//	PrintWindowsErrorMessage(L"FreeLibrary");
	//	return 1;
	//}
	
	typedef int (*OpenArchiveF)(tOpenArchiveData* ArchiveData);
	OpenArchiveF OpenArchive;
	OpenArchive = (OpenArchiveF)GetProcAddress(hm, "OpenArchive");
	tOpenArchiveData ArchiveData;
	int res = OpenArchive(&ArchiveData);

	cout << "OpenArchive() result:" << res << endl;

	return 0;
}
*/
