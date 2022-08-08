#include "Mesh.h"

ComPtr<D3D12MA::Allocation> Mesh::CreateBufferResource(
	ID3D12Device5* device,
	ID3D12GraphicsCommandList4* cmdList,
	D3D12MA::Allocator* allocator,
	ResourceStateTracker& tracker,
	const void* initData, UINT64 byteSize,
	D3D12MA::Allocation* uploadBuffer,
	D3D12_HEAP_TYPE heapType)
{
	D3D12MA::ALLOCATION_DESC allocationDesc = {};
	allocationDesc.HeapType = heapType;

	auto resourceDesc = CD3DX12_RESOURCE_DESC(D3D12_RESOURCE_DIMENSION_BUFFER, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
		byteSize, 1, 1, 1, DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE);

	D3D12MA::Allocation* defaultAllocation;
	allocator->CreateResource(
		&allocationDesc,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		NULL,
		&defaultAllocation,
		IID_NULL, NULL);

	allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
	allocator->CreateResource(
		&allocationDesc,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		NULL,
		&uploadBuffer,
		IID_NULL, NULL);

	tracker.AddTrackingResource(defaultAllocation->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST);

	D3D12_SUBRESOURCE_DATA subresourceData = {};
	subresourceData.pData = initData;
	subresourceData.RowPitch = byteSize;
	subresourceData.SlicePitch = byteSize;

	UpdateSubresources(cmdList, defaultAllocation->GetResource(),
		uploadBuffer->GetResource(), 0, 0, 1, &subresourceData);

	tracker.TransitionBarrier(cmdList, defaultAllocation->GetResource(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	ComPtr<D3D12MA::Allocation> raiiAllocation(defaultAllocation);
	return raiiAllocation;
}

void Mesh::InitializeMeshBuffers(
	ID3D12Device5* device,
	ID3D12GraphicsCommandList4* cmdList,
	D3D12MA::Allocator* allocator,
	ResourceStateTracker& tracker,
	UINT vbStride, UINT ibStride,
	D3D12_PRIMITIVE_TOPOLOGY topology,
	const void* vbData, UINT vbCount,
	const void* ibData, UINT ibCount)
{
	mPrimitiveTopology = topology;

	const UINT vbByteSize = vbCount * vbStride;

	D3D12MA::Allocation* vertexUploadAlloc = nullptr;
	mVertexBufferAlloc = CreateBufferResource(device, cmdList, allocator, tracker,
		vbData, vbByteSize, vertexUploadAlloc);

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

		D3D12MA::Allocation* indexUploadAlloc = nullptr;
		mIndexBufferAlloc = CreateBufferResource(device, cmdList, allocator, tracker,
			ibData, ibByteSize, indexUploadAlloc);

		mIndexBufferView.BufferLocation = mIndexBufferAlloc->GetResource()->GetGPUVirtualAddress();
		mIndexBufferView.SizeInBytes = ibByteSize;
		mIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	}
}