#pragma once
#include "stdafx.h"

class Renderer
{
public:
	virtual void Init(HWND winHandle, uint32_t winWidth, uint32_t winHeight) {};
	virtual void Release() {};
	virtual void Draw() {};

protected:
	HWND mWinHandle;
	XMFLOAT2 mSwapChainSize;
};