// 2020 - 2022 Baltazarus

#include "SubtitleLaboratoryParser.h"
using namespace SubtitleLaboratory;

/*
 * TODO:
 *	ADD FUNCTION THAT CHECKS IF ALL TITLE INDEXES IN LIST ARE VALID! e.g. 12345...9 is valid, 12347 is invalid.
 *  IN resource.rc, ADD FUNCTIONALITY THAT SHOWS AND HIDES SPECIFIED WINDOWS (ToolBar, StatusBar) by checking item.
 * FIX ADD SUBTITLE DIALOG SYSTEM! MUST ADD SUBTITLE TO LIST WITH VALID INDEX!
 * WHEN LOADING SRT FILE, \n SHOULD BE ADDED FOR NEW LINES TO BE DISPLAYED IN TEXT COLUMN IN MAIN LIST! MUSTN'T BE NON-SEPARATED
 * AFTER EVERYTIME USER OPENS NEW SUBTITLE ON 'Open Subtitle' MENU, CLEAR WHOLE MAIN LIST AND IMPORT PARSED DATA FROM CURRENT OPEN FILE.
 *		USE bSubtitleFileOpened TO UNDERGO THIS OPERATION. IT WILL CHECK IF FILE IS ALREADY OPEN OR NOT, AND IF IT IS, IT WILL UNDERGO THIS OPERATION!
 * FIX THE OPERATION, WHEN USER OPENS SUBTITLE WITH INVALID TIMERS OR DATA, WHEN ERROR MESSAGE BOX APPEARS, CANCEL WHOLE OPENING OPERATION,
 *		AND DON'T INSERT ANY TITLES INTO A LIST OR SUBTITLE GLOBAL DEQUE (subtitles_deque)!!!\
 * 
 * INPUTING ONLY ONE NUMBER INSIDE HHMMSSMS EDITS IN ADD SUBTITLE WILL NOT ADD ZERO BEFORE SPECIFIED NUMBER. FIX IT.
 * E.G.    Input: 5, Output: 5, Expected: 05. Implement this system.
*/

#define DLG_DISPLAY_ERROR(par) DialogBox(nullptr, MAKEINTRESOURCE(IDD_ERRORREPORT), par, (DLGPROC)&Application::DlgProc_ErrorReport);

#define IDC_LV_EDITOR_LIST				20001
#define IDC_STATUS_BAR					20002
#define IDC_TOOL_BAR					20003

#define IDC_EDIT_SUBTITLE_TEXT			31000
#define IDC_EDIT_TIMER_BEGIN(hhmmssms)	32000 + hhmmssms
#define IDC_EDIT_TIMER_END(hhmmssms)	33000 + hhmmssms

#define ID_HH							100
#define ID_MM							200
#define ID_SS							300
#define ID_MS							400

#define IDC_BUTTON_ADD_TITLE			34011

#define HHMMSS_MAX_LEN					2
#define MS_MAX_LEN						3
#define IDC_STATIC_REVIEW_TITLE			24201
#define IDC_STATIC_ADD_CONTROL_PANEL    24202

#define IDC_CHECKBOX_SYNC_BEGINNING		34012

typedef struct
{
	int Width;
	int Height;
} WDimension;

Application::WClass Application::WClass::WCInstance;
std::wstring Application::sopen_with_path;
SubtitleLaboratory::SubRipParser obj_Parser;

void InitUI(HWND w_Handle, HINSTANCE w_Inst);
void SetupToolbar(HWND w_Toolbar);
void SetupStatusBar(HWND w_Statusbar);
bool LV_InsertColumns(HWND w_lvHandle, std::vector<const wchar_t*> Columns, void(*ptr_fix_col_size)(std::size_t&, int&));
bool LV_InsertItems(HWND, int, std::vector<const wchar_t*>);
std::string SaveOpenFilePath(HWND w_Handle, const char* title, const char* fFilter, int criteria);
std::wstring SaveOpenFilePathW(HWND w_Handle, const wchar_t* title, const wchar_t* fFilter, int criteria);
SLProjectStruct* LoadProject(const char* path);
bool SaveProject(const char* path, SLProjectStruct& projectStruct);
void InitializeSRTFileToListView(HWND w_lvHandle, const char* srtPath);
bool AddTitle(HWND w_lvHandle, SubtitleLaboratory::SubtitleContainer cnt);
bool SaveSRTToFile(HWND w_Handle);
bool IsTimerValid(SubtitleLaboratory::SubRipTimer timer);
bool IsTimerBeginEndValid(SubtitleLaboratory::SubRipTimer begin, SubtitleLaboratory::SubRipTimer end);
void RefreshTitleList(HWND w_lvHandle);
void AttachZerosIfSingleDigit(std::wostringstream& woss, SubtitleLaboratory::SubRipTimer& ref_timer);
double GetTitleDuration(int unit_id, SubtitleLaboratory::SubtitleContainer cnt);
double ConvertTimeUnit(int from_unit_id, int to_unit_id, unsigned long int amount);
HWND InitializeSubtitleReviewControl(HWND w_Handle);
WDimension GetWindowDimensions(RECT& rRect);
void UpdateReviewText(HWND w_rwHandle, const wchar_t* title_text);
void ShowHideControl(bool& bControlVar, int cID, HWND w_cHandle);
signed int LV_GetSelectedItemIndex(HWND w_lvHandle);
signed int LV_GetSelectedItemCount(HWND w_lvHandle);
void DeleteSubtitleItem(HWND w_lvHandle, int index);
SubtitleLaboratory::SubRipTimer CalculateTime(int criteria, SubtitleLaboratory::SubRipTimer first_time, SubtitleLaboratory::SubRipTimer second_time);
std::size_t FindCaseSensitive(std::wstring tstr, const wchar_t* wfind, std::size_t pos = 0ull);
void FindAllCaseSensitiveOccurrences(std::wstring tstr, const wchar_t* wfind, std::vector<std::size_t>& pos_vec);

void p_FixSizeForReviewList(std::size_t& for_index, int& cx);
void p_FixSizeMainList(std::size_t& for_index, int& cx);
void p_FixSizeForErrorList(std::size_t& for_index, int& cx);

// ============================ Runtime variables... ===============================
std::deque<SubtitleLaboratory::SubtitleContainer> subtitles_deque;
std::wstring current_opened_subtitle_path = NO_FILE_PATH;
static bool bSubtitleFileOpened = false;

static SLProjectStruct Runtime_LoadedProject; // Loaded project info.
static bool bRuntime_ProjectLoaded = false; // Checks if project is loaded.
static bool bRedrawReview = false;
static bool bRuntime_SubtitleSelected = false;
static std::wstring Runtime_SelectedSubtitleText = std::wstring();
static std::wstring Runtime_ErrorBoxString = std::wstring();
static int Runtime_ErrorBoxTitle = 0;

static int MainList_Width = 0;
static int MainList_Height = 0;

static bool bRuntime_SyncBeginning = true;
static bool bRuntime_OpenWith = false;

static std::wstring Runtime_LastSubtitleError = std::wstring();
static unsigned int Runtime_SubtitleIndex = 1u;

// ============================ Control handle variables... =========================
static HWND w_MainTitleList = nullptr;
static HWND w_StatusBar = nullptr;
static HWND w_ToolBar = nullptr;
static HWND w_AddTitlePanel = nullptr;

static HWND w_SubtitleText = nullptr;
static HWND w_TitleTimerBeginHH = nullptr, w_TitleTimerBeginMM = nullptr, w_TitleTimerBeginSS = nullptr, w_TitleTimerBeginMS = nullptr;
static HWND w_TitleTimerEndHH = nullptr, w_TitleTimerEndMM = nullptr, w_TitleTimerEndSS = nullptr, w_TitleTimerEndMS = nullptr;
static HWND w_ButtonAddTitle = nullptr;

static HWND w_SubtitleReview = nullptr;
static HWND w_CheckBoxSyncBeginTime = nullptr;

Application::WClass::WClass()
{
	WNDCLASSEXA wcex = { 0 };

	memset(&wcex, 0, sizeof(wcex));
	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_OWNDC;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.lpfnWndProc = &Application::WndProcSetup;
	wcex.hInstance = Application::WClass::GetInstance();
	wcex.hCursor = LoadCursorW(Application::WClass::GetInstance(), IDC_ARROW);
	wcex.hIcon = LoadIconW(Application::WClass::GetInstance(), IDI_APPLICATION);
	wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wcex.lpszClassName = Application::WClass::GetWClassName();
	wcex.lpszMenuName = MAKEINTRESOURCEA(IDR_MENUBAR);
	wcex.hIconSm = LoadIconW(this->GetInstance(), IDI_APPLICATION);

	if (!RegisterClassExA(&wcex))
		MessageBoxA(0, "Cannot Register Window Class!", "Error!", MB_OK | MB_ICONEXCLAMATION);
}

Application::WClass::~WClass()
{
	UnregisterClassA(this->GetWClassName(), this->GetInstance());
}

HINSTANCE Application::WClass::GetInstance() noexcept
{
	return WCInstance.w_Inst;
}

const char* Application::WClass::GetWClassName() noexcept
{
	return WCInstance.WClassName;
}

Application::Application(HWND w_Parent, const char* Caption, WTransform w_Transform, std::wstring& open_with_path)
	: w_Transform(w_Transform)
{
	Application::sopen_with_path = open_with_path;
	w_Handle = CreateWindowExA(
		WS_EX_CLIENTEDGE,
		Application::WClass::GetWClassName(),
		Caption,
		WS_OVERLAPPEDWINDOW,
		w_Transform.X, w_Transform.Y, w_Transform.Width, w_Transform.Height,
		w_Parent, nullptr, Application::WClass::GetInstance(), this
	);

	ShowWindow(this->GetHandle(), SW_SHOWMAXIMIZED);
}

Application::Application(HWND w_Parent, const char* Caption, int X, int Y, int Width, int Height, std::wstring& open_with_path)
	: Application(w_Parent, Caption, { X, Y, Width, Height }, open_with_path)
{ }

Application::~Application()
{
	DestroyWindow(this->GetHandle());
}

LRESULT __stdcall Application::WndProcSetup(HWND w_Handle, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (Msg == WM_NCCREATE)
	{
		const CREATESTRUCTA* const w_Create = reinterpret_cast<CREATESTRUCTA*>(lParam);
		Application* const w_App = reinterpret_cast<Application*>(w_Create->lpCreateParams);
		SetWindowLongPtrA(w_Handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(w_App));
		SetWindowLongPtrA(w_Handle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&Application::Thunk));
		return w_App->WndProc(w_Handle, Msg, wParam, lParam);
	}
	return DefWindowProcA(w_Handle, Msg, wParam, lParam);
}

LRESULT __stdcall Application::Thunk(HWND w_Handle, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	Application* const w_App = reinterpret_cast<Application*>(GetWindowLongPtrA(w_Handle, GWLP_WNDPROC));
	return w_App->WndProc(w_Handle, Msg, wParam, lParam);
}

LRESULT __stdcall Application::WndProc(HWND w_Handle, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	static bool bCheckView_ShowToolBar = true;
	static bool bCheckView_ShowStatusBar = true;
	static bool bCheckView_ShowTitleList = true;
	static bool bCheckView_ShowAddPanel = true;
	static bool bCheckView_ShowSubtitleReview = true;

	switch (Msg)
	{
	case WM_CREATE:
	{
		InitUI(w_Handle, Application::WClass::GetInstance());
		SetWindowSubclass(
			w_AddTitlePanel,
			reinterpret_cast<SUBCLASSPROC>(&Application::SubclassProc_AddTitlePanel),
			0u, 0u
		);

		if (!Application::sopen_with_path.empty())
		{
			bRuntime_OpenWith = true;
			OpenSubtitleFile(w_Handle, CRITERIA_FILE_OPEN_WITH);
		}
		break;
	}
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
			case ID_FILE_NEWPROJECT:
			{
				DialogBoxA(
					Application::WClass::GetInstance(),
					MAKEINTRESOURCEA(IDD_CREATEPROJECT),
					w_Handle,
					reinterpret_cast<DLGPROC>(&Application::DlgProc_CreateProject)
				);
				break;
			}
			case ID_FILE_OPENPROJECT:
			{
				SLProjectStruct projectStruct;
				char* c_buffer = new char[MAX_PATH];
				std::string path = SaveOpenFilePath(w_Handle, "Open Project", "Subtitle Laboratory Project\0*.slproj\0", CRITERIA_OPEN);
				if (path.length() < 1)
				{
					// If user decides to cancel the operation.
					delete[] c_buffer;
					return IDCANCEL;
				}
				strcpy(c_buffer, path.c_str());

				projectStruct = *LoadProject(c_buffer);
				// InitializeSRTFileToListView(w_MainTitleList, projectStruct.srtPath);
				memcpy(&::Runtime_LoadedProject, &projectStruct, sizeof(SLProjectStruct));
				SendMessageA(w_StatusBar, SB_SETTEXTA, 0u, reinterpret_cast<LPARAM>(c_buffer));
				SendMessageA(w_StatusBar, SB_SETTEXTA, 1u, reinterpret_cast<LPARAM>(projectStruct.srtPath));
				delete[] c_buffer;
				bRuntime_ProjectLoaded = true;
				break;
			}
			case ID_FILE_SAVEPROJECT:
			{
				break; // For now.
				// In Progress...
				SLProjectStruct projectStruct;
				memset(&projectStruct, 0, sizeof(SLProjectStruct));
				// Test.
				strcpy(projectStruct.authorName, "Baltazarus");
				strcpy(projectStruct.projectDescription, "This is test project and will not be used for real intention.");
				strcpy(projectStruct.projectCreationDate, __DATE__);
				strcpy(projectStruct.projectLastModifyDate, __DATE__);
				strcpy(projectStruct.projectName, "Test Project");
				projectStruct.sizeInBytes = (BYTE)sizeof(SLProjectStruct);
				strcpy(projectStruct.srtPath, "test.srt");

				char* buff = new char[MAX_PATH];
				strcpy(buff, SaveOpenFilePath(w_Handle, "Save Project", "Subtitle Laboratory Project\0*.slproj\0", CRITERIA_SAVE).c_str());
				SaveProject(buff, projectStruct);
				delete[] buff;
				if (!bRuntime_ProjectLoaded)
					bRuntime_ProjectLoaded = true;
				break;
			}
			case ID_FILE_NEWSUBTITLE:
			{
				::RefreshTitleList(w_MainTitleList);
				break;
			}
			case ID_FILE_OPENSUBTITLE:
			{
				this->OpenSubtitleFile(w_Handle, CRITERIA_FILE_OPEN_DEFAULT);
				break;
			}
			case ID_FILE_SAVESUBTITLE:
			{
				if (!::SaveSRTToFile(w_MainTitleList))
				{
					MessageBoxA(w_Handle, "Cannot save SRT to file.", "Error!", MB_OK | MB_ICONERROR);
					return -1;
				}
				break;
			}
			case ID_FILE_EXIT:
				DestroyWindow(w_Handle);
				break;
			case ID_EDIT_SRTEDITOR:
			{
				DialogBoxA(
					Application::WClass::GetInstance(),
					MAKEINTRESOURCEA(IDD_SRT_FILE_EDIT),
					w_Handle,
					reinterpret_cast<DLGPROC>(&Application::DlgProc_SrtEditor)
				);
				break;
			}
			case ID_EDIT_GOTOTITLE:
				DialogBoxA(
					GetModuleHandleA(nullptr), 
					MAKEINTRESOURCEA(IDD_GOTOTITLE), 
					w_Handle, 
					reinterpret_cast<DLGPROC>(&Application::DlgProc_GotoSubtitle)
				);
				break;
			case ID_HELP_PROJECTINFO:
			{
				DialogBoxA(
					Application::WClass::GetInstance(),
					MAKEINTRESOURCEA(IDD_PROJECTINFO),
					w_Handle,
					reinterpret_cast<DLGPROC>(&Application::DlgProc_ProjectInfo)
				);
				break;
			}
			case ID_VIEW_SHOW_TOOLBAR:
				::ShowHideControl(bCheckView_ShowToolBar, ID_VIEW_SHOW_TOOLBAR, w_ToolBar);
				break;
			case ID_VIEW_SHOW_STATUSBAR:
				::ShowHideControl(bCheckView_ShowStatusBar, ID_VIEW_SHOW_STATUSBAR, w_StatusBar);
				break;
			case ID_VIEW_SHOW_TITLE_LIST:
				::ShowHideControl(bCheckView_ShowTitleList, ID_VIEW_SHOW_TITLE_LIST, w_MainTitleList);
				break;
			case ID_VIEW_SHOWADDPANEL:
				::ShowHideControl(bCheckView_ShowAddPanel, ID_VIEW_SHOWADDPANEL, w_AddTitlePanel);
				break;
			case ID_VIEW_SHOWSUBTITLEREVIEW:
				::ShowHideControl(bCheckView_ShowSubtitleReview, ID_VIEW_SHOWSUBTITLEREVIEW, w_SubtitleReview);
				break;
			case ID_EDIT_ADDSUBTITLE:
			{
				DialogBox(
					Application::WClass::GetInstance(),
					MAKEINTRESOURCE(IDD_ADDSUBTITLE),
					w_Handle, reinterpret_cast<DLGPROC>(&Application::DlgProc_AddSubtitle)
				);
				break;
			}
			case ID_EDIT_CUT:
			{

				break;
			}
			case ID_EDIT_DELETESUBTITLE:
			{
				signed int selected_item_index = LV_GetSelectedItemIndex(w_MainTitleList);
				::DeleteSubtitleItem(w_MainTitleList, selected_item_index);
				break;
			}
			case ID_HELP_ABOUT:
			{
				DialogBox(
					Application::WClass::GetInstance(),
					MAKEINTRESOURCE(IDD_ABOUT),
					w_Handle,
					reinterpret_cast<DLGPROC>(&Application::DlgProc_About)
				);
				break;
			}
		}
		break;
	}
	case WM_NOTIFY:
	{
		switch (reinterpret_cast<LPNMHDR>(lParam)->code)
		{
			case LVN_COLUMNCLICK:
			{
				if (reinterpret_cast<LPNMHDR>(lParam)->idFrom == IDC_LV_EDITOR_LIST)
				{
					DialogBox(
						Application::WClass::GetInstance(), MAKEINTRESOURCE(IDD_REVIEWSRT),
						w_Handle, reinterpret_cast<DLGPROC>(&Application::DlgProc_ReviewSubtitle)
					);
				}
				break;
			}
			case NM_DBLCLK:
			{
				this->SubtitleReviewDraw(w_SubtitleReview, GetDC(w_SubtitleReview));
				break;
			}
		}
		break;
	}
	case WM_SIZE:
	{
		// TODO: FIX REVIEW SUBTITLE and ADD PANEL RECT POSITIONS ON RESIZE!
		RECT cwRect = { };
		GetClientRect(w_Handle, &cwRect);

		RECT lvRect = { };
		int lvHeight = 0;
		GetWindowRect(w_MainTitleList, &lvRect);

		RECT tbRect = { };
		int tbHeight = 0;
		SendMessage(w_ToolBar, TB_AUTOSIZE, 0u, 0u);
		GetWindowRect(w_ToolBar, &tbRect);
		tbHeight = tbRect.bottom - tbRect.top;

		RECT sbRect = { };
		int sbHeight = 0;
		SendMessage(w_StatusBar, WM_SIZE, 0u, 0u);
		SetupStatusBar(w_StatusBar);

		RECT rwRect;
		GetClientRect(w_SubtitleReview, &rwRect);

		RECT addRect;
		GetClientRect(w_AddTitlePanel, &addRect);

		GetClientRect(w_StatusBar, &sbRect);
		sbHeight = sbRect.bottom - sbRect.top;
		lvHeight = cwRect.bottom - sbHeight - tbHeight;

		MoveWindow(w_AddTitlePanel, 0, lvHeight - 56, cwRect.right - 400, 100, TRUE);

		GetClientRect(w_MainTitleList, &lvRect);
		GetClientRect(w_AddTitlePanel, &addRect);

		MoveWindow(w_MainTitleList, 0, tbHeight, cwRect.right, lvHeight - 100, TRUE);


		int wRect_Width = cwRect.right - cwRect.left;
		int wRect_Height = cwRect.bottom - cwRect.top;

		SendMessage(w_SubtitleReview, WM_SIZE, 0u, 0u);
		MoveWindow(
			w_SubtitleReview,
			(addRect.right - addRect.left) + 13,		// TODO: -2 should be removed if control overlaps the subtitle add panel.
			tbHeight + lvHeight - (addRect.bottom - addRect.top) - 15,
			wRect_Width - (addRect.right - addRect.left),
			wRect_Height - lvHeight + sbHeight + 12,
			TRUE
		);

		::MainList_Height = lvHeight;
		::MainList_Width = cwRect.right;

		break;
	}
	case WM_PAINT:
	{
		UpdateWindow(w_SubtitleReview);
		HDC hdcrw = GetDC(w_SubtitleReview);
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(w_Handle, &ps);
		this->SubtitleReviewDraw(w_SubtitleReview, hdc);
		EndPaint(w_Handle, &ps);
		break;
	}
	case WM_CLOSE:
		DestroyWindow(w_Handle);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProcA(w_Handle, Msg, wParam, lParam);
	}
	return 0;
}

LRESULT __stdcall Application::DlgProc_CreateProject(HWND w_Dlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HWND w_ProjectName = GetDlgItem(w_Dlg, IDC_EDIT_PROJECT_NAME);
	HWND w_ProjectAuthor = GetDlgItem(w_Dlg, IDC_EDIT_PROJECT_AUTHOR);
	HWND w_ProjectDescription = GetDlgItem(w_Dlg, IDC_EDIT_PROJECT_DESCRIPTION);
	HWND w_DescCounter = GetDlgItem(w_Dlg, IDC_STATIC_PROJECT_DESCRIPTION_CHARACTER_COUNT);

	switch (Msg)
	{
	case WM_INITDIALOG:
	{
		SendMessage(w_ProjectName, EM_SETCUEBANNER, 0, reinterpret_cast<LPARAM>(L"Project Name"));
		SendMessage(w_ProjectAuthor, EM_SETCUEBANNER, 0, reinterpret_cast<LPARAM>(L"Author Name"));
		SendMessage(w_ProjectDescription, EM_SETCUEBANNER, 0, reinterpret_cast<LPARAM>(L"Description"));
		break;
	}
	case WM_COMMAND:
	{
		if (wParam == IDR_CREATE_PROJECT)
		{
			// In Progress...
			EndDialog(w_Dlg, wParam);
		}
		if (wParam == IDCANCEL)
			EndDialog(w_Dlg, IDCANCEL);
		break;
	}
	case WM_KEYUP:
		EndDialog(w_Dlg, 0);
		break;
	case WM_CLOSE:
		EndDialog(w_Dlg, 0);
		break;
	}
	return 0;
}

LRESULT __stdcall Application::DlgProc_SrtEditor(HWND w_Dlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG:
	{
		std::fstream file;
		file.open(::Runtime_LoadedProject.srtPath, std::ios::in | std::ios::out);
		if (file.is_open())
		{
			std::string line;
			std::ostringstream oss;
			while (std::getline(file, line))
				oss << line.c_str() << '\r\n';
			SetWindowTextA(GetDlgItem(w_Dlg, IDC_EDIT_SRT_FILE), oss.str().c_str());
			file.close();
		}
		else
		{
			MessageBoxA(w_Dlg, "Cannot open file!", ::Runtime_LoadedProject.srtPath, MB_OK | MB_ICONERROR);
			EndDialog(w_Dlg, -1);
			return -1;
		}
		break;
	}
	case WM_CLOSE:
		EndDialog(w_Dlg, 0);
		break;
	}
	return 0;
}

LRESULT __stdcall Application::DlgProc_ProjectInfo(HWND w_Dlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HWND w_Name = GetDlgItem(w_Dlg, IDC_STATIC_PROJECT_NAME);
	HWND w_cDate = GetDlgItem(w_Dlg, IDC_STATIC_CREATE_DATE);
	HWND w_mDate = GetDlgItem(w_Dlg, IDC_STATIC_MODIFY_DATE);
	HWND w_Author = GetDlgItem(w_Dlg, IDC_STATIC_INFO_AUTHOR);
	HWND w_Size = GetDlgItem(w_Dlg, IDC_STATIC_PROJECT_SIZE_INFO);
	HWND w_SrtPath = GetDlgItem(w_Dlg, IDC_EDIT_PROJECT_SRT_PATH);

	switch (Msg)
	{
	case WM_INITDIALOG:
	{
		if (!bRuntime_ProjectLoaded)
			return 0;
		else
		{
			SetWindowTextA(w_Name, ::Runtime_LoadedProject.projectName);
			SetWindowTextA(w_cDate, ::Runtime_LoadedProject.projectCreationDate);
			SetWindowTextA(w_mDate, ::Runtime_LoadedProject.projectLastModifyDate);
			SetWindowTextA(w_Author, ::Runtime_LoadedProject.authorName);
			SetWindowTextA(w_Size, ConvertToString(::Runtime_LoadedProject.sizeInBytes).c_str());
			SetWindowTextA(w_SrtPath, ::Runtime_LoadedProject.srtPath);
		}
		break;
	}
	case WM_CLOSE:
		EndDialog(w_Dlg, 0);
		break;
	case WM_COMMAND:
		if (wParam == IDOK)
			EndDialog(w_Dlg, 0);
		break;
	}
	return 0;
}

LRESULT __stdcall Application::DlgProc_AddSubtitle(HWND w_Dlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HWND w_SubtitleText = GetDlgItem(w_Dlg, IDC_EDIT_SUBTITLETEXT);

	HWND w_TimerBeginHH = GetDlgItem(w_Dlg, IDC_EDIT_HH_BEGIN);
	HWND w_TimerBeginMM = GetDlgItem(w_Dlg, IDC_EDIT_MM_BEGIN);
	HWND w_TimerBeginSS = GetDlgItem(w_Dlg, IDC_EDIT_SS_BEGIN);
	HWND w_TimerBeginMS = GetDlgItem(w_Dlg, IDC_EDIT_MS_BEGIN);

	HWND w_TimerEndHH = GetDlgItem(w_Dlg, IDC_EDIT_HH_END);
	HWND w_TimerEndMM = GetDlgItem(w_Dlg, IDC_EDIT_MM_END);
	HWND w_TimerEndSS = GetDlgItem(w_Dlg, IDC_EDIT_SS_END);
	HWND w_TimerEndMS = GetDlgItem(w_Dlg, IDC_EDIT_MS_END);

	switch (Msg)
	{
	case WM_INITDIALOG:
		break;
	case WM_COMMAND:
	{
		if (LOWORD(wParam) == IDOK)
		{
			// Check if length of HH, MM and SS is <= 2, and MS <= 3
			if ((GetWindowTextLength(w_TimerBeginHH) > 2) ||
				(GetWindowTextLength(w_TimerBeginMM) > 2) ||
				(GetWindowTextLength(w_TimerBeginSS) > 2) ||
				(GetWindowTextLength(w_TimerBeginMS) > 3)
				)
			{
				MessageBoxA(
					w_Dlg,
					"Invalid length specified for begin timer!\nDigits must be:\nHH < 2\nMM < 2\nSS < 2\nMS < 3",
					"Invalid Value!",
					MB_OK | MB_ICONINFORMATION
				);
				return -1;
			}

			if ((GetWindowTextLength(w_TimerBeginHH) > 2) ||
				(GetWindowTextLength(w_TimerBeginMM) > 2) ||
				(GetWindowTextLength(w_TimerBeginSS) > 2) ||
				(GetWindowTextLength(w_TimerBeginMS) > 3)
				)
			{
				MessageBoxA(
					w_Dlg,
					"Invalid length specified for end timer!\nDigits must be:\nHH < 2\nMM < 2\nSS < 2\nMS < 3",
					"Invalid Value!",
					MB_OK | MB_ICONINFORMATION
				);
				return -1;
			}


			if ((GetWindowTextLength(w_TimerBeginHH) == 0) ||
				(GetWindowTextLength(w_TimerBeginMM) == 0) ||
				(GetWindowTextLength(w_TimerBeginSS) == 0) ||
				(GetWindowTextLength(w_TimerBeginMS) == 0) ||
				(GetWindowTextLength(w_TimerEndHH) == 0) ||
				(GetWindowTextLength(w_TimerEndMM) == 0) ||
				(GetWindowTextLength(w_TimerEndSS) == 0) ||
				(GetWindowTextLength(w_TimerEndMS) == 0) ||
				(GetWindowTextLength(w_SubtitleText) == 0)
				)
			{
				MessageBoxA(
					w_Dlg,
					"Specify needed values.",
					"Invalid Value!",
					MB_OK | MB_ICONINFORMATION
				);
				return -1;
			}

			// Values per one subtitle.
			unsigned int index = ::subtitles_deque.size() + 1;
			SubtitleLaboratory::SubRipTimer time_begin = { };
			SubtitleLaboratory::SubRipTimer time_end = { };
			SubtitleLaboratory::SubtitleContainer sct = { };
			wchar_t* lpstrText = nullptr;

			// Values that will temporary store the values
			unsigned int HHb, MMb, SSb, MSb; // Timer Begin
			unsigned int HHe, MMe, SSe, MSe; // Timer End

			// Begin Timer
			char cHHBegin[3]; // HH
			char cMMBegin[3]; // MM
			char cSSBegin[3]; // SS
			char cMSBegin[4]; // MS
			// End Timer
			char cHHEnd[3];   // HH
			char cMMEnd[3];	  // MM
			char cSSEnd[3];	  // SS
			char cMSEnd[4];	  // MS

			// Used for converting string to int.
			std::wstringstream wss;

			// Get values and store them into buffers above.
			GetWindowTextA(w_TimerBeginHH, cHHBegin, 2);  GetWindowTextA(w_TimerEndHH, cHHEnd, 2);
			GetWindowTextA(w_TimerBeginMM, cMMBegin, 2);  GetWindowTextA(w_TimerEndMM, cMMEnd, 2);
			GetWindowTextA(w_TimerBeginSS, cSSBegin, 2);  GetWindowTextA(w_TimerEndSS, cSSEnd, 2);
			GetWindowTextA(w_TimerBeginMS, cMSBegin, 3);  GetWindowTextA(w_TimerEndMS, cMSEnd, 3);

			wss << cHHBegin; wss >> HHb; wss.str(std::wstring()); wss.clear();
			wss << cHHEnd;   wss >> HHe; wss.str(std::wstring()); wss.clear();

			wss << cMMBegin; wss >> MMb; wss.str(std::wstring()); wss.clear();
			wss << cMMEnd;   wss >> MMe; wss.str(std::wstring()); wss.clear();

			wss << cSSBegin; wss >> SSb; wss.str(std::wstring()); wss.clear();
			wss << cSSEnd;   wss >> SSe; wss.str(std::wstring()); wss.clear();

			wss << cMSBegin; wss >> MSb; wss.str(std::wstring()); wss.clear();
			wss << cMSEnd;   wss >> MSe; wss.str(std::wstring()); wss.clear();

			// Breakpoint:  this case needs to be fixed. See breakpoint!
							// Get length of the subtitle text.
			int title_text_length = Edit_GetTextLength(w_SubtitleText);

			// Allocate buffer that will store the subtitle text.
			lpstrText = new wchar_t[title_text_length + 1ull]; // Needs more attention!
			if (lpstrText)
				GetWindowText(w_SubtitleText, lpstrText, title_text_length);

			sct.number = index;
			sct.time_begin = { HHb, MMb, SSb, MSb };
			sct.time_end = { HHe, MMe, SSe, MSe };
			sct.lpstrText = lpstrText;

			AddTitle(w_MainTitleList, sct);
			delete[] lpstrText;
			EndDialog(w_Dlg, IDOK);
			break;
		}
		if (LOWORD(wParam) == IDCANCEL)
			EndDialog(w_Dlg, IDCANCEL);
		break;
	}
	case WM_CLOSE:
		EndDialog(w_Dlg, 0);
		break;
	}
	return 0;
}

LRESULT __stdcall Application::DlgProc_ReviewSubtitle(HWND w_Dlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HWND w_ReviewEdit = GetDlgItem(w_Dlg, IDC_EDIT_SRT_REVIEW);
	HWND w_InfoList = GetDlgItem(w_Dlg, IDC_LIST_SRT_REVIEW_INFO);

	static unsigned int titles_number = 0u;

	switch (Msg)
	{
	case WM_INITDIALOG:
	{
		// If user change mind and wants to cancel opening of Subtitle Laboratory...
		if (!::bRuntime_OpenWith)
			ShowWindow(GetDlgItem(w_Dlg, IDCANCEL), SW_HIDE);

		// Initialize font for subtitle review edit control.
		HFONT hFont = CreateFont(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, L"Arial");
		SendMessage(w_ReviewEdit, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), 1u);

		// Initialize other stuff.
		std::size_t titles_number = obj_Parser.GetTitlesNumber(), lines_number = 0ull;

		SetWindowLongPtr(w_InfoList, GWL_EXSTYLE, static_cast<LONG_PTR>(LVS_EX_FULLROWSELECT));
		SendMessage(w_InfoList, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

		// Set title to the dialog window. If there is no subtitle opened, proceed to else.
		if (!current_opened_subtitle_path.empty())
			SetWindowText(w_Dlg, ::current_opened_subtitle_path.c_str());
		else
			SetWindowText(w_Dlg, L"No subtitle opened.");

		// Insert columns into a list view.
		std::vector<const wchar_t*> cols{ L"Info", L"Value" };
		if (!LV_InsertColumns(w_InfoList, cols, &::p_FixSizeForReviewList))
		{
			MessageBoxA(w_Dlg, "Cannot initialize info for SRT!", "Error!", MB_OK | MB_ICONERROR);
			return -1;
		}
		SetWindowLongPtr(w_InfoList, GWLP_USERDATA, static_cast<LONG_PTR>(cols.size()));

		// If path is valid or non-empty, fill list view with specified info, and import subtitle text into a edit.
		std::wostringstream woss;
		std::wfstream file;
		file.open(::current_opened_subtitle_path.c_str(), std::ios::in | std::ios::out);
		if (file.is_open())
		{
			file.seekg(0);
			std::wstring line;
			while (std::getline(file, line))
			{
				++lines_number;
				woss << line << L"\r\n";
			}
			SetWindowText(GetDlgItem(w_Dlg, IDC_EDIT_SRT_REVIEW), woss.str().c_str());
			file.close();
		}
		else
		{
			if (::current_opened_subtitle_path.empty())
				return 1;
			else
			{
				std::wostringstream woss;
				woss << L"Cannot review subtitle file!\n" << ::current_opened_subtitle_path << L" is invalid path!";
				MessageBoxW(w_Dlg, L"Cannot review subtitle file!", L"Error!", MB_OK | MB_ICONERROR);
			}
			return -1;
		}

		std::wostringstream woss_info;
		woss_info << obj_Parser.GetTitlesNumber();
		LV_InsertItems(w_InfoList, 0u, { L"Titles Count", woss_info.str().c_str() });
		woss_info.str(std::wstring());

		woss_info << lines_number;
		LV_InsertItems(w_InfoList, 0u, { L"Lines Count", woss_info.str().c_str() });
		woss_info.str(std::wstring());
		break;
	}
	case WM_CLOSE:
		EndDialog(w_Dlg, 0);
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
			EndDialog(w_Dlg, IDOK);
		if (LOWORD(wParam) == IDCANCEL)
			PostQuitMessage(0);
		break;
	}
	return 0;
}

LRESULT __stdcall Application::DlgProc_GotoSubtitle(HWND w_Dlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HWND w_TextBox = GetDlgItem(w_Dlg, IDC_EDIT_GOTO_INDEX);

	switch (Msg)
	{
		case WM_INITDIALOG:
			break;
		case WM_CLOSE:
			EndDialog(w_Dlg, 0);
			break;
		case WM_COMMAND:
		{
			if (LOWORD(wParam) == IDOK)
			{
				signed int index = 0;
				const std::size_t length = (std::size_t)Edit_GetTextLength(w_TextBox) + 1;
				char* buffer = new char[length + 1];

				SendMessageA(
					w_TextBox, WM_GETTEXT, 
					static_cast<WPARAM>(length),
					reinterpret_cast<LPARAM>(buffer) 
				);

				std::stringstream ss(buffer);
				ss >> index;

				if (index == 0)
				{
					MessageBoxA(
						w_Dlg,
						"Index must be greater than 0.",
						"Fail",
						MB_OK | MB_ICONINFORMATION
					);
					return 1;
				}

				index -= 1;

				delete[] buffer;
				
				if (index > ::subtitles_deque.size())
				{
					if (::subtitles_deque.size() < 1)
					{
						MessageBoxA(w_Dlg, "There are no subtitles loaded.", "Hey", MB_OK | MB_ICONINFORMATION);
						EndDialog(w_Dlg, 0);
						return 1;
					}

					MessageBoxA(
						w_Dlg,
						"Specified index is bigger than total number of subtitles.",
						"Fail",
						MB_OK | MB_ICONINFORMATION
					);
					return 1;
				}
				ListView_SetItemState(w_MainTitleList, index, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
				EndDialog(w_Dlg, 0);
			}
			if (LOWORD(wParam) == IDCANCEL)
				EndDialog(w_Dlg, IDCANCEL);
			break;
		}
	}
	return 0;
}

LRESULT __stdcall Application::DlgProc_ErrorReport(HWND w_Dlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HBRUSH dlgbr = CreateSolidBrush(GetSysColor(COLOR_WINDOW));

	switch (Msg)
	{
		case WM_INITDIALOG:
		{
			std::vector<const wchar_t*> cols{ L"Line", L"Title", L"Error Info" };
			LV_InsertColumns(GetDlgItem(w_Dlg, IDC_LIST_ERRORS_INFO), cols, &p_FixSizeForErrorList);
			SetWindowText(GetDlgItem(w_Dlg, IDC_STATIC_ERROR_REPORT), ::Runtime_LastSubtitleError.c_str());
			break;
		}
		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK)
				EndDialog(w_Dlg, IDOK);
			break;
		case WM_CTLCOLORSTATIC:
		{
			if (reinterpret_cast<HWND>(lParam) == GetDlgItem(w_Dlg, IDC_STATIC_ERROR_REPORT))
			{
				LOGBRUSH lbr;
				GetObject(dlgbr, sizeof(HBRUSH), &lbr);

				SetBkColor(reinterpret_cast<HDC>(wParam), lbr.lbColor);
				SetTextColor(reinterpret_cast<HDC>(wParam), RGB(0xFF, 0x00, 0x00));
			}
			return (LRESULT)dlgbr;
		}
		case WM_CLOSE:
			EndDialog(w_Dlg, 0);
			break;
	}
	return 0;
}

LRESULT __stdcall Application::DlgProc_About(HWND w_Dlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
		case WM_INITDIALOG:
		{
			wchar_t about_buffer[255];
			LoadString(GetModuleHandle(nullptr), IDS_STRING_ABOUT, about_buffer, 255);
			SetWindowText(GetDlgItem(w_Dlg, IDC_STATIC_ABOUT), about_buffer);
			break;
		}
		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK)
				EndDialog(w_Dlg, 0);
			break;
		case WM_CLOSE:
			EndDialog(w_Dlg, 0);
			break;
	}
	return 0;
}

LRESULT __stdcall Application::SubclassProc_AddTitlePanel(HWND w_Handle, UINT Msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (Msg)
	{
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDC_CHECKBOX_SYNC_BEGINNING:
				{
					if (::bRuntime_SyncBeginning)
					{
						bRuntime_SyncBeginning = false;
						Button_SetCheck(w_CheckBoxSyncBeginTime, BST_UNCHECKED);
					}
					else
					{
						bRuntime_SyncBeginning = true;
						Button_SetCheck(w_CheckBoxSyncBeginTime, BST_CHECKED);
					}
					break;
				}
				case IDC_BUTTON_ADD_TITLE:
				{
					SubtitleLaboratory::SubtitleContainer obj_Container;

					// Begin time length
					int b_hh_len = 1 + static_cast<int>(SendMessage(GetDlgItem(w_Handle, IDC_EDIT_TIMER_BEGIN(ID_HH)), WM_GETTEXTLENGTH, 0u, 0u));
					int b_mm_len = 1 + static_cast<int>(SendMessage(GetDlgItem(w_Handle, IDC_EDIT_TIMER_BEGIN(ID_MM)), WM_GETTEXTLENGTH, 0u, 0u));
					int b_ss_len = 1 + static_cast<int>(SendMessage(GetDlgItem(w_Handle, IDC_EDIT_TIMER_BEGIN(ID_SS)), WM_GETTEXTLENGTH, 0u, 0u));
					int b_ms_len = 1 + static_cast<int>(SendMessage(GetDlgItem(w_Handle, IDC_EDIT_TIMER_BEGIN(ID_MS)), WM_GETTEXTLENGTH, 0u, 0u));

					// End time length
					int e_hh_len = 1 + static_cast<int>(SendMessage(GetDlgItem(w_Handle, IDC_EDIT_TIMER_END(ID_HH)), WM_GETTEXTLENGTH, 0u, 0u));
					int e_mm_len = 1 + static_cast<int>(SendMessage(GetDlgItem(w_Handle, IDC_EDIT_TIMER_END(ID_MM)), WM_GETTEXTLENGTH, 0u, 0u));
					int e_ss_len = 1 + static_cast<int>(SendMessage(GetDlgItem(w_Handle, IDC_EDIT_TIMER_END(ID_SS)), WM_GETTEXTLENGTH, 0u, 0u));
					int e_ms_len = 1 + static_cast<int>(SendMessage(GetDlgItem(w_Handle, IDC_EDIT_TIMER_END(ID_MS)), WM_GETTEXTLENGTH, 0u, 0u));

					if (
						b_hh_len - 1 < 1 || b_mm_len - 1 < 1 || b_ss_len - 1 < 1 || b_ms_len - 1 < 1 ||
						e_hh_len - 1 < 1 || e_mm_len - 1 < 1 || e_ss_len - 1 < 1 || e_ms_len - 1 < 1
					)
					{
						// If there if empty field...
						MessageBox(GetParent(w_Handle), L"Specify needed values.", L"Specify", MB_OK);
						return 1;
					}

					int subtitle_text_len = SendMessage(GetDlgItem(w_Handle, IDC_EDIT_SUBTITLE_TEXT), WM_GETTEXTLENGTH, 0u, 0u) + 1;
					if (subtitle_text_len - 1 < 1)
					{
						MessageBox(w_Handle, L"Enter subtitle text.", L"Subtitle requires text.", MB_OK);
						return 0;
					}

					// Begin Timer
					wchar_t* b_hh_buff = new wchar_t[b_hh_len];
					if (!b_hh_buff) { MessageBoxA(w_Handle, "Cannot allocate buffer for START HOUR!", "Error!", MB_OK | MB_ICONERROR); return -1; }
					SendMessage(GetDlgItem(w_Handle, IDC_EDIT_TIMER_BEGIN(ID_HH)), WM_GETTEXT, static_cast<WPARAM>(b_hh_len), reinterpret_cast<LPARAM>(b_hh_buff));

					wchar_t* b_mm_buff = new wchar_t[b_mm_len];
					if (!b_hh_buff) { MessageBoxA(w_Handle, "Cannot allocate buffer for START MINUTE!", "Error!", MB_OK | MB_ICONERROR); return -1; }
					SendMessage(GetDlgItem(w_Handle, IDC_EDIT_TIMER_BEGIN(ID_MM)), WM_GETTEXT, static_cast<WPARAM>(b_mm_len), reinterpret_cast<LPARAM>(b_mm_buff));

					wchar_t* b_ss_buff = new wchar_t[b_ss_len];
					if (!b_hh_buff) { MessageBoxA(w_Handle, "Cannot allocate buffer for START SECOND!", "Error!", MB_OK | MB_ICONERROR); return -1; }
					SendMessage(GetDlgItem(w_Handle, IDC_EDIT_TIMER_BEGIN(ID_SS)), WM_GETTEXT, static_cast<WPARAM>(b_ss_len), reinterpret_cast<LPARAM>(b_ss_buff));

					wchar_t* b_ms_buff = new wchar_t[b_ms_len];
					if (!b_ms_buff) { MessageBoxA(w_Handle, "Cannot allocate buffer for START MILLISECOND!", "Error!", MB_OK | MB_ICONERROR); return -1; }
					SendMessage(GetDlgItem(w_Handle, IDC_EDIT_TIMER_BEGIN(ID_MS)), WM_GETTEXT, static_cast<WPARAM>(b_ms_len), reinterpret_cast<LPARAM>(b_ms_buff));


					// End Timer
					wchar_t* e_hh_buff = new wchar_t[e_hh_len];
					if (!e_hh_buff) { MessageBoxA(w_Handle, "Cannot allocate buffer for END HOUR!", "Error!", MB_OK | MB_ICONERROR); return -1; }
					SendMessage(GetDlgItem(w_Handle, IDC_EDIT_TIMER_END(ID_HH)), WM_GETTEXT, static_cast<WPARAM>(e_hh_len), reinterpret_cast<LPARAM>(e_hh_buff));

					wchar_t* e_mm_buff = new wchar_t[e_mm_len];
					if (!e_mm_buff) { MessageBoxA(w_Handle, "Cannot allocate buffer for END MINUTE!", "Error!", MB_OK | MB_ICONERROR); return -1; }
					SendMessage(GetDlgItem(w_Handle, IDC_EDIT_TIMER_END(ID_MM)), WM_GETTEXT, static_cast<WPARAM>(e_mm_len), reinterpret_cast<LPARAM>(e_mm_buff));

					wchar_t* e_ss_buff = new wchar_t[e_ss_len];
					if (!e_ss_buff) { MessageBoxA(w_Handle, "Cannot allocate buffer for END SECOND!", "Error!", MB_OK | MB_ICONERROR); return -1; }
					SendMessage(GetDlgItem(w_Handle, IDC_EDIT_TIMER_END(ID_SS)), WM_GETTEXT, static_cast<WPARAM>(e_ss_len), reinterpret_cast<LPARAM>(e_ss_buff));

					wchar_t* e_ms_buff = new wchar_t[e_ms_len];
					if (!e_ms_buff) { MessageBoxA(w_Handle, "Cannot allocate buffer for START MILLISECOND!", "Error!", MB_OK | MB_ICONERROR); return -1; }
					SendMessage(GetDlgItem(w_Handle, IDC_EDIT_TIMER_END(ID_MS)), WM_GETTEXT, static_cast<WPARAM>(e_ms_len), reinterpret_cast<LPARAM>(e_ms_buff));

					// Convert time strings to unique SubRipTimer object
					SubtitleLaboratory::SubRipTimer begin_timer = obj_Parser.ConvertTimerStringToTimerObject(b_hh_buff, b_mm_buff, b_ss_buff, b_ms_buff);
					SubtitleLaboratory::SubRipTimer end_timer = obj_Parser.ConvertTimerStringToTimerObject(e_hh_buff, e_mm_buff, e_ss_buff, e_ms_buff);
					
					// Subtitle index number
					::subtitles_deque.size();

					// Subtitle Text
					wchar_t* c_buffer = new wchar_t[subtitle_text_len];
					if (!c_buffer)
					{
						MessageBoxA(w_Handle, "Cannot allocate buffer for title text!", "Error!", MB_OK | MB_ICONERROR);
						return -1;
					}

					GetWindowText(w_SubtitleText, c_buffer, subtitle_text_len);

					obj_Container.number = ::Runtime_SubtitleIndex;
					obj_Container.time_begin = begin_timer;
					obj_Container.time_end = end_timer;
					obj_Container.lpstrText = c_buffer;
					delete[] c_buffer;

					if (!::AddTitle(w_MainTitleList, obj_Container))
						return -1;

					::Runtime_SubtitleIndex += 1;

					int tpes[] = { ID_HH, ID_MM, ID_SS, ID_MS };
					for (int i = 0; i < (sizeof(tpes) / sizeof(tpes[0])) * 2; ++i)
					{
						SetWindowText(GetDlgItem(w_Handle, IDC_EDIT_TIMER_BEGIN(tpes[i])), nullptr);
						SetWindowText(GetDlgItem(w_Handle, IDC_EDIT_TIMER_END(tpes[i])), nullptr);
					}

					if (::bRuntime_SyncBeginning)
					{
						const wchar_t* buffers_ptr[] = { e_hh_buff, e_mm_buff, e_ss_buff, e_ms_buff };
						for (int i = 0; i < sizeof(buffers_ptr) / sizeof(buffers_ptr[0]); ++i)
						{
							if (i < sizeof(buffers_ptr) / sizeof(buffers_ptr[0]) - 1)
								SetWindowText(GetDlgItem(w_Handle, IDC_EDIT_TIMER_BEGIN(tpes[i])), buffers_ptr[i]);
							else
							{
								// TODO: This will break rule of timing if MS is 999. It mustn't be 1000
								// Must use system to sync time (1000 ms will add one second to HH)

								std::wstringstream woss;
								int converted_val = 0;

								woss << e_ms_buff;
								woss >> converted_val;
								converted_val += 1;

								woss.clear(); 
								woss.str(std::wstring());
								woss << converted_val;
								SetWindowText(GetDlgItem(w_Handle, IDC_EDIT_TIMER_BEGIN(tpes[i])), woss.str().c_str());
							}
						}
					}

					// Release buffers.
					delete[] b_hh_buff; delete[] b_mm_buff; delete[] b_ss_buff;
					delete[] e_hh_buff; delete[] e_mm_buff; delete[] e_ss_buff;
					
					SetWindowText(w_SubtitleText, nullptr);
					break;
				}
			}
			break;
		}
		default:
			return DefSubclassProc(w_Handle, Msg, wParam, lParam);
	}
	return 0;
}

void Application::SubtitleReviewDraw(HWND w_rwHandle, HDC hdc)
{
	RECT rwRect;
	GetWindowRect(w_rwHandle, &rwRect);

	HDC rwhdc = GetDC(w_rwHandle);
	Gdiplus::Graphics gfx(rwhdc);
	Gdiplus::Pen bg_pen(Gdiplus::Color(0x00, 0x00, 0xFF));

	Gdiplus::SolidBrush bg_brush(Gdiplus::Color(0x00, 0x00, 0xFF));
	Gdiplus::SolidBrush tx_brush(Gdiplus::Color(0xFF, 0xFF, 0xFF));

	WDimension w_dimensions = GetWindowDimensions(rwRect);

	Gdiplus::Rect rc1;
	rc1.X = 0;
	rc1.Y = 0;
	rc1.Width = w_dimensions.Width;
	rc1.Height = w_dimensions.Height;
	gfx.FillRectangle(&bg_brush, rc1);

	Gdiplus::PointF origin;
	rc1.X = w_dimensions.Width / 2;
	rc1.Y = w_dimensions.Height / 2;

	HFONT hFont = CreateFontW(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, L"Arial");
	Gdiplus::Font gpFont(rwhdc, hFont);
	if (!bRuntime_SubtitleSelected)
		gfx.DrawString(L"*No subtitle selected.\n*Double click on subtitle from list.", 59, &gpFont, origin, &tx_brush);
	else
		gfx.DrawString(::Runtime_SelectedSubtitleText.c_str(), Runtime_SelectedSubtitleText.length(), &gpFont, origin, &tx_brush);
	return;
}

void Application::OpenSubtitleFile(HWND w_Handle, int criteria)
{
	try
	{
		static bool bErr = false;
		std::wstring path = std::wstring();
		if (criteria == CRITERIA_FILE_OPEN_WITH)
			path = this->sopen_with_path;
		else
			path = ::SaveOpenFilePathW(w_Handle, L"Open Subtitle", L"SubRip File (*.srt)\0*.srt\0", CRITERIA_OPEN);
		std::deque<SubtitleLaboratory::SubtitleContainer> parsed_titles = std::deque<SubtitleLaboratory::SubtitleContainer>();

		// If file is already opened...
		if (path != current_opened_subtitle_path)
			current_opened_subtitle_path = path;
		else
			return;

		if (path.length() > 1)
			parsed_titles = obj_Parser.ParseSubtitleFromFile(path.c_str());
		::Runtime_SubtitleIndex = parsed_titles.size() + 1ull;

		for (std::size_t i = 0ull; i < parsed_titles.size(); ++i)
		{
			if (!IsTimerBeginEndValid(parsed_titles[i].time_begin, parsed_titles[i].time_end))
			{
				std::wostringstream oss;
				oss << L"Invalid timer specified at subtitle [" << i + 1 << ']' << std::endl;
				oss << L"Reason: " << ::Runtime_LastSubtitleError << std::endl;
				oss << L"Operation Canceled." << std::endl;
				MessageBoxW(w_Handle, oss.str().c_str(), L"Parsing Error!", MB_OK | MB_ICONERROR);
				bErr = true;
			}
			else
			{
				if (!bErr)
					::AddTitle(w_MainTitleList, parsed_titles[i]);
			}
		}
		if (!bErr && path.length() > 0)
		{
			DialogBox(
				Application::WClass::GetInstance(),
				MAKEINTRESOURCE(IDD_REVIEWSRT),
				w_Handle,
				reinterpret_cast<DLGPROC>(&Application::DlgProc_ReviewSubtitle)
			);
			SendMessage(w_StatusBar, SB_SETTEXTW, 1u, reinterpret_cast<LPARAM>(path.c_str()));
		}
	}
	catch (std::exception& e)
	{
		MessageBoxA(w_Handle, e.what(), "Exception Thrown!", MB_OK | MB_ICONERROR);
	}
	return;
}

HWND Application::GetHandle() const noexcept
{
	return this->w_Handle;
}

WTransform Application::GetWindowTransform() const noexcept
{
	return this->w_Transform;
}

std::pair<int, int> Application::GetWindowScale() const noexcept
{
	return std::pair<int, int>(GetWindowTransform().Width, GetWindowTransform().Height);
}

std::pair<int, int> Application::GetWindowPosition() const noexcept
{
	return std::pair<int, int>(GetWindowTransform().X, GetWindowTransform().Y);
}

void Application::RunMessageLoop()
{
	MSG Msg = { };

	while (GetMessageA(&Msg, nullptr, 0, 0) > 0)
	{
		if (LV_GetSelectedItemCount(w_MainTitleList) < 1)
			Runtime_SelectedSubtitleText = L"No subtitle selected.";
		else
		{
			bRuntime_SubtitleSelected = true;
			SubtitleLaboratory::SubtitleContainer title_obj = ::subtitles_deque[LV_GetSelectedItemIndex(w_MainTitleList)];
			::Runtime_SelectedSubtitleText = title_obj.lpstrText;
			UpdateWindow(w_SubtitleReview);
		}
		TranslateMessage(&Msg);
		DispatchMessageA(&Msg);
	}
	return;
}

void InitUI(HWND w_Handle, HINSTANCE w_Inst)
{
	DWORD defWndStyle = (WS_VISIBLE | WS_CHILD);

	w_MainTitleList = CreateWindowExA(
		LVS_EX_FULLROWSELECT, WC_LISTVIEWA, nullptr,
		defWndStyle | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
		0, 0, 0, 0,
		w_Handle, ID(IDC_LV_EDITOR_LIST), w_Inst, nullptr
	);

	SendMessageA(
		w_MainTitleList,
		LVM_SETEXTENDEDLISTVIEWSTYLE,
		(WPARAM)LVS_EX_FULLROWSELECT,
		(LPARAM)LVS_EX_FULLROWSELECT
	);

	// Initialize columns of the list view.
	std::vector<const wchar_t*> Columns{ L"#", L"Begin Time", L"End Time",L"Text" };
	LV_InsertColumns(w_MainTitleList, Columns, &::p_FixSizeMainList);
	SetWindowLongPtrA(w_MainTitleList, GWLP_USERDATA, (LONG_PTR)Columns.size());


	w_StatusBar = CreateWindowExA(
		0, STATUSCLASSNAMEA, nullptr,
		defWndStyle | SBS_SIZEGRIP,
		0, 0, 0, 0,
		w_Handle, ID(IDC_STATUS_BAR), w_Inst, nullptr
	);

	SetupStatusBar(w_StatusBar);
	SendMessageA(w_StatusBar, SB_SETTEXTA, 0u, reinterpret_cast<LPARAM>("No project loaded."));
	SendMessageA(w_StatusBar, SB_SETTEXTA, 1u, reinterpret_cast<LPARAM>("No Subtitle loaded."));

	w_ToolBar = CreateWindowExA(
		0, TOOLBARCLASSNAMEA, nullptr,
		defWndStyle,
		0, 0, 0, 0,
		w_Handle, ID(IDC_TOOL_BAR), w_Inst, nullptr
	);
	SetupToolbar(w_ToolBar);

	w_AddTitlePanel = CreateWindow(
		WC_STATIC, nullptr,
		WS_VISIBLE | WS_CHILD | WS_THICKFRAME,
		0, 0, 0, 0,
		w_Handle, reinterpret_cast<HMENU>(IDC_STATIC_ADD_CONTROL_PANEL), w_Inst, nullptr
	);

	const int timer_comps_num = 4;
	HWND w_AddonsB[timer_comps_num] = { w_TitleTimerBeginHH, w_TitleTimerBeginMM, w_TitleTimerBeginSS, w_TitleTimerBeginMS };
	HWND w_AddonsE[timer_comps_num] = { w_TitleTimerEndHH, w_TitleTimerEndMM, w_TitleTimerEndSS, w_TitleTimerEndMS };
	int Id_Arr[timer_comps_num] = { ID_HH, ID_MM, ID_SS, ID_MS };

	HWND w_BeginStatic = CreateWindow(
		WC_STATIC, L"Begin",
		WS_VISIBLE | WS_CHILD,
		0, 2, 30, 20,
		w_AddTitlePanel, nullptr, w_Inst, nullptr
	);

	HWND w_EndStatic = CreateWindow(
		WC_STATIC, L"End",
		WS_VISIBLE | WS_CHILD,
		0, 22, 30, 20,
		w_AddTitlePanel, nullptr, w_Inst, nullptr
	);

	for (std::size_t i = 0ull; i < timer_comps_num; ++i)
	{
		static int increment_pos = 0;

		w_AddonsB[i] = CreateWindow(
			WC_EDIT, nullptr,
			WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
			40 + increment_pos, 0, 40, 20,
			w_AddTitlePanel, reinterpret_cast<HMENU>(IDC_EDIT_TIMER_BEGIN(Id_Arr[i])), w_Inst, nullptr
		);

		increment_pos += 41;
		SendMessage(w_AddonsB[i], WM_SETFONT, reinterpret_cast<WPARAM>((HFONT)GetStockObject(DEFAULT_GUI_FONT)), 1u);
	}
	for (std::size_t i = 0ull; i < timer_comps_num; ++i)
	{
		static int increment_pos = 0;
		w_AddonsE[i] = CreateWindow(
			WC_EDIT, nullptr,
			WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
			40 + increment_pos, 21, 40, 20,
			w_AddTitlePanel, reinterpret_cast<HMENU>(IDC_EDIT_TIMER_END(Id_Arr[i])), w_Inst, nullptr
		);
		increment_pos += 41;
		SendMessage(w_AddonsE[i], WM_SETFONT, reinterpret_cast<WPARAM>((HFONT)GetStockObject(DEFAULT_GUI_FONT)), 1u);
	}

	w_SubtitleText = CreateWindow(
		WC_EDIT, nullptr,
		WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE,
		40, 42, 200, 40,
		w_AddTitlePanel, reinterpret_cast<HMENU>(IDC_EDIT_SUBTITLE_TEXT), w_Inst, nullptr
	);

	w_ButtonAddTitle = CreateWindow(
		WC_BUTTON, L"  Add",
		WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
		246, 42, 100, 40,
		w_AddTitlePanel, reinterpret_cast<HMENU>(IDC_BUTTON_ADD_TITLE), w_Inst, nullptr
	);

	HICON hAddIcon = (HICON)LoadImageA(
		GetModuleHandleA(nullptr),
		MAKEINTRESOURCEA(IDI_ICON_ADD),
		IMAGE_ICON,
		24, 24,
		LR_DEFAULTCOLOR
	);

	SendMessage(w_ButtonAddTitle, BM_SETIMAGE, static_cast<WPARAM>(IMAGE_ICON), reinterpret_cast<LPARAM>(hAddIcon));

	w_CheckBoxSyncBeginTime = CreateWindow(
		WC_BUTTON, L"Sync Beginning",
		WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
		210, 10, 100, 20,
		w_AddTitlePanel, ID(IDC_CHECKBOX_SYNC_BEGINNING), w_Inst, nullptr
	);
	Button_SetCheck(w_CheckBoxSyncBeginTime, BST_CHECKED);
	
	w_SubtitleReview = ::InitializeSubtitleReviewControl(w_Handle);

	const std::size_t controls_num = 5ull;
	HWND w_Controls[controls_num] = { w_SubtitleText, w_ButtonAddTitle, w_BeginStatic, w_EndStatic, w_CheckBoxSyncBeginTime };
	for (std::size_t i = 0ull; i < controls_num; ++i)
		SendMessage(w_Controls[i], WM_SETFONT, reinterpret_cast<WPARAM>((HFONT)GetStockObject(DEFAULT_GUI_FONT)), 1u);
	return;
}

void SetupToolbar(HWND w_Toolbar)
{
	SendMessageA(w_Toolbar, TB_BUTTONSTRUCTSIZE, static_cast<WPARAM>(sizeof(TBBUTTON)), 0u);

	TBADDBITMAP tbb;
	tbb.hInst = HINST_COMMCTRL;
	tbb.nID = IDB_STD_SMALL_COLOR;
	SendMessageA(w_Toolbar, TB_ADDBITMAP, 0u, reinterpret_cast<LPARAM>(&tbb));

	TBBUTTON tbbs[11] = { };

	memset(&tbbs, 0, sizeof(TBBUTTON));
	tbbs[0].iBitmap = STD_FILENEW;
	tbbs[0].fsState = TBSTATE_ENABLED;
	tbbs[0].fsStyle = TBSTYLE_BUTTON;
	tbbs[0].idCommand = ID_FILE_NEW;
	tbbs[0].iString = (INT_PTR)"New";

	tbbs[1].iBitmap = STD_FILEOPEN;
	tbbs[1].fsState = TBSTATE_ENABLED;
	tbbs[1].fsStyle = TBSTYLE_BUTTON;
	tbbs[1].idCommand = ID_FILE_OPEN;
	tbbs[1].iString = (INT_PTR)"Open";

	tbbs[2].iBitmap = STD_FILESAVE;
	tbbs[2].fsState = TBSTATE_ENABLED;
	tbbs[2].fsStyle = TBSTYLE_BUTTON;
	tbbs[2].idCommand = ID_FILE_SAVE;
	tbbs[2].iString = (INT_PTR)"Save";

	tbbs[3].iBitmap = STD_DELETE;
	tbbs[3].fsState = TBSTATE_ENABLED;
	tbbs[3].fsStyle = TBSTYLE_BUTTON;
	tbbs[3].idCommand = ID_FILE_EXIT;
	tbbs[3].iString = (INT_PTR)"Exit";

	tbbs[4].fsStyle = TBSTYLE_SEP;

	tbbs[5].iBitmap = STD_CUT;
	tbbs[5].fsState = TBSTATE_ENABLED;
	tbbs[5].fsStyle = TBSTYLE_BUTTON;
	tbbs[5].idCommand = ID_EDIT_CUT;
	tbbs[5].iString = (INT_PTR)"Cut";

	tbbs[6].iBitmap = STD_COPY;
	tbbs[6].fsState = TBSTATE_ENABLED;
	tbbs[6].fsStyle = TBSTYLE_BUTTON;
	tbbs[6].idCommand = ID_EDIT_COPY;
	tbbs[6].iString = (INT_PTR)"Copy";

	tbbs[7].iBitmap = STD_PASTE;
	tbbs[7].fsState = TBSTATE_ENABLED;
	tbbs[7].fsStyle = TBSTYLE_BUTTON;
	tbbs[7].idCommand = ID_EDIT_PASTE;
	tbbs[7].iString = (INT_PTR)"Paste";

	tbbs[8].fsStyle = TBSTYLE_SEP;

	tbbs[9].iBitmap = STD_PASTE;
	tbbs[9].fsState = TBSTATE_ENABLED;
	tbbs[9].fsStyle = TBSTYLE_BUTTON;
	tbbs[9].idCommand = ID_EDIT_ADDSUBTITLE;
	tbbs[9].iString = (INT_PTR)"Add";

	tbbs[10].iBitmap = STD_DELETE;
	tbbs[10].fsState = TBSTATE_ENABLED;
	tbbs[10].fsStyle = TBSTYLE_BUTTON;
	tbbs[10].idCommand = ID_EDIT_DELETESUBTITLE;
	tbbs[10].iString = (INT_PTR)"Delete";

	SendMessageA(w_Toolbar, TB_ADDBUTTONSA, (sizeof(tbbs) / sizeof(TBBUTTON)), reinterpret_cast<LPARAM>(&tbbs));
	return;
}

void SetupStatusBar(HWND w_Statusbar)
{
	RECT cRect = { };
	GetClientRect(GetParent(w_StatusBar), &cRect);

	int iParts[3] = { cRect.right / 3, cRect.right - 100, -1 };

	SendMessageA(
		w_Statusbar, SB_SETPARTS,
		static_cast<WPARAM>((sizeof(iParts) / sizeof(iParts[0]))),
		reinterpret_cast<LPARAM>(iParts)
	);
}

bool LV_InsertColumns(HWND w_lvHandle, std::vector<const wchar_t*> Columns, void(*ptr_fix_col_size)(std::size_t&, int&))
{
	LVCOLUMN lvc = { 0 };

	memset(&lvc, 0, sizeof(lvc));
	lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	lvc.iSubItem = 0;
	lvc.cx = 300;
	lvc.pszText = const_cast<wchar_t*>(Columns[0]);

	for (std::size_t i = 0; i < Columns.size(); ++i)
	{
		lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
		lvc.iSubItem = i;

		if(ptr_fix_col_size != nullptr)
			ptr_fix_col_size(i, lvc.cx);

		lvc.pszText = const_cast<wchar_t*>(Columns[i]);
		if (SendMessageA(w_lvHandle, LVM_INSERTCOLUMN, static_cast<WPARAM>(i), reinterpret_cast<LPARAM>(&lvc)) == -1)
			return false;
	}
	return true;
}

bool LV_InsertItems(HWND w_lvHandle, int iItem, std::vector<const wchar_t*> Items)
{
	//if ((Items.size() == GetWindowLongPtrA(w_lvHandle, GWLP_USERDATA)) || (Items.size() < GetWindowLongPtrA(w_lvHandle, GWLP_USERDATA)))
	if ((Items.size() == GetWindowLongPtrA(w_lvHandle, GWLP_USERDATA)))
	{
		LVITEM lvi = { 0 };

		memset(&lvi, 0, sizeof(lvi));
		lvi.mask = LVIF_TEXT;
		lvi.iItem = iItem;
		lvi.iSubItem = 0;
		lvi.pszText = const_cast<wchar_t*>(Items[0]);
		ListView_InsertItem(w_lvHandle, &lvi);
		for (std::size_t i = 0; i < Items.size(); ++i)
		{
			lvi.mask = LVIF_TEXT;
			lvi.iItem = iItem;
			lvi.iSubItem = i;
			lvi.pszText = const_cast<wchar_t*>(Items[i]);
			if (SendMessageA(w_lvHandle, LVM_SETITEM, 0, reinterpret_cast<LPARAM>(&lvi)) == -1)
				return false;
		}
	}
	else
	{
		std::ostringstream oss;
		oss << "Cannot add " << Items.size() << " items in list with " << GetWindowLongPtrA(w_lvHandle, GWLP_USERDATA) << " columns!";
		MessageBoxA(GetParent(w_lvHandle), oss.str().c_str(), "Error!", MB_OK | MB_ICONERROR);
		return false;
	}
	return true;
}

std::string SaveOpenFilePath(HWND w_Handle, const char* title, const char* fFilter, int criteria)
{
	char c_buffer[MAX_PATH];
	std::string ct_buffer;

	try
	{
		OPENFILENAMEA ofn;

		memset(&ofn, 0, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = w_Handle;
		ofn.lpstrFilter = fFilter;
		ofn.nFilterIndex = 1;
		ofn.lpstrFile = c_buffer;
		ofn.lpstrFile[0] = '\0';
		ofn.lpstrTitle = title;
		ofn.nMaxFile = MAX_PATH;
		ofn.Flags = (OFN_EXPLORER | OFN_PATHMUSTEXIST);

		switch (criteria)
		{
		case CRITERIA_OPEN:
		{
			if (!GetOpenFileNameA(&ofn))
				return std::string("");
			break;
		}
		case CRITERIA_SAVE:
		{
			if (!GetSaveFileNameA(&ofn))
				return std::string("");
			break;
		}
		}
		ct_buffer = c_buffer;
		return ct_buffer;
	}
	catch (const char* e)
	{
		MessageBoxA(w_Handle, e, "Exception Thrown!", MB_OK | MB_ICONERROR);
		return std::string("");
	}
	catch (const std::bad_alloc& e)
	{
		MessageBoxA(w_Handle, e.what(), "Exception Thrown!", MB_OK | MB_ICONERROR);
		return std::string("");
	}
}

std::wstring SaveOpenFilePathW(HWND w_Handle, const wchar_t* title, const wchar_t* fFilter, int criteria)
{
	wchar_t c_buffer[MAX_PATH];
	std::wstring ct_buffer;

	try
	{
		OPENFILENAMEW ofn;

		memset(&ofn, 0, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = w_Handle;
		ofn.lpstrFilter = fFilter;
		ofn.nFilterIndex = 1;
		ofn.lpstrFile = c_buffer;
		ofn.lpstrFile[0] = '\0';
		ofn.lpstrTitle = title;
		ofn.nMaxFile = MAX_PATH;
		ofn.Flags = (OFN_EXPLORER | OFN_PATHMUSTEXIST);

		switch (criteria)
		{
		case CRITERIA_OPEN:
		{
			if (!GetOpenFileNameW(&ofn))
				return std::wstring(L"");
			break;
		}
		case CRITERIA_SAVE:
		{
			if (!GetSaveFileNameW(&ofn))
				return std::wstring(L"");
			break;
		}
		}
		ct_buffer = c_buffer;
		return ct_buffer;
	}
	catch (const char* e)
	{
		MessageBoxA(w_Handle, e, "Exception Thrown!", MB_OK | MB_ICONERROR);
		return std::wstring(L"");
	}
	catch (const std::bad_alloc& e)
	{
		MessageBoxA(w_Handle, e.what(), "Exception Thrown!", MB_OK | MB_ICONERROR);
		return std::wstring(L"");
	}
	return std::wstring();
}

SLProjectStruct* LoadProject(const char* path)
{
	static SLProjectStruct proj_temp = { };
	memset(&proj_temp, 0, sizeof(proj_temp));

	std::fstream file;
	file.open(path, std::ios::in | std::ios::out | std::ios::binary);
	if (file.is_open())
	{
		file.read(reinterpret_cast<char*>(&proj_temp), sizeof(SLProjectStruct));
		file.close();
	}
	else
	{
		MessageBoxA(0, "Cannot load project!", "Error!", MB_OK | MB_ICONERROR);
		return nullptr;
	}
	return &proj_temp;
}

bool SaveProject(const char* path, SLProjectStruct& projectStruct)
{
	std::fstream file;
	file.open(path, std::ios::in | std::ios::out | std::ios::binary);
	if (file.is_open())
	{
		file.write(reinterpret_cast<char*>(&projectStruct), sizeof(SLProjectStruct));
		file.close();
	}
	else
	{
		MessageBoxA(0, "Cannot save project!", "Error!", MB_OK | MB_ICONERROR);
		return false;
	}
	return true;
}

void InitializeSRTFileToListView(HWND w_lvHandle, const char* srtPath)
{
	// In Progress...
	// Needs to implement parsing of SubRip file.

	return;
}

bool AddTitle(HWND w_lvHandle, SubtitleLaboratory::SubtitleContainer cnt)
{
	if (!::IsTimerBeginEndValid(cnt.time_begin, cnt.time_end))
	{
		DLG_DISPLAY_ERROR(GetParent(w_lvHandle))
		return false;
	}

	unsigned int index = 0u;
	std::wostringstream woss_tbeg, woss_tend, woss_index;
	woss_index << cnt.number;

	// HOURS: If there is just single-digit number, attach 0 to its left side.
	AttachZerosIfSingleDigit(woss_tbeg, cnt.time_begin);
	AttachZerosIfSingleDigit(woss_tend, cnt.time_end);

	if (!LV_InsertItems(w_lvHandle, 0u, { woss_index.str().c_str(), woss_tbeg.str().c_str(), woss_tend.str().c_str(), cnt.lpstrText.c_str() }))
	{
		MessageBoxA(GetParent(w_lvHandle), "Cannot add title!", "Error!", MB_OK | MB_ICONERROR);
		PostQuitMessage(-1);
	}

	::subtitles_deque.push_back(cnt);
	return true;
}

bool SaveSRTToFile(HWND w_Handle)
{
	std::wstring path = ::SaveOpenFilePathW(w_Handle, L"Save Subtitle", L"SubRip File\0*.srt\0", CRITERIA_SAVE);
	if (path.length() < 1)
		return true;

	std::wfstream file(path.c_str(), std::ios::in | std::ios::out | std::ios::trunc);
	if (file.is_open())
	{
		for (auto& ttl : ::subtitles_deque)
		{
			if (!::IsTimerBeginEndValid(ttl.time_begin, ttl.time_end))
				return false;

			std::wostringstream woss;
			woss << ttl.number << std::endl;

			if (ttl.time_begin.HH <= 9)
				woss << '0' << ttl.time_begin.HH << ':' << ttl.time_begin.MM << ':' << ttl.time_begin.SS << ',' << ttl.time_begin.MS;
			else
				woss << ttl.time_begin.HH << ':' << ttl.time_begin.MM << ':' << ttl.time_begin.SS << ',' << ttl.time_begin.MS;

			woss << " --> ";

			if (ttl.time_end.HH <= 9)
				woss << '0' << ttl.time_end.HH << ':' << ttl.time_end.MM << ':' << ttl.time_end.SS << ',' << ttl.time_end.MS << std::endl;
			else
				woss << ttl.time_end.HH << ':' << ttl.time_end.MM << ':' << ttl.time_end.SS << ',' << ttl.time_end.MS << std::endl;

			woss << ttl.lpstrText << std::endl;
			woss << std::endl; // Should be avoided for the last subtitle in deque.
			file << woss.str().c_str();
		}
		file.close();
	}
	else
	{
		MessageBoxA(w_Handle, "is_open() failed!", "Error!", MB_OK | MB_ICONERROR);
		return false;
	}
	return true;
}

bool IsTimerValid(SubtitleLaboratory::SubRipTimer timer)
{
	if ((timer.MS > 999))
		return false;
	if ((timer.MM > 59))
		return false;
	if ((timer.SS > 59))
		return false;
	return true;
}

bool IsTimerBeginEndValid(SubtitleLaboratory::SubRipTimer begin, SubtitleLaboratory::SubRipTimer end)
{
	// Convert all timers to smallest unit to be able to compare their value
	double begin_time_ms = obj_Parser.ConvertToUnit<double, SubtitleLaboratory::millisecond>(begin);
	double end_time_ms = obj_Parser.ConvertToUnit<double, SubtitleLaboratory::millisecond>(end);

	// Check if values are valid per SubRipTimer
	if (!IsTimerValid(begin) || !IsTimerValid(end))
	{
		::Runtime_LastSubtitleError = L"Timers are invalid.";
		return false;
	}
	if (begin_time_ms > end_time_ms)
	{
		::Runtime_LastSubtitleError = L"Start time is greater than end time.";
		return false;
	}
	if (begin_time_ms == end_time_ms)
	{
		::Runtime_LastSubtitleError = L"Timers are equal.";
		return false;
	}

	// Those are unused because conditions above are better and simpler.
	// Check if HOUR values are valid for both SubRipTimer-s (dependent)
	if (begin.HH > end.HH)
		return false;

	// If begin and end timer hour are equal, but begin time has bigger minute count than end time.
	if ((begin.HH == end.HH) && (begin.MM > end.MM))
		return false;
	// If begin and end timer hour are equal, minutes too, but begin time has bigger seconds count than end time.
	if ((begin.HH == end.HH) && (begin.MM == end.MM) && (begin.SS > end.SS))
		return false;
	// If begin and end timer hour are equal, minutes and seconds too, but begin time has bigger milliseconds count than end time.
	if ((begin.HH == end.HH) && (begin.MM == end.MM) && (begin.SS == end.SS) && (begin.MS > end.MS))
		return false;
	// If timers begin and end in same time (possible, but useless and unneeded, clasified as error)
	if ((begin.HH == end.HH) && (begin.MM == end.MM) && (begin.SS == end.SS) && (begin.MS == end.MS))
		return false;
	return true;
}

void RefreshTitleList(HWND w_lvHandle)
{
	std::deque<SubtitleLaboratory::SubtitleContainer> titles_deque(::subtitles_deque);
	ListView_DeleteAllItems(w_lvHandle);
	for (auto& itl : titles_deque)
		AddTitle(w_lvHandle, itl);
	return;
}

void p_FixSizeForReviewList(std::size_t& for_index, int& cx)
{
	if (for_index == 0ull) cx = 100;
	else cx = 50;
}

void p_FixSizeMainList(std::size_t& for_index, int& cx)
{
	if (for_index == 0)
		cx = 50; // For #
	else if ((for_index == 1) || (for_index == 2))
		cx = 150; // For title begin and end
	else
		cx = 610; // For title text
}

void p_FixSizeForErrorList(std::size_t& for_index, int& cx)
{
	cx = 100;
	return;
}

void AttachZerosIfSingleDigit(std::wostringstream& woss, SubtitleLaboratory::SubRipTimer& ref_timer)
{
	// HOURS
	if (ref_timer.HH <= 9) woss << L'0' << ref_timer.HH;
	else woss << ref_timer.HH;

	woss << L':';

	// MINUTES
	if (ref_timer.MM <= 9) woss << L'0' << ref_timer.MM;
	else woss << ref_timer.MM;

	woss << L':';

	// SECONDS
	if (ref_timer.SS <= 9) woss << L'0' << ref_timer.SS;
	else woss << ref_timer.SS;

	woss << L',';

	// MILLISECONDS
	if (ref_timer.MS <= 9) woss << L"00" << ref_timer.MS;
	else if (ref_timer.MS <= 99 && ref_timer.MS > 9) woss << L'0' << ref_timer.MS;
	else if (ref_timer.MS > 9 || ref_timer.MS > 99) woss << ref_timer.MS;
	return;
}

double GetTitleDuration(int unit_id, SubtitleLaboratory::SubtitleContainer cnt)
{
	std::uint32_t Begin_HH = cnt.time_begin.HH, End_HH = cnt.time_begin.HH;
	std::uint32_t Begin_MM = cnt.time_begin.MM, End_MM = cnt.time_begin.MM;
	std::uint32_t Begin_SS = cnt.time_begin.SS, End_SS = cnt.time_begin.SS;
	std::uint32_t Begin_MS = cnt.time_begin.MS, End_MS = cnt.time_begin.MS;

	std::uint32_t hours_difference = 0u, minutes_difference = 0u, seconds_difference = 0u, milliseconds_difference = 0u;

	// We are not checking if timers are valid because parameter is member of subtitles_deque where all elements are checked and valid.

	// We will start from left to right.
	double total_amount = 0.0;
	std::uint32_t ms_seconds_additional_decrement = 0u; // in case that begin milliseconds are bigger than end.

	if (End_MS > 0u)
	{
		if (End_MS > Begin_MS)
			total_amount += (End_MS - Begin_MS);
	}
	else if (End_MS < Begin_MS)
	{
		total_amount += (End_MS - Begin_MS);
		ms_seconds_additional_decrement += 1u;
	}


	total_amount = ::ConvertTimeUnit(0, unit_id, total_amount);
	return total_amount;
}

double ConvertTimeUnit(int from_unit_id, int to_unit_id, unsigned long int amount)
{
	if (from_unit_id == ID_HH)
	{
		switch (to_unit_id)
		{
		case ID_MM:
			return (static_cast<double>(amount) * 60.0);
			break;
		case ID_SS:
			return (static_cast<double>(amount) * 3600.0);
			break;
		case ID_MS:
			return (static_cast<double>(amount) * 3600000.0);
		default:
			return -1.0;
		}
	}
	else if (from_unit_id == ID_MM)
	{
		switch (to_unit_id)
		{
		case ID_MM:
			return (static_cast<double>(amount) / 60.0);
			break;
		case ID_SS:
			return (static_cast<double>(amount) * 60.0);
			break;
		case ID_MS:
			return (static_cast<double>(amount) * 60000.0);
		default:
			return -1.0;
		}
	}
	else if (from_unit_id == ID_SS)
	{
		switch (to_unit_id)
		{
		case ID_HH:
			return (static_cast<double>(amount) / 3600.0);
			break;
		case ID_MM:
			return (static_cast<double>(amount) / 60.0);
			break;
		case ID_MS:
			return (static_cast<double>(amount) * 1000.0);
			break;
		default:
			return -1.0;
		}
	}
	else if (from_unit_id == ID_MS)
	{
		switch (to_unit_id)
		{
		case ID_HH:
			return (static_cast<double>(amount) / 3600000.0);
			break;
		case ID_MM:
			return (static_cast<double>(amount) / 60000.0);
			break;
		case ID_SS:
			return (static_cast<double>(amount) / 1000.0);
			break;
		default:
			return -1.0;
		}
	}
	else
		return -1.0;
	return 0.0;
}

HWND InitializeSubtitleReviewControl(HWND w_Handle)
{
	RECT lvRect;
	GetWindowRect(GetDlgItem(w_Handle, IDC_LV_EDITOR_LIST), &lvRect);

	RECT sRect;
	GetWindowRect(GetDlgItem(w_Handle, IDC_STATIC_ADD_CONTROL_PANEL), &sRect);

	HWND w_SubtitleReviewTemp = CreateWindow(
		WC_STATIC, nullptr,
		WS_VISIBLE | WS_CHILD | WS_BORDER,
		sRect.right + 1, lvRect.bottom + 1, lvRect.right, sRect.bottom,
		w_Handle, reinterpret_cast<HMENU>(IDC_STATIC_REVIEW_TITLE), GetModuleHandle(nullptr), nullptr
	);

	return w_SubtitleReviewTemp;
}

WDimension GetWindowDimensions(RECT& rRect)
{
	WDimension dimensions_obj;
	dimensions_obj.Width = rRect.right - rRect.left;
	dimensions_obj.Height = rRect.bottom - rRect.top;
	return dimensions_obj;
}

void UpdateReviewText(HWND w_rwHandle, const wchar_t* title_text)
{

}

void ShowHideControl(bool& bControlVar, int cID, HWND w_cHandle)
{
	if (!bControlVar)
	{
		bControlVar = true;
		CheckMenuItem(GetMenu(GetParent(w_cHandle)), cID, MF_CHECKED);
		ShowWindow(w_cHandle, SW_SHOWDEFAULT);
	}
	else
	{
		bControlVar = false;
		CheckMenuItem(GetMenu(GetParent(w_cHandle)), cID, MF_UNCHECKED);
		ShowWindow(w_cHandle, SW_HIDE);
	}
	return;
}

signed int LV_GetSelectedItemIndex(HWND w_lvHandle)
{
	int sel_item_index = SendMessage(w_lvHandle, LVM_GETNEXTITEM, (WPARAM)-1, (LPARAM)LVNI_SELECTED);
	if (sel_item_index < 0)
		return -1;
	return sel_item_index;
}

signed int LV_GetSelectedItemCount(HWND w_lvHandle)
{
	return ListView_GetSelectedCount(w_MainTitleList);
}

void DeleteSubtitleItem(HWND w_lvHandle, int index)
{
	int itr_index = 0;
	for (std::deque<SubtitleLaboratory::SubtitleContainer>::iterator itr = subtitles_deque.begin(); itr != subtitles_deque.end(); ++itr)
	{
		if (LV_GetSelectedItemCount(w_MainTitleList) < 1)
		{
			MessageBoxA(GetParent(w_lvHandle), "0 items selected.", "Delete Item", MB_OK | MB_ICONINFORMATION);
			break;
		}
		else
		{
			if (itr_index == LV_GetSelectedItemIndex(w_MainTitleList))
			{
				auto a = ::subtitles_deque.size();
				::subtitles_deque.erase(itr);
				auto b = ::subtitles_deque.size();
				ListView_DeleteItem(w_MainTitleList, itr_index);
				return;
			}
		}
		++itr_index;
	}
	return;
}

SubtitleLaboratory::SubRipTimer CalculateTime(int criteria, SubtitleLaboratory::SubRipTimer first_time, SubtitleLaboratory::SubRipTimer second_time)
{
	SubtitleLaboratory::SubRipTimer obj_timer = { };

	switch (criteria)
	{
	case TIME_PLUS:
	{
		unsigned int ss_add = (first_time.SS + second_time.SS);
		unsigned int mm_add = (first_time.MM + second_time.MM);
		unsigned int hh_add = (first_time.HH + second_time.HH);
		unsigned int ms_add = (first_time.MS + second_time.MS);

		if (ms_add > 1000u)
		{
			if ((ss_add % 1000u) == 0u)
			{
				ss_add += (ms_add / 1000u);
				ms_add = 0u;
			}
			else
			{
				int rest_of_milliseconds = (ms_add % 1000u);
				ss_add += (ms_add / 1000u);
				ms_add = rest_of_milliseconds;
			}
		}
		if (ss_add > 60u)
		{
			if ((ss_add % 60u) == 0u)
			{
				mm_add += (ss_add / 60u);
				ss_add = 0u;
			}
			else
			{
				int rest_of_seconds = (ss_add % 60u);
				mm_add += (ss_add / 60u);
				ss_add = rest_of_seconds;
			}
		}

		if (mm_add > 60u)
		{
			if ((mm_add % 60u) == 0u)
			{
				hh_add += (mm_add / 60u);
				mm_add = 0u;
			}
			else
			{
				int rest_of_minutes = (mm_add % 60u);
				hh_add += (mm_add / 60u);
				mm_add = rest_of_minutes;
			}
		}

		obj_timer.HH = hh_add; obj_timer.MM = mm_add; obj_timer.SS = ss_add; obj_timer.MS = ms_add;
		break;
	}
	case TIME_MINUS:
	{
		unsigned int ss_sub = (first_time.SS - second_time.SS);
		unsigned int mm_sub = (first_time.MM - second_time.MM);
		unsigned int hh_sub = (first_time.HH - second_time.HH);
		unsigned int ms_sub = (first_time.MS - second_time.MS);

		if (!::IsTimerBeginEndValid(first_time, second_time))
		{
			MessageBoxA(0, "IsTimerBeginEndValid() Failed!", "Subdivistion failed", MB_OK | MB_ICONERROR);
			break;
		}
		break;
	}
	default:
		break;
	}
	return obj_timer;
}

std::size_t FindCaseSensitive(std::wstring tstr, const wchar_t* wfind, std::size_t pos )
{
	std::wstring wsfind(wfind);
	std::transform(tstr.begin(), tstr.end(), tstr.begin(), ::tolower);
	std::transform(wsfind.begin(), wsfind.end(), wsfind.begin(), ::tolower);
	return tstr.find(wfind, pos);
}

void FindAllCaseSensitiveOccurrences(std::wstring tstr, const wchar_t* wfind, std::vector<std::size_t>& pos_vec)
{
	std::wstring wsfind(wfind);
	std::size_t pos = tstr.find(wsfind);
	while (pos != std::wstring::npos)
	{
		pos_vec.push_back(pos);
		pos = FindCaseSensitive(tstr, wfind, pos + wsfind.size());
	}
	return;
}