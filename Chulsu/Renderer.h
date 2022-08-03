#pragma once
#include "stdafx.h"
#include "AssetManager.h"

class Renderer
{
public:
	Renderer() {}
	virtual ~Renderer() {}

	virtual void Init(HWND winHandle, uint32_t winWidth, uint32_t winHeight) {}
	virtual void Draw() {}

protected:
	HWND mWinHandle = NULL;
	XMFLOAT2 mSwapChainSize = {0, 0};
	AssetManager mAssetMgr;
};