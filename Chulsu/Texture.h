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

	void SetDescriptorHeapInfo(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle, UINT DescriptorHeapIndex);

	void SetDimension(D3D12_SRV_DIMENSION dimension) { mViewDimension = dimension; }
	string SetName(string name) { mName = name; }
	string GetName() { return mName; }

public:
	D3D12_SHADER_RESOURCE_VIEW_DESC ShaderResourceView() const;
	ID3D12Resource* GetResource() const { return mTextureBufferAlloc->GetResource(); }

protected:
	ComPtr<D3D12MA::Allocation> mTextureBufferAlloc;

	D3D12_SRV_DIMENSION mViewDimension{};

	DXGI_FORMAT mBufferFormats	{};
	UINT mBufferElementsCount = 0;

	D3D12_CPU_DESCRIPTOR_HANDLE mCPUHandle = {};
	D3D12_GPU_DESCRIPTOR_HANDLE mGPUHandle = {};

	UINT mDescriptorHeapIndex = 0;

	string mName = {};
};