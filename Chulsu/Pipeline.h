#pragma once
#include "stdafx.h"
#include "SubObject.h"

class Pipeline
{
public:
	void CreatePipelineState(ComPtr<ID3D12Device5> device, const WCHAR* filename);

protected:
	vector<D3D12_STATE_SUBOBJECT> mSubObjects;
	ComPtr<ID3D12StateObject> mPipelineState;
	ComPtr<ID3D12RootSignature> mEmptyRootSig;
};