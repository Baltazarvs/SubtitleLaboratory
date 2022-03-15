// 2020 - 2022 Baltazarus

#pragma once
#ifndef __cplusplus
	#error C++ is required for this application!
#endif

#define _WIN32_WINNT 0x501
#define _WIN32_IE 0x0300

#include <Windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include <vector>
#include <deque>
#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>

#pragma warning(disable: 4996)
#pragma comment(lib, "comctl32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define LP_CLASS_NAME		"Subtitle Lab - Baltazarus"

#define I_ERROR_CANNOT_PROCESS_MESSAGE			-0x01
#define I_ERROR_CANNOT_REGISTER_WCLASS			-0x02
#define I_ERROR_CANNOT_CREATE_WINDOW			-0x03

typedef struct
{
	int X, Y, Width, Height;
} WTransform, *LPWTransform;