#include "stdafx.h"
#include "Framework.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int mCmdShow)
{
#if defined(DEBUG) || defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	try
	{
		Framework app;
		app.Init("Chulsu Renderer");
		app.Run();
	}
	catch (std::exception& ex)
	{
		MessageBoxA(nullptr, ex.what(), "ERROR", MB_OK);
		return -1;
	}

	return 0;
}