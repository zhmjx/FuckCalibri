#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <psapi.h>
#include <algorithm>
#include <string>
#include <cctype>
#include <sstream>
#include <iostream>
#include <winuser.h>
#include <memoryapi.h>
#include <map>
#include <winbase.h>

std::map<DWORD, DWORD> gProcessesMap;

int patrol_scan() {
	DWORD aProcesses[1024];
	DWORD cbNeeded;
	DWORD cProcesses;
	unsigned int i;

	if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
		return 1;

	cProcesses = cbNeeded / sizeof(DWORD);
	WCHAR procWChar[MAX_PATH];
	HANDLE hProcess;

	//update gProcessesMap
	unsigned int k;
	std::map<DWORD, DWORD>::iterator it;
	std::map<DWORD, DWORD>::iterator it1;
	std::map<DWORD, DWORD> processMap;

	for (k = 0; k < cProcesses; k++) {
		processMap.insert(std::pair<DWORD,DWORD>(aProcesses[k], aProcesses[k]));
	}

	it = gProcessesMap.begin();
	while( it != gProcessesMap.end()) {
		it1 = processMap.find(it->first);
		if (it1 == processMap.end())
			gProcessesMap.erase(it++);
		else
			it++;
	}

	for (i = 0; i < cProcesses; i++)
	{
		//skip process we have opened
		it = gProcessesMap.find(aProcesses[i]);
		if (it != gProcessesMap.end())
			continue;
		
		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, aProcesses[i]);
		gProcessesMap.insert(std::pair<DWORD,DWORD>(aProcesses[i], aProcesses[i]));

		if (NULL == hProcess) {
			continue;
		}

		DWORD namelen = GetProcessImageFileName(hProcess, procWChar, sizeof(procWChar) / sizeof(*procWChar));
		std::wstring procName = std::wstring(procWChar);
		size_t lastPath = procName.find_last_of(L"\\");
		procName = procName.substr(lastPath + 1, procName.length() - lastPath - 1);
		const WCHAR* procWChar = procName.c_str();
		if (_wcsicmp(procWChar, L"onenote.exe") && _wcsicmp(procWChar, L"onenoteim.exe")) {
			CloseHandle(hProcess);
			continue;
		}

		HMODULE hMods[1024];
		if (EnumProcessModulesEx(hProcess, hMods, sizeof(hMods), &cbNeeded, LIST_MODULES_ALL))
		{
			std::wstringstream modsString;
			int j;
			for (j = 0; j < (cbNeeded / sizeof(HMODULE)); j++)
			{
				TCHAR szModName[MAX_PATH];
				if (GetModuleBaseName(hProcess, hMods[j], szModName, sizeof(szModName) / sizeof(TCHAR)))
				{
					if (!_wcsicmp(szModName, L"onmain.dll") || !_wcsicmp(szModName, L"onmainim.dll") || !_wcsicmp(szModName, L"onmainw32.dll")) {
						MODULEINFO mi;
						GetModuleInformation(hProcess, hMods[j], &mi, sizeof(mi));
						unsigned char* buffer = (unsigned char*)malloc(mi.SizeOfImage);
						SIZE_T readlen;
						unsigned char* pread = (unsigned char*)mi.lpBaseOfDll;

						ReadProcessMemory(hProcess, pread, buffer, mi.SizeOfImage, &readlen);
						unsigned char* ptr;

						for (ptr = buffer; ptr < buffer + readlen - 5; ptr++) {
							if ((*ptr == 0xb9 || *ptr == 0x68) && (*(DWORD*)(ptr + 1) == 0x302 || (*(DWORD*)(ptr + 1) == 0x103 && *(ptr + 5) == 0xe9))) {
								*(ptr + 1) &= 0xfd;
								WriteProcessMemory(hProcess, pread + (ptr - buffer), ptr, 6, NULL);//fuck it
							}
						}
						free(buffer);
					}
				}
			}
		}
		CloseHandle(hProcess);
	}
	return 0;
}

int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nShowCmd
) {
	CreateMutex(NULL, FALSE, L"FuckCalibri");
	if (GetLastError() == ERROR_ALREADY_EXISTS) 
		return 1;
	
	if (!strstr(lpCmdLine, "/quite")) {
		MessageBoxA(NULL, "彻底治愈强迫症，修复ONENOTE英文模式下自动切换回Calibri字体。\n在LXF同学的基础上适配ONENOTE 2016。", "FuckCalibri", 0);
	}
	while (1) {
		patrol_scan();
		Sleep(1000);
	}
}
