#pragma once

class Texture
{
public:
	Texture() = default;
	virtual ~Texture() { }

	void SetResource(ComPtr<D3D12MA::Allocation> alloc) { mTextureBufferAlloc = alloc; };

	void LoadTextureFromDDS(
		ID3D12Device5* device,
		ID3D12GraphicsCommandList4* cmdList,
		D3D12MA::Allocator* allocator,
		ResourceStateTracker& tracker,
		const std::wstring& filePath,
		D3D12_RESOURCE_STATES resourceStates = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	void SetSRVDescriptorHeapInfo(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle, UINT DescriptorHeapIndex);
	void SetUAVDescriptorHeapInfo(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle, UINT DescriptorHeapIndex);

	void SetSRVDimension(D3D12_SRV_DIMENSION dimension) { mSRVDimension = dimension; }
	void SetUAVDimension(D3D12_UAV_DIMENSION dimension) { mUAVDimension = dimension; }
	string SetName(string name) { mName = name; }
	string GetName() { return mName; }

public:
	D3D12_SHADER_RESOURCE_VIEW_DESC ShaderResourceView() const;
	ID3D12Resource* GetResource() const { return mTextureBufferAlloc->GetResource(); }

protected:
	ComPtr<D3D12MA::Allocation> mTextureBufferAlloc;

	D3D12_SRV_DIMENSION mSRVDimension{};
	D3D12_UAV_DIMENSION mUAVDimension{};

	DXGI_FORMAT mBufferFormats	{};
	UINT mBufferElementsCount = 0;

	D3D12_CPU_DESCRIPTOR_HANDLE mSRVCPUHandle = {};
	D3D12_GPU_DESCRIPTOR_HANDLE mSRVGPUHandle = {};
	UINT mSRVDescriptorHeapIndex = 0;

	D3D12_CPU_DESCRIPTOR_HANDLE mUAVCPUHandle = {};
	D3D12_GPU_DESCRIPTOR_HANDLE mUAVGPUHandle = {};
	UINT mUAVDescriptorHeapIndex = 0;

	string mName = {};
};