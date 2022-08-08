#pragma once
#include "stdafx.h"

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

	void InitializeMeshBuffers(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList,
		ResourceStateTracker& tracker, UINT vbStride, UINT ibStride, D3D12_PRIMITIVE_TOPOLOGY topology, const void* vbData, UINT vbCount, const void* ibData, UINT ibCount);

private:
	int mAlbedoTextureIndex = NULL;
	int mMetalicTextureIndex = NULL;
	int mRoughnessTextureIndex = NULL;
	int mNormalMapTextureIndex = NULL;

	ComPtr<ID3D12Resource> mVertexBufferAlloc;
	ComPtr<ID3D12Resource> mIndexBufferAlloc;

	D3D12_VERTEX_BUFFER_VIEW mVertexBufferView = {};
	D3D12_INDEX_BUFFER_VIEW mIndexBufferView = {};

	D3D12_PRIMITIVE_TOPOLOGY mPrimitiveTopology = {};

	BoundingOrientedBox mOOBB = {};

	UINT mSlot = 0;
	UINT mVerticesCount = 0;
	UINT mVertexStride = 0;
	UINT mIndexCount = 0;

	ComPtr<ID3D12Resource> CreateBufferResource(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList,
		ResourceStateTracker& tracker, const void* initData, UINT64 byteSize, ID3D12Resource* uploadBuffer, D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT);
};