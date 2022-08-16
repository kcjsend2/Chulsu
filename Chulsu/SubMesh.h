#pragma once
#include "stdafx.h"
#include "AssetManager.h"

struct Vertex
{
public:
	XMFLOAT3 position = { 0, 0, 0 };
	XMFLOAT3 normal = { 0, 0, 0 };
	XMFLOAT2 texCoord = { 0, 0 };
	XMFLOAT3 tangent = { 0, 0, 0 };
	XMFLOAT3 biTangent = { 0, 0, 0 };
};

class SubMesh	
{
public:
	SubMesh() {}
	virtual ~SubMesh() {}

	void InitializeBuffers(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList,
		ComPtr<D3D12MA::Allocator> allocator, ResourceStateTracker tracker, AssetManager& assetMgr, UINT vbStride, UINT ibStride,
		D3D12_PRIMITIVE_TOPOLOGY topology, const void* vbData, UINT vbCount, const void* ibData, UINT ibCount);

	void SetBLAS(const AccelerationStructureBuffers& BLAS) { mBLAS = BLAS; }
	AccelerationStructureBuffers GetBLAS() { return mBLAS; }

	UINT GetVertexCount() { return mVerticesCount; }
	UINT GetIndexCount() { return mIndexCount; }

	ComPtr<D3D12MA::Allocation> GetVertexBufferAlloc () { return mVertexBufferAlloc; }
	ComPtr<D3D12MA::Allocation> GetIndexBufferAlloc() { return mIndexBufferAlloc; }

private:
	AccelerationStructureBuffers mBLAS;

	ComPtr<D3D12MA::Allocation> mVertexBufferAlloc;
	ComPtr<D3D12MA::Allocation> mIndexBufferAlloc;

	D3D12_VERTEX_BUFFER_VIEW mVertexBufferView = {};
	D3D12_INDEX_BUFFER_VIEW mIndexBufferView = {};

	D3D12_PRIMITIVE_TOPOLOGY mPrimitiveTopology = {};

	BoundingOrientedBox mOOBB = {};

	UINT mSlot = 0;
	UINT mVerticesCount = 0;
	UINT mVertexStride = 0;
	UINT mIndexCount = 0;
};