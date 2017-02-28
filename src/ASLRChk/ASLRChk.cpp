#include "stdafx.h"

bool CheckFile(char * lpFilePath, bool bUnprotect);

bool CheckFile(char * lpFilePath, bool bUnprotect) {
	HANDLE hFile = CreateFile(lpFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		std::cout << "Not found file in this application." << std::endl;
		return false;
	}
	
	LARGE_INTEGER LargeFileSize = { 0, };
	if (!GetFileSizeEx(hFile, &LargeFileSize)) { 
		std::cout << "Failed to read file in this application." << std::endl;
		CloseHandle(hFile);
		return false;
	}
	if (LargeFileSize.HighPart) { 
		std::cout << "This file is not supported in this application." << std::endl;
		CloseHandle(hFile);
		return false;
	}
	
	PBYTE lpBinary = new BYTE[LargeFileSize.LowPart];
	DWORD dwNumberOfBytes;
	if (!ReadFile(hFile, lpBinary, LargeFileSize.LowPart, &dwNumberOfBytes, NULL) || LargeFileSize.LowPart != dwNumberOfBytes) {
		std::cout << "Failed to read file in this application." << std::endl;
		delete[] lpBinary;
		CloseHandle(hFile);
		return false;
	}

	PIMAGE_DOS_HEADER pIDH = (PIMAGE_DOS_HEADER)lpBinary;
	if ( pIDH->e_magic != IMAGE_DOS_SIGNATURE || 
		pIDH->e_lfanew < sizeof(IMAGE_DOS_HEADER) || 
		(DWORD)pIDH->e_lfanew > LargeFileSize.LowPart) {
		std::cout << "This file is not supported in this application." << std::endl;
		delete[] lpBinary;
		CloseHandle(hFile);
		return false;
	}

	PIMAGE_NT_HEADERS pINH = (PIMAGE_NT_HEADERS)(lpBinary + pIDH->e_lfanew);
	if ( pINH->Signature != IMAGE_NT_SIGNATURE ||
		pINH->FileHeader.Machine != IMAGE_FILE_MACHINE_I386 ||
		pINH->FileHeader.NumberOfSections == 0L || 
		!(pINH->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE) ||
		!(pINH->FileHeader.Characteristics & IMAGE_FILE_32BIT_MACHINE) ||
		pINH->FileHeader.Characteristics & IMAGE_FILE_DLL ||
		pINH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress ) {
		std::cout << "This file is not supported in this application." << std::endl;
		delete[] lpBinary;
		CloseHandle(hFile);
		return false;
	}

	if (pINH->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE) {
		std::cout << "This file has ASLR enabled." << std::endl;
		if (bUnprotect) {
		Unprotect:
			pINH->OptionalHeader.DllCharacteristics &= ~IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE;
			if (SetFilePointer(hFile, pIDH->e_lfanew, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
				delete[] lpBinary;
				CloseHandle(hFile);
				return false;
			}
			if (WriteFile(hFile, pINH, sizeof(IMAGE_NT_HEADERS), &dwNumberOfBytes, NULL)) {
				std::cout << "Completed disable ASLR!" << std::endl;
			}else{
				delete[] lpBinary;
				CloseHandle(hFile);
				return false;
			}
		}else{
			char szConfirm[2];
			std::cout << "Would you like to disable ASLR? (Y/N)" << std::endl << "  ";
			scanf_s("%s", szConfirm, 2);
			if (!_strnicmp(szConfirm, "y", 2)) {
				goto Unprotect;
			}
		}
	}
	else {
		std::cout << "This file hasn't ASLR enabled." << std::endl;
	}

	delete [] lpBinary;
	CloseHandle(hFile);
	return true;
}
 
int main(int argc, char * argv[]) {
	std::cout << std::endl
		<< "    db    .dP\"Y8 88     88\"\"Yb  dP\"\"b8 88  88 88  dP " << std::endl
		<< "   dPYb   `Ybo.\" 88     88__dP dP   `\" 88  88 88odP  " << std::endl
		<< "  dP__Yb  o.`Y8b 88  .o 88\"Yb  Yb      888888 88\"Yb  " << std::endl
		<< " dP\"\"\"\"Yb 8bodP' 88ood8 88  Yb  YboodP 88  88 88  Yb " << std::endl << std::endl;

	if (argc == 2) {
		CheckFile(argv[1], false);
	}
	else if (argc == 3) {
		if (!_strnicmp(argv[1], "-d", 2)) {
			CheckFile(argv[2], true);
		}
		else {
			std::cout << "This option is not supported in this application." << std::endl
						<< "usage : aslrchk [-d] filepath" << std::endl;
			return 0;
		}
	}
	else {
		char szFilePath[MAX_PATH];
		ReCheck:
		std::cout << "Input the file path." << std::endl << "  ";
		scanf_s("%s", szFilePath, MAX_PATH);
		CheckFile(szFilePath, false);
	}

	char szConfirm[2];
	std::cout << "Do you want to check more? (Y/N)" << std::endl << "  ";
	scanf_s("%s", szConfirm, 2);
	if (!_strnicmp(szConfirm, "y", 2)) {
		std::cout << std::endl;
		goto ReCheck;
	}
	return 0;
}