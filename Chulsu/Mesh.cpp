#include "Mesh.h"

void Mesh::InitializeMeshBuffers(
	ID3D12Device5* device,
	ID3D12GraphicsCommandList4* cmdList,
	D3D12MA::Allocator* d3dAllocator,
	ResourceStateTracker tracker,
	UINT vbStride, UINT ibStride,
	D3D12_PRIMITIVE_TOPOLOGY topology,
	const void* vbData, UINT vbCount,
	const void* ibData, UINT ibCount)
{
	mPrimitiveTopology = topology;

	const UINT vbByteSize = vbCount * vbStride;

	mVertexBufferAlloc =
		CreateBufferResource(device, cmdList, d3dAllocator, tracker, vbData, vbByteSize, 
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_FLAG_NONE);

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

		mIndexBufferAlloc = CreateBufferResource(device, cmdList, d3dAllocator, tracker, ibData, ibByteSize,
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_FLAG_NONE);

		mIndexBufferView.BufferLocation = mIndexBufferAlloc->GetResource()->GetGPUVirtualAddress();
		mIndexBufferView.SizeInBytes = ibByteSize;
		mIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	}
}