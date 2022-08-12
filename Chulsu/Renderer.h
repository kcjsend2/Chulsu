#pragma once
#include "stdafx.h"

class Renderer
{
public:
	Renderer() {}
	virtual ~Renderer() {}

	virtual void Init(HWND winHandle, uint32_t winWidth, uint32_t winHeight) {}
	virtual void Draw() {}
	virtual void BuildObjects() {}

protected:
	HWND mWinHandle = NULL;
	XMFLOAT2 mSwapChainSize = {0, 0};
};