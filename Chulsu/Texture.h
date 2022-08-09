#pragma once

class Texture
{
public:
	Texture() = default;
	virtual ~Texture() { }

	void LoadTextureFromDDS(
		ID3D12Device5* device,
		ID3D12GraphicsCommandList4* cmdList,
		D3D12MA::Allocator* allocator,
		ResourceStateTracker& tracker,
		const std::wstring& filePath,
		D3D12_RESOURCE_STATES resourceStates = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	void CreateTexture(
		ID3D12Device5* device,
		UINT width, UINT height, UINT elements, UINT miplevels,
		DXGI_FORMAT format, D3D12_RESOURCE_FLAGS resourceFlags,
		D3D12_RESOURCE_STATES resourceStates, D3D12_CLEAR_VALUE* clearValue);

	void SetDimension(D3D12_SRV_DIMENSION dimension) { mViewDimension = dimension; }

public:
	D3D12_SHADER_RESOURCE_VIEW_DESC ShaderResourceView() const;
	ID3D12Resource* GetResource() const { return mTextureBufferAlloc->GetResource(); }

protected:
	ComPtr<D3D12MA::Allocation> mTextureBufferAlloc;

	D3D12_SRV_DIMENSION mViewDimension{};

	DXGI_FORMAT mBufferFormats{};
	UINT mBufferElementsCount = 0;
};