#pragma once
#pragma once

#include "stdafx.h"
#include "AssetManager.h"

template<typename Cnst>
class ConstantBuffer
{
public:
	ConstantBuffer(ID3D12Device5* device,
		ID3D12GraphicsCommandList4* cmdList,
		UINT count,
		ComPtr<D3D12MA::Allocator> alloc,
		ResourceStateTracker& tracker,
		AssetManager& assetMgr)
	{
		// 바이트 크기는 항상 256의 배수가 되어야 한다.
		mByteSize = (sizeof(Cnst) + 255) & ~255;

		mUploadAlloc = assetMgr.CreateResource(device, cmdList, alloc, tracker, NULL, mByteSize * count, 1, D3D12_RESOURCE_STATE_GENERIC_READ,
			D3D12_RESOURCE_DIMENSION_BUFFER, DXGI_FORMAT_UNKNOWN, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE, D3D12_HEAP_TYPE_UPLOAD);

		ThrowIfFailed(mUploadAlloc->GetResource()->Map(0, nullptr, (void**)(&mData)));
	}
	ConstantBuffer(const ConstantBuffer& rhs) = delete;
	ConstantBuffer& operator=(const ConstantBuffer& rhs) = delete;
	virtual ~ConstantBuffer()
	{
		if (mUploadAlloc)
			mUploadAlloc->GetResource()->Unmap(0, nullptr);
		mData = nullptr;
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress(int idx) const
	{
		return mUploadAlloc->GetResource()->GetGPUVirtualAddress() + idx * mByteSize;
	}

	void CopyData(int index, const Cnst& data)
	{
		memcpy(&mData[index * mByteSize], &data, sizeof(Cnst));
	}

	UINT GetByteSize() const
	{
		return mByteSize;
	}

private:
	ComPtr<D3D12MA::Allocation> mUploadAlloc = nullptr;
	BYTE* mData = nullptr;
	UINT mByteSize = 0;
};