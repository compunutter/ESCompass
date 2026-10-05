#include "Plugin.hpp"

#define NOMINMAX
#include <objidl.h>
#include <gdiplus.h>

static ULONG_PTR g_GdiplusToken = 0;
static CESCompassPlugin* g_Plugin = NULL;

extern "C" __declspec(dllexport) void EuroScopePlugInInit(EuroScopePlugIn::CPlugIn** ppPlugInInstance)
{
	if (g_GdiplusToken == 0)
	{
		Gdiplus::GdiplusStartupInput input;
		if (Gdiplus::GdiplusStartup(&g_GdiplusToken, &input, NULL) != Gdiplus::Ok)
			g_GdiplusToken = 0;
	}

	g_Plugin = new CESCompassPlugin();
	*ppPlugInInstance = g_Plugin;
}

extern "C" __declspec(dllexport) void EuroScopePlugInExit(void)
{
	delete g_Plugin;
	g_Plugin = NULL;

	if (g_GdiplusToken != 0)
	{
		Gdiplus::GdiplusShutdown(g_GdiplusToken);
		g_GdiplusToken = 0;
	}
}
