#include "Mesh.h"

void Mesh::InitializeMeshBuffers(
	ID3D12Device5* device,
	ID3D12GraphicsCommandList4* cmdList,
	ComPtr<D3D12MA::Allocator> d3dAllocator,
	ResourceStateTracker tracker,
	AssetManager* assetMgr,
	UINT vbStride, UINT ibStride,
	D3D12_PRIMITIVE_TOPOLOGY topology,
	const void* vbData, UINT vbCount,
	const void* ibData, UINT ibCount)
{
	mPrimitiveTopology = topology;

	const UINT vbByteSize = vbCount * vbStride;

	mVertexBufferAlloc =
		assetMgr->CreateBufferResource(device, cmdList, d3dAllocator, tracker, vbData, vbByteSize, 1,
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_DIMENSION_BUFFER, DXGI_FORMAT_UNKNOWN, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE);

	mVertexBufferView.BufferLocation = mVertexBufferAlloc->GetResource()->GetGPUVirtualAddress();
	mVertexBufferView.SizeInBytes = vbByteSize;
	mVertexBufferView.StrideInBytes = vbStride;

	mVerticesCount = vbCount;
	mVertexStride = vbStride;

	if (ibCount > 0)
	{
		mIndexCount = ibCount;
		mSlot = 0;

		const UINT ibByteSize = ibCount * ibStride;

		mIndexBufferAlloc = assetMgr->CreateBufferResource(device, cmdList, d3dAllocator, tracker, ibData, ibByteSize, 1,
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_DIMENSION_BUFFER, DXGI_FORMAT_UNKNOWN, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE);

		mIndexBufferView.BufferLocation = mIndexBufferAlloc->GetResource()->GetGPUVirtualAddress();
		mIndexBufferView.SizeInBytes = ibByteSize;
		mIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	}
}