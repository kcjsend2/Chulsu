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

	void SetBLAS(const AccelerationStructureBuffers& BLAS) { mBLAS = BLAS; }
	AccelerationStructureBuffers GetBLAS() { return mBLAS; }

	void SetVertexCount(UINT vertexCount) { mVerticesCount = vertexCount; }
	void SetIndexCount(UINT indexCount) { mIndexCount = indexCount; }

	UINT GetVertexCount() { return mVerticesCount; }
	UINT GetIndexCount() { return mIndexCount; }

	void SetVertexOffset(UINT vertexOffset) { mVertexOffset = vertexOffset; }
	void SetIndexOffset(UINT indexOffset) { mIndexOffset = indexOffset; }

	UINT GetVertexOffset() { return mVertexOffset; }
	UINT GetIndexOffset() { return mIndexOffset; }

	void SetMaterialIndex(UINT materialIndex) { mMaterialIndex = materialIndex; }
	UINT GetMaterialIndex() { return mMaterialIndex; }

	string GetName() { return mName; }
	void SetName(string name) { mName = name; }

private:
	AccelerationStructureBuffers mBLAS;

	D3D12_PRIMITIVE_TOPOLOGY mPrimitiveTopology = {};

	BoundingOrientedBox mOOBB = {};

	UINT mVerticesCount = 0;
	UINT mIndexCount = 0;

	string mName = {};

	UINT mVertexOffset = 0;
	UINT mIndexOffset = 0;

	UINT mMaterialIndex = UINT_MAX;
};