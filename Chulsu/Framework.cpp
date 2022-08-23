#include "Framework.h"

LRESULT CALLBACK Framework::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Framework* pThis = nullptr;
    if (msg == WM_CREATE)
    {
        CREATESTRUCT* pcs = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<Framework*>(pcs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
    }
    else
    {
        pThis = reinterpret_cast<Framework*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (pThis)
        return pThis->OnProcessMessage(hwnd, msg, wParam, lParam);
    else
        return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT Framework::OnProcessMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (mRenderer)
        return mRenderer->OnProcessMessage(hwnd, msg, wParam, lParam);

    switch (msg)
    {
    case WM_KEYDOWN:
    case WM_KEYUP:
        if (wParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
            return 0;
        }
        break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
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
    mRenderer->BuildObjects();

    MsgLoop();

    DestroyWindow(mWinHandle);
}

void Framework::Init(const string& winTitle, uint32_t width, uint32_t height)
{
    const WCHAR* className = L"Main Window";

    // Register the window class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = className;
    wc.hIconSm = LoadIcon(wc.hInstance, IDI_APPLICATION);

    if (RegisterClassEx(&wc) == 0)
    {
        return;
    }

    // Window size we have is for client area, calculate actual window size
    RECT r{ 0, 0, (LONG)width, (LONG)height };

    int windowWidth = r.right - r.left;
    int windowHeight = r.bottom - r.top;

    // create the window
    wstring wTitle = stringTowstring(winTitle);
    mWinHandle = CreateWindow(className, wTitle.c_str(),
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        windowWidth, windowHeight, NULL, NULL,
        GetModuleHandle(NULL), this);

    if (mWinHandle == nullptr)
    {
        return;
    }
}

void Framework::MsgLoop()
{
    MSG msg{};

    mTimer.Reset();

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            mTimer.Tick();

            mRenderer->SetDeltaTime(mTimer.ElapsedTime());
            mRenderer->Update();
            mRenderer->Draw();
        }
    }
}
