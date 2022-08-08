#pragma once

#define NOMINMAX
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include <crtdbg.h>
#include <dxgidebug.h>

#include <Windows.h>
#include <windowsx.h>
#include <sdkddkver.h>
#include <wrl.h>
#include <comdef.h>

#include <d3d12.h>
#include <dxgi1_4.h>

#include <DirectXCollision.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <DirectXPackedVector.h>

#include "d3dx12.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

#include <array>
#include <vector>
#include <stack>
#include <set>
#include <map>
#include <unordered_map>
#include <string>
#include <sstream>
#include <memory>
#include <fstream>
#include <iostream>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <tchar.h>
#include <conio.h>
#include <io.h>
#include <codecvt>
#include <filesystem>
#include <span>

#include "dxcapi.h" 
#include <d3dcompiler.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "ResourceStateTracker.h"
#include "DDSTextureLoader12.h"
#include "D3D12MemAlloc.h"

using namespace std;
using namespace DirectX;
using namespace DirectX::PackedVector;

using Microsoft::WRL::ComPtr;

extern int gFrameWidth;
extern int gFrameHeight;

#define arraysize(a) (sizeof(a)/sizeof(a[0]))
#define align_to(_alignment, _val) (((_val + _alignment - 1) / _alignment) * _alignment)

inline UINT GetConstantBufferSize(UINT bytes)
{
	return ((bytes + 255) & ~255);
}

inline wstring AnsiToWString(const string& str)
{
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return wstring(buffer);
}

inline wstring stringTowstring(const string& s)
{
	wstring_convert<codecvt_utf8<WCHAR>> cvt;
	wstring ws = cvt.from_bytes(s);
	return ws;
}

inline string wstringTostring(const wstring& ws)
{
	wstring_convert<codecvt_utf8<wchar_t>> cvt;
	string s = cvt.to_bytes(ws);
	return s;
}

inline void ReportLiveObjects()
{
	Microsoft::WRL::ComPtr<IDXGIDebug1> dxgi_debug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgi_debug.GetAddressOf()))))
	{
		dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
	}
}

#ifndef ThrowIfFailed
#define ThrowIfFailed(f)												\
{																		\
	HRESULT hr_ = (f);													\
	wstring wfn = AnsiToWString(__FILE__);							\
	if (FAILED(hr_))	{ throw DxException(hr_, L#f, wfn, __LINE__); } \
}																		
#endif

class DxException
{
public:
	DxException() = default;

	DxException(HRESULT hr, const wstring& functionName, const wstring& fileName, int lineNumber)
		: mError(hr), mFuncName(functionName), mFileName(fileName), mLineNumber(lineNumber)
	{
	}

	wstring ToString() const
	{
		_com_error error(mError);
		wstring msg = error.ErrorMessage();
		return mFuncName + L" failed in " + mFileName + L";  line "
			+ to_wstring(mLineNumber) + L";  Error : " + msg;
	}

	HRESULT mError = S_OK;
	wstring mFuncName;
	wstring mFileName;
	int mLineNumber = -1;
};
