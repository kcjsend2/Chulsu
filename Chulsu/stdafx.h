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
#include "WICTextureLoader12.h"
#include "pix3.h"

using namespace std;
using namespace DirectX;
using namespace DirectX::PackedVector;

using Microsoft::WRL::ComPtr;

static dxc::DxcDllSupport gDxcDllSupport;

#define arraysize(a) (sizeof(a)/sizeof(a[0]))
#define align_to(_alignment, _val) (((_val + _alignment - 1) / _alignment) * _alignment)

#define MAX_TEXTURE_SUBRESOURCE_COUNT 3
#define PI 3.1415926535f

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

	inline XMFLOAT4X4 Inverse(const XMFLOAT4X4& mat)
	{
		XMFLOAT4X4 ret;
		XMStoreFloat4x4(&ret, XMMatrixInverse(nullptr, XMLoadFloat4x4(&mat)));
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

namespace Vector3
{
	inline float Distance(const XMFLOAT3& v1, const XMFLOAT3& v2)
	{
		return (float)sqrt(pow(v1.x - v2.x, 2) + pow(v1.y - v2.y, 2) + pow(v1.z - v2.z, 2));
	}

	inline XMFLOAT3 Zero()
	{
		return XMFLOAT3(0.0f, 0.0f, 0.0f);
	}

	inline XMFLOAT3 VectorToFloat3(FXMVECTOR& vector)
	{
		XMFLOAT3 ret;
		XMStoreFloat3(&ret, vector);
		return ret;
	}

	inline XMFLOAT3 Replicate(float value)
	{
		return VectorToFloat3(XMVectorReplicate(value));
	}

	inline XMFLOAT3 Multiply(float scalar, const XMFLOAT3& v)
	{
		XMFLOAT3 ret;
		XMStoreFloat3(&ret, scalar * XMLoadFloat3(&v));
		return ret;
	}

	inline XMFLOAT3 Multiply(const XMFLOAT3& v1, const XMFLOAT3& v2)
	{
		return XMFLOAT3(v1.x * v2.x, v1.y * v2.y, v1.z * v2.z);
	}

	inline XMFLOAT3 Divide(float scalar, XMFLOAT3& v)
	{
		XMFLOAT3 ret;
		XMStoreFloat3(&ret, XMLoadFloat3(&v) / scalar);
		return ret;
	}

	inline XMFLOAT3 MultiplyAdd(float delta, const XMFLOAT3& src, const XMFLOAT3& dst)
	{
		auto v = Replicate(delta);
		XMVECTOR v1 = XMLoadFloat3(&v);
		XMVECTOR v2 = XMLoadFloat3(&src);
		XMVECTOR v3 = XMLoadFloat3(&dst);
		return VectorToFloat3(XMVectorMultiplyAdd(v1, v2, v3));
	}

	inline XMFLOAT3 TransformNormal(XMFLOAT3& src, FXMMATRIX& mat)
	{
		return VectorToFloat3(XMVector3TransformNormal(XMLoadFloat3(&src), mat));
	}

	inline XMFLOAT3 Transform(XMFLOAT3& src, FXMMATRIX& mat)
	{
		return VectorToFloat3(XMVector3Transform(XMLoadFloat3(&src), mat));
	}

	inline XMFLOAT3 TransformCoord(XMFLOAT3& src, XMFLOAT4X4& mat)
	{
		return VectorToFloat3(XMVector3TransformCoord(XMLoadFloat3(&src), XMLoadFloat4x4(&mat)));
	}

	inline XMFLOAT3 Normalize(XMFLOAT3& v)
	{
		return VectorToFloat3(XMVector3Normalize(XMLoadFloat3(&v)));
	}

	inline XMFLOAT3 Normalize(XMFLOAT3&& v)
	{
		return VectorToFloat3(XMVector3Normalize(XMLoadFloat3(&v)));
	}

	inline XMFLOAT3 Subtract(const XMFLOAT3& v1, const XMFLOAT3& v2)
	{
		return VectorToFloat3(XMVectorSubtract(XMLoadFloat3(&v1), XMLoadFloat3(&v2)));
	}

	inline XMFLOAT3 ScalarProduct(const XMFLOAT3& v, float scalar)
	{
		return VectorToFloat3(XMLoadFloat3(&v) * scalar);
	}

	inline XMFLOAT3 Cross(const XMFLOAT3& v1, const XMFLOAT3& v2)
	{
		return VectorToFloat3(XMVector3Cross(XMLoadFloat3(&v1), XMLoadFloat3(&v2)));
	}

	inline float Length(const XMFLOAT3& v)
	{
		XMFLOAT3 ret = VectorToFloat3(XMVector3Length(XMLoadFloat3(&v)));
		return ret.x;
	}

	inline float Dot(const XMFLOAT3& v1, const XMFLOAT3& v2)
	{
		return XMVectorGetX(XMVector3Dot(XMLoadFloat3(&v1), XMLoadFloat3(&v2)));
	}

	inline XMFLOAT3 Add(const XMFLOAT3& v, float value)
	{
		return VectorToFloat3(XMVectorAdd(XMLoadFloat3(&v), XMVectorReplicate(value)));
	}

	inline XMFLOAT3 Add(const XMFLOAT3& v1, const XMFLOAT3& v2)
	{
		return VectorToFloat3(XMLoadFloat3(&v1) + XMLoadFloat3(&v2));
	}

	inline XMFLOAT3 Add(const XMFLOAT3& v1, const XMFLOAT3& v2, float distance)
	{
		return VectorToFloat3(XMLoadFloat3(&v1) + XMLoadFloat3(&v2) * distance);
	}

	inline XMFLOAT3 ClampFloat3(const XMFLOAT3& input, const XMFLOAT3& min, const XMFLOAT3& max)
	{
		XMFLOAT3 ret;
		ret.x = (min.x > input.x) ? min.x : ((max.x < input.x) ? max.x : input.x);
		ret.y = (min.y > input.y) ? min.y : ((max.y < input.y) ? max.y : input.y);
		ret.z = (min.z > input.z) ? min.z : ((max.z < input.z) ? max.z : input.z);
		return ret;
	}

	inline XMFLOAT3 Absf(XMFLOAT3& v)
	{
		return XMFLOAT3(std::abs(v.x), std::abs(v.y), std::abs(v.z));
	}

	inline bool Equal(XMFLOAT3& v1, XMFLOAT3& v2)
	{
		return XMVector3Equal(XMLoadFloat3(&v1), XMLoadFloat3(&v2));
	}

	inline bool Less(XMFLOAT3& v, float x)
	{
		return XMVector3Less(XMLoadFloat3(&v), XMVectorReplicate(x));
	}

	inline XMFLOAT3 Lerp(const XMFLOAT3& from, const XMFLOAT3& to, float t)
	{
		return VectorToFloat3(XMVectorLerp(XMLoadFloat3(&from), XMLoadFloat3(&to), t));
	}
}

namespace Vector4
{
	inline XMFLOAT4 Zero()
	{
		return XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	}

	inline bool Equal(XMFLOAT4& v1, XMFLOAT4& v2)
	{
		return XMVector4Equal(XMLoadFloat4(&v1), XMLoadFloat4(&v2));
	}

	inline XMFLOAT4 Add(XMFLOAT4& v1, XMFLOAT4& v2)
	{
		XMFLOAT4 ret;
		XMStoreFloat4(&ret, XMVectorAdd(XMLoadFloat4(&v1), XMLoadFloat4(&v2)));
		return ret;
	}

	inline XMFLOAT4 Multiply(float scalar, XMFLOAT4& v)
	{
		XMFLOAT4 ret;
		XMStoreFloat4(&ret, scalar * XMLoadFloat4(&v));
		return ret;
	}

	inline XMFLOAT4 RotateQuaternionAxis(const XMFLOAT3& axis, float angle)
	{
		XMFLOAT4 ret;
		XMStoreFloat4(&ret, XMQuaternionRotationAxis(XMLoadFloat3(&axis), angle));
		return ret;
	}

	inline XMFLOAT4 RotateQuaternionRollPitchYaw(const XMFLOAT3& rotation)
	{
		XMFLOAT4 ret;
		XMStoreFloat4(&ret, XMQuaternionRotationRollPitchYaw(rotation.x, rotation.y, rotation.z));
		return ret;
	}

	inline XMFLOAT4 Slerp(const XMFLOAT4& from, const XMFLOAT4& to, float t)
	{
		XMFLOAT4 ret;
		XMStoreFloat4(&ret, XMQuaternionSlerp(XMLoadFloat4(&from), XMLoadFloat4(&to), t));
		return ret;
	}
}
