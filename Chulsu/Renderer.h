#pragma once
#include "stdafx.h"

class Renderer
{
public:
	Renderer() {}
	virtual ~Renderer() {}

	virtual void Init(HWND winHandle, uint32_t winWidth, uint32_t winHeight) {}
	virtual void BuildObjects() {}

	virtual LRESULT OnProcessMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) = 0;

	virtual void Update() = 0;
	virtual void Draw() = 0;

protected:
	virtual void OnResize() = 0;

	virtual void OnProcessMouseDown(WPARAM buttonState, int x, int y) = 0;
	virtual void OnProcessMouseUp(WPARAM buttonState, int x, int y) = 0;
	virtual void OnProcessMouseMove(WPARAM buttonState, int x, int y) = 0;
	virtual void OnProcessKeyInput(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;

protected:
	HWND mWinHandle = NULL;
	XMFLOAT2 mSwapChainSize = {0, 0};
};