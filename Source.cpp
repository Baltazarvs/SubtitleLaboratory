// 2020 - 2022 Baltazarus
#include "Application.h"

int __stdcall WinMain(HINSTANCE _In_ hInstance, HINSTANCE _In_ hPrevInstance, LPSTR _In_ lpCmdLine, int _In_ nCmdShow)
{
	static wchar_t open_with_buff[255];
	std::vector<std::wstring> ArgsVec;
	std::wstring open_with_path;

	int argc = 0;
	wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	for (int i = 0; i < argc; ++i)
		ArgsVec.push_back(argv[i]);
	HeapFree(GetProcessHeap(), NULL, argv);
	
	if (argc > 1)
	{
		wcscpy(open_with_buff, ArgsVec[1].c_str());
		open_with_path = open_with_buff;
	}

	// Main Application unit.
	Application App(nullptr, "Subtitle Laboratory 1.0b", { 100, 100, 500, 500 }, open_with_path);
	App.RunMessageLoop();
	return 0;
}
