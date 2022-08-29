#include "Mesh.h"

void Mesh::InitializeBuffers(
	ID3D12Device5* device,
	ID3D12GraphicsCommandList4* cmdList,
	ComPtr<D3D12MA::Allocator> alloc,
	ResourceStateTracker& tracker,
	AssetManager& assetMgr,
	UINT vbStride, UINT ibStride,
	D3D12_PRIMITIVE_TOPOLOGY topology,
	const void* vbData, UINT vbCount,
	const void* ibData, UINT ibCount)
{
	mPrimitiveTopology = topology;

	const UINT vbByteSize = vbCount * vbStride;

	mVertexBufferAlloc =
		assetMgr.CreateResource(device, cmdList, alloc, tracker, vbData, vbByteSize, 1,
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_DIMENSION_BUFFER, DXGI_FORMAT_UNKNOWN, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE);

	mVerticesCount = vbCount;
	mVertexStride = vbStride;

	if (ibCount > 0)
	{
		mIndexCount = ibCount;
		mSlot = 0;

		const UINT ibByteSize = ibCount * ibStride;

		mIndexBufferAlloc = assetMgr.CreateResource(device, cmdList, alloc, tracker, ibData, ibByteSize, 1,
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_DIMENSION_BUFFER, DXGI_FORMAT_UNKNOWN, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE);
	}
}

const D3D12_SHADER_RESOURCE_VIEW_DESC Mesh::VertexShaderResourceView()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;

	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = mVerticesCount;
	srvDesc.Buffer.StructureByteStride = sizeof(Vertex);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	return srvDesc;
}

const D3D12_SHADER_RESOURCE_VIEW_DESC Mesh::IndexShaderResourceView()
{
	if (mIndexCount == 0)
		return D3D12_SHADER_RESOURCE_VIEW_DESC();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;

	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = mIndexCount;
	srvDesc.Buffer.StructureByteStride = sizeof(UINT);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	return srvDesc;
}