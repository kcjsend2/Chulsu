#include "stdafx.h"
#include "texture.h"


void Texture::LoadTextureFromDDS(
	ID3D12Device5* device,
	ID3D12GraphicsCommandList4* cmdList,
	D3D12MA::Allocator* d3dAllocator,
	ResourceStateTracker& tracker,
	const std::wstring& filePath,
	D3D12_RESOURCE_STATES resourceStates)
{
	std::unique_ptr<uint8_t[]> ddsData;
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	DDS_ALPHA_MODE alphaMode = DDS_ALPHA_MODE_UNKNOWN;
	bool isCubeMap = false;
	mName = wstringTostring(filePath);

	ComPtr<D3D12MA::Allocation> textureAlloc;
	LoadDDSTextureFromFileEx(
		device, filePath.c_str(), 0,
		D3D12_RESOURCE_FLAG_NONE, DDS_LOADER_DEFAULT,
		textureAlloc.Get(), d3dAllocator, ddsData, subresources, &alphaMode, &isCubeMap);

	const UINT subresourcesCount = (UINT)subresources.size();
	UINT64 bytes = GetRequiredIntermediateSize(mTextureBufferAlloc->GetResource(), 0, subresourcesCount);

	ComPtr<D3D12MA::Allocation> uploadAlloc;
	D3D12MA::ALLOCATION_DESC allocationDesc;
	allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

	auto resourceDesc = CD3DX12_RESOURCE_DESC(D3D12_RESOURCE_DIMENSION_BUFFER,
		D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, bytes, 1, 1,
		1, DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE);

	d3dAllocator->CreateResource(
		&allocationDesc,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		NULL,
		&uploadAlloc,
		IID_NULL, NULL);

	UpdateSubresources(cmdList, mTextureBufferAlloc->GetResource(), uploadAlloc->GetResource(),
		0, 0, subresourcesCount, subresources.data());

	tracker.AddTrackingResource(mTextureBufferAlloc->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST);
	tracker.TransitionBarrier(cmdList, mTextureBufferAlloc->GetResource(), resourceStates);
}

void Texture::SetDescriptorHeapInfo(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle, UINT DescriptorHeapIndex)
{
	mCPUHandle = cpuHandle;
	mGPUHandle = gpuHandle;
	mDescriptorHeapIndex = DescriptorHeapIndex;
}

D3D12_SHADER_RESOURCE_VIEW_DESC Texture::ShaderResourceView() const
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mTextureBufferAlloc->GetResource()->GetDesc().Format;
	srvDesc.ViewDimension = mViewDimension;

	switch (mViewDimension)
	{
	case D3D12_SRV_DIMENSION_TEXTURE2D:
		srvDesc.Texture2D.MipLevels = -1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		break;

	case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
		srvDesc.Texture2DArray.MipLevels = -1;
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.PlaneSlice = 0;
		srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
		srvDesc.Texture2DArray.FirstArraySlice = 0;
		srvDesc.Texture2DArray.ArraySize = mTextureBufferAlloc->GetResource()->GetDesc().DepthOrArraySize;
		break;

	case D3D12_SRV_DIMENSION_TEXTURECUBE:
		srvDesc.TextureCube.MipLevels = -1;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		break;

	case D3D12_SRV_DIMENSION_BUFFER:
		srvDesc.Format = mBufferFormats;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = mBufferElementsCount;
		srvDesc.Buffer.StructureByteStride = 0;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		break;
	}
	return srvDesc;
}
