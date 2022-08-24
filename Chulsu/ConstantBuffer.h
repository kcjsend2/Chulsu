#pragma once
#pragma once

#include "stdafx.h"
#include "AssetManager.h"

template<typename Cnst>
class UploadBuffer
{
public:
	UploadBuffer(ID3D12Device5* device,
		ID3D12GraphicsCommandList4* cmdList,
		UINT count,
		ComPtr<D3D12MA::Allocator> alloc,
		ResourceStateTracker& tracker,
		AssetManager& assetMgr, bool isConstant)
	{
		// 바이트 크기는 항상 256의 배수가 되어야 한다.
		if (isConstant)
			mByteSize = (sizeof(Cnst) + 255) & ~255;
		else
			mByteSize = sizeof(Cnst);

		mUploadAlloc = assetMgr.CreateResource(device, cmdList, alloc, tracker, NULL, mByteSize * count, 1, D3D12_RESOURCE_STATE_GENERIC_READ,
			D3D12_RESOURCE_DIMENSION_BUFFER, DXGI_FORMAT_UNKNOWN, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE, D3D12_HEAP_TYPE_UPLOAD);

		ThrowIfFailed(mUploadAlloc->GetResource()->Map(0, nullptr, (void**)(&mData)));
	}
	UploadBuffer(const UploadBuffer& rhs) = delete;
	UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
	virtual ~UploadBuffer()
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

	ComPtr<D3D12MA::Allocation> GetUploadAllocation() { return mUploadAlloc; }

private:
	ComPtr<D3D12MA::Allocation> mUploadAlloc = nullptr;
	BYTE* mData = nullptr;
	UINT mByteSize = 0;
};