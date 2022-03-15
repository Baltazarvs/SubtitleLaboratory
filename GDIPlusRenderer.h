// 2020 - 2022 Baltazarus

#pragma once
#ifndef RENDERER_H
#define RENDERER_H
#include "Utilities.h"
#include <gdiplus.h>
#pragma comment(lib, "Gdiplus.lib")

class GDIPlusRenderer
{
private:
	Gdiplus::GdiplusStartupInput gp_i;
	ULONG_PTR gp_ulong;

public:
	GDIPlusRenderer()
	{
		Gdiplus::GdiplusStartup(&this->gp_ulong, &gp_i, nullptr);
	}

	~GDIPlusRenderer()
	{
		Gdiplus::GdiplusShutdown(this->gp_ulong);
	}
};

#endif