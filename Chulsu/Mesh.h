#pragma once
#include "stdafx.h"
#include "SubMesh.h"

class Mesh
{
public:
	Mesh() = default;
	Mesh(vector<SubMesh>& subMeshes) { mSubMeshes = subMeshes; }

	void SetBLAS(AccelerationStructureBuffers& blas) { mBLAS = blas; }
	AccelerationStructureBuffers& GetBLAS() { return mBLAS; }

	void SetSubMesh(SubMesh subMesh) { mSubMeshes.push_back(subMesh); }
	vector<SubMesh>& GetSubMeshes() { return mSubMeshes; }
	void SetVertexAttribIndex(UINT attribIndex) { mVertexAttribIndex = attribIndex; }
	void SetIndexBufferIndex(UINT attribIndex) { mIndexBufferIndex = attribIndex; }

	UINT GetVertexAttribIndex() { return mVertexAttribIndex; }
	UINT GetIndexBufferIndex() { return mIndexBufferIndex; }

	ComPtr<D3D12MA::Allocation> GetVertexBufferAlloc() { return mVertexBufferAlloc; }
	ComPtr<D3D12MA::Allocation> GetIndexBufferAlloc() { return mIndexBufferAlloc; }

	const UINT GetSubMeshCount() { return mSubMeshes.size(); }

	void InitializeBuffers(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList,
		ComPtr<D3D12MA::Allocator> alloc, ResourceStateTracker& tracker, AssetManager& assetMgr,
		UINT vbStride, UINT ibStride, D3D12_PRIMITIVE_TOPOLOGY topology, const void* vbData, UINT vbCount,
		const void* ibData, UINT ibCount);

	const D3D12_SHADER_RESOURCE_VIEW_DESC VertexShaderResourceView();

	const D3D12_SHADER_RESOURCE_VIEW_DESC IndexShaderResourceView();


private:
	vector<SubMesh> mSubMeshes;
	AccelerationStructureBuffers mBLAS;

	UINT mVertexAttribIndex = UINT_MAX;
	UINT mIndexBufferIndex = UINT_MAX;

	ComPtr<D3D12MA::Allocation> mVertexBufferAlloc;
	ComPtr<D3D12MA::Allocation> mIndexBufferAlloc;

	D3D12_PRIMITIVE_TOPOLOGY mPrimitiveTopology = {};

	UINT mSlot = 0;
	UINT mVerticesCount = 0;
	UINT mVertexStride = 0;
	UINT mIndexCount = 0;
};