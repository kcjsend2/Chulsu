#pragma once
#include "stdafx.h"

class Renderer
{
public:
	Renderer() {}
	~Renderer() {}

	virtual void Init(HWND winHandle, uint32_t winWidth, uint32_t winHeight) {}
	virtual void Draw() {}

protected:
	HWND mWinHandle = NULL;
	XMFLOAT2 mSwapChainSize = {0, 0};
};