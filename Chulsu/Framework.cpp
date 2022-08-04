#include "Framework.h"
#include "DX12Renderer.h"

LRESULT CALLBACK Framework::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

void Framework::Run()
{
    RECT r;
    GetClientRect(mWinHandle, &r);
    mWidth = r.right - r.left;
    mHeight = r.bottom - r.top;

    std::atexit(ReportLiveObjects);
    ShowWindow(mWinHandle, SW_SHOWNORMAL);

    mRenderer = make_shared<DX12Renderer>();
    mRenderer->Init(mWinHandle, mWidth, mHeight);

    MsgLoop();

    DestroyWindow(mWinHandle);
}

void Framework::Init(const string& winTitle, uint32_t width, uint32_t height)
{
    const WCHAR* className = L"Main Window";
    DWORD winStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

    // Register the window class
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = className;

    if (RegisterClass(&wc) == 0)
    {
        return;
    }

    // Window size we have is for client area, calculate actual window size
    RECT r{ 0, 0, (LONG)width, (LONG)height };
    AdjustWindowRect(&r, winStyle, false);

    int windowWidth = r.right - r.left;
    int windowHeight = r.bottom - r.top;

    // create the window
    wstring wTitle = stringTowstring(winTitle);
    mWinHandle = CreateWindowEx(0, className, wTitle.c_str(), winStyle, CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight, nullptr, nullptr, wc.hInstance, nullptr);
    if (mWinHandle == nullptr)
    {
        return;
    }
}

void Framework::MsgLoop()
{
    MSG msg{};

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            mRenderer->Draw();
        }
    }
}
