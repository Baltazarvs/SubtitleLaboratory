// 2020 - 2022 Baltazarus

#pragma once
#ifndef APP_CLASS
#define APP_CLASS
#include "Utilities.h"
#include "resource.h"
#include "GDIPlusRenderer.h"
#include <iostream>

#define ID(_id)			reinterpret_cast<HMENU>(_id)
#define TIME_PLUS		1
#define TIME_MINUS		2

#define CRITERIA_FILE_OPEN_DEFAULT	0
#define CRITERIA_FILE_OPEN_WITH		1

typedef struct
{
	char projectName[255];			// A-Z a-z 1-9 _#
	char authorName[50];
	char projectDescription[500];
	char projectCreationDate[25];	// MM/DD/YYYY HH:MM:SS
	char projectLastModifyDate[25]; // MM/DD/YYYY HH:MM:SS
	char srtPath[MAX_PATH];
	BYTE sizeInBytes;
} SLProjectStruct, *LPSLProjectStruct;

#define CRITERIA_SAVE		1
#define CRITERIA_OPEN		2

class Application
{
private:
	class WClass
	{
	private:
		WClass();
		~WClass();
		static WClass WCInstance;
		static constexpr const char* WClassName = LP_CLASS_NAME;
		HINSTANCE w_Inst;

	public:
		static HINSTANCE GetInstance() noexcept;
		static const char* GetWClassName() noexcept;
	};

public:
	Application(HWND w_Parent, const char* Caption, WTransform w_Transform, std::wstring& open_with_path);
	Application(HWND w_Parent, const char* Caption, int X, int Y, int Width, int Height, std::wstring& open_with_path);
	~Application();

	static LRESULT __stdcall WndProcSetup(HWND w_Handle, UINT Msg, WPARAM wParam, LPARAM lParam);
	static LRESULT __stdcall Thunk(HWND w_Handle, UINT Msg, WPARAM wParam, LPARAM lParam);
	LRESULT __stdcall WndProc(HWND w_Handle, UINT Msg, WPARAM wParam, LPARAM lParam);

	static LRESULT __stdcall DlgProc_CreateProject(HWND w_Dlg, UINT Msg, WPARAM wParam, LPARAM lParam);
	static LRESULT __stdcall DlgProc_SrtEditor(HWND w_Dlg, UINT Msg, WPARAM wParam, LPARAM lParam);
	static LRESULT __stdcall DlgProc_ProjectInfo(HWND w_Dlg, UINT Msg, WPARAM wParam, LPARAM lParam);
	static LRESULT __stdcall DlgProc_AddSubtitle(HWND w_Dlg, UINT Msg, WPARAM wParam, LPARAM lParam);
	static LRESULT __stdcall DlgProc_ReviewSubtitle(HWND w_Dlg, UINT Msg, WPARAM wParam, LPARAM lParam);
	static LRESULT __stdcall DlgProc_GotoSubtitle(HWND w_Dlg, UINT Msg, WPARAM wParam, LPARAM lParam);
	static LRESULT __stdcall DlgProc_ErrorReport(HWND w_Dlg, UINT Msg, WPARAM wParam, LPARAM lParam);

	static LRESULT __stdcall SubclassProc_AddTitlePanel(HWND w_Handle, UINT Msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

	void SubtitleReviewDraw(HWND w_rwHandle, HDC hdc);
	void OpenSubtitleFile(HWND w_Handle, int criteria);
	HWND GetHandle() const noexcept;

	WTransform GetWindowTransform() const noexcept;
	std::pair<int, int> GetWindowScale() const noexcept;
	std::pair<int, int> GetWindowPosition() const noexcept;
	
	void RunMessageLoop();

private:
	HWND w_Handle;
	WTransform w_Transform;
	GDIPlusRenderer gp_Renderer;
	static std::wstring sopen_with_path;
};

#endif