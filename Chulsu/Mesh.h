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
	Mesh();
	virtual ~Mesh() {}

private:
	int mAlbedoTextureIndex = NULL;
	int mMetalicTextureIndex = NULL;
	int mRoughnessTextureIndex = NULL;

	ComPtr<ID3D12Resource> mVertexBuffer;
	ComPtr<ID3D12Resource> mIndexBuffer;

	ComPtr<ID3D12Resource> mVertexUploadBuffer;
	ComPtr<ID3D12Resource> mIndexUploadBuffer;

	D3D12_VERTEX_BUFFER_VIEW mVertexBufferView = {};
	D3D12_INDEX_BUFFER_VIEW mIndexBufferView = {};

	BoundingOrientedBox mOOBB = {};

	UINT mSlot = 0;
	UINT mVerticesCount = 0;
	UINT mVertexStride = 0;
	UINT mIndexCount = 0;
};