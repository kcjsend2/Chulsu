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
#include <DXProgrammableCapture.h>

#include "dxc/dxcapi.use.h"
#include <d3dcompiler.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "DirectXTex.h"
#include "D3D12MemAlloc.h"
#include "DDSTextureLoader12.h"
#include "pix3.h"

using namespace std;
using namespace DirectX;
using namespace DirectX::PackedVector;

using Microsoft::WRL::ComPtr;

static dxc::DxcDllSupport gDxcDllSupport;

extern int gFrameWidth;
extern int gFrameHeight;

#define arraysize(a) (sizeof(a)/sizeof(a[0]))
#define align_to(_alignment, _val) (((_val + _alignment - 1) / _alignment) * _alignment)

#define MAX_TEXTURE_SUBRESOURCE_COUNT 3


class ResourceStateTracker
{
public:
	ResourceStateTracker() {}
	~ResourceStateTracker()
	{
		mResourceStateMap.clear();
	}

	void TransitionBarrier(ComPtr<ID3D12GraphicsCommandList4> CmdList, ID3D12Resource* resource, D3D12_RESOURCE_STATES state)
	{
		if (mResourceStateMap[resource] == state)
			return;

		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, mResourceStateMap[resource], state, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
		mResourceStateMap[resource] = state;
		CmdList->ResourceBarrier(1, &barrier);
	}

	void AddTrackingResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES state) { mResourceStateMap[resource] = state; }
	D3D12_RESOURCE_STATES GetResourceState(ID3D12Resource* resource) { return mResourceStateMap[resource]; }

private:
	unordered_map<ID3D12Resource*, D3D12_RESOURCE_STATES> mResourceStateMap;
};


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

template<class BlotType>
inline std::string ConvertBlobToString(BlotType* pBlob)
{
	std::vector<char> infoLog(pBlob->GetBufferSize() + 1);
	memcpy(infoLog.data(), pBlob->GetBufferPointer(), pBlob->GetBufferSize());
	infoLog[pBlob->GetBufferSize()] = 0;
	return std::string(infoLog.data());
}


inline void ReportLiveObjects()
{
	Microsoft::WRL::ComPtr<IDXGIDebug1> dxgi_debug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgi_debug.GetAddressOf()))))
	{
		dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
	}
}

inline D3D12_DESCRIPTOR_HEAP_DESC DescriptorHeapDesc(
	UINT numDescriptors,
	D3D12_DESCRIPTOR_HEAP_TYPE descriptorType,
	D3D12_DESCRIPTOR_HEAP_FLAGS descriptorFlags,
	UINT NodeMase)
{
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	descriptorHeapDesc.Type = descriptorType;
	descriptorHeapDesc.Flags = descriptorFlags;
	descriptorHeapDesc.NodeMask = 0;
	return descriptorHeapDesc;
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


namespace Matrix4x4
{
	inline XMFLOAT4X4 Identity4x4()
	{
		XMFLOAT4X4 ret;
		XMStoreFloat4x4(&ret, XMMatrixIdentity());
		return ret;
	}

	inline XMFLOAT4X4 Transpose(const XMFLOAT4X4& mat)
	{
		XMFLOAT4X4 ret;
		XMStoreFloat4x4(&ret, XMMatrixTranspose(XMLoadFloat4x4(&mat)));
		return ret;
	}

	inline XMFLOAT4X4 Multiply(const XMFLOAT4X4& mat1, const XMFLOAT4X4& mat2)
	{
		XMFLOAT4X4 ret;
		XMStoreFloat4x4(&ret, XMMatrixMultiply(XMLoadFloat4x4(&mat1), XMLoadFloat4x4(&mat2)));
		return ret;
	}

	inline XMFLOAT4X4 Multiply(const FXMMATRIX& mat1, const XMFLOAT4X4& mat2)
	{
		XMFLOAT4X4 ret;
		XMStoreFloat4x4(&ret, mat1 * XMLoadFloat4x4(&mat2));
		return ret;
	}

	inline XMFLOAT4X4 Reflect(XMFLOAT4& plane)
	{
		XMFLOAT4X4 ret;
		XMStoreFloat4x4(&ret, XMMatrixReflect(XMLoadFloat4(&plane)));
		return ret;
	}

	inline XMFLOAT4X4 CalulateWorldTransform(const XMFLOAT3& position, const XMFLOAT4& quaternion, const XMFLOAT3& scale)
	{
		XMMATRIX translation = XMMatrixTranslationFromVector(XMLoadFloat3(&position));
		XMMATRIX rotation = XMMatrixRotationQuaternion(XMLoadFloat4(&quaternion));
		XMMATRIX scaling = XMMatrixScalingFromVector(XMLoadFloat3(&scale));

		XMFLOAT4X4 world{};
		XMStoreFloat4x4(&world, scaling * rotation * translation);
		return world;
	}

	inline XMFLOAT4X4 CalulateWorldTransform(const XMFLOAT3& position, const XMFLOAT3& rotate, const XMFLOAT3& scale)
	{
		XMMATRIX translation = XMMatrixTranslationFromVector(XMLoadFloat3(&position));
		XMMATRIX rotation = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&rotate));
		XMMATRIX scaling = XMMatrixScalingFromVector(XMLoadFloat3(&scale));

		XMFLOAT4X4 world{};
		XMStoreFloat4x4(&world, scaling * rotation * translation);
		return world;
	}
}