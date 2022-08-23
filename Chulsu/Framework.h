#pragma once
#include "Timer.h"
#include "DX12Renderer.h"

class Framework
{
public:
    void Run();
    void Init(const std::string& winTitle, uint32_t width = 1920, uint32_t height = 1080);

private:
    HWND mWinHandle = nullptr;
    uint32_t mWidth = NULL;
    uint32_t mHeight = NULL;

    shared_ptr<Renderer> mRenderer;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    LRESULT OnProcessMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void MsgLoop();

    Timer mTimer;
};