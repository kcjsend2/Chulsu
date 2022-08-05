#pragma once
#include "stdafx.h"
#include "Mesh.h"

class AssetManager
{
public:
	void LoadModel(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList,
		D3D12MA::Allocator* allocator, ResourceStateTracker tracker, const std::string& path);

private:
	unordered_map<string, vector<shared_ptr<Mesh>>> mCachedMeshes;
	//unordered_map<string, shared_ptr<Texture>> mCachedTextures;

	//EVERY Texture will stores here for THE TECHNIQUE OF THE GOD (I mean, Bindless Resources.)
	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap;
};