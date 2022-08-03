#pragma once
#include "stdafx.h"
#include "ResourceStateTracker.h"

class Vertex
{
public:
	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT2 texCoord;
	XMFLOAT3 tangent;
	XMFLOAT3 biTangent;
};

class Mesh	
{
public:
	Mesh() {}
	virtual ~Mesh() {}

	void CreateResourceInfo(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList,
		D3D12MA::Allocator* allocator, ResourceStateTracker tracker, UINT vbStride, UINT ibStride, D3D12_PRIMITIVE_TOPOLOGY topology, const void* vbData, UINT vbCount, const void* ibData, UINT ibCount);

private:
	int mAlbedoTextureIndex = NULL;
	int mMetalicTextureIndex = NULL;
	int mRoughnessTextureIndex = NULL;
	int mNormalMapTextureIndex = NULL;

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

	ComPtr<D3D12MA::Allocation> CreateBufferResource(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList,
		D3D12MA::Allocator* allocator, ResourceStateTracker& tracker, const void* initData, UINT64 byteSize, D3D12MA::Allocation* uploadBuffer, D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT);
};