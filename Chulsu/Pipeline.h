#pragma once
#include "stdafx.h"

class AssetManager;

class Pipeline
{
public:
	void CreatePipelineState(ComPtr<ID3D12Device5> device, const WCHAR* filename);
	void CreateShaderTable(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList,
		ComPtr<D3D12MA::Allocator> alloc, ResourceStateTracker& tracker, AssetManager& assetMgr);

	ComPtr<D3D12MA::Allocation> GetShaderTable() { return mShaderTable; }
	ComPtr<ID3D12RootSignature> GetEmptyRootSignature() { return mEmptyRootSig; }
	ComPtr<ID3D12StateObject> GetStateObject() { return mPipelineState; }

	UINT GetShaderTableEntrySize() { return mShaderTableEntrySize; }

protected:
	UINT mShaderTableEntrySize = NULL;

	ComPtr<D3D12MA::Allocation> mShaderTable = NULL;

	ComPtr<ID3D12StateObject> mPipelineState = NULL;
	ComPtr<ID3D12RootSignature> mEmptyRootSig = NULL;
};