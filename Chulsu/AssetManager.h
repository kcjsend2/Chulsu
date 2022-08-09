#pragma once
#include "stdafx.h"
#include "Mesh.h"
#include "Texture.h"

class AssetManager
{
public:
	AssetManager() { };

	void LoadModel(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList,
		D3D12MA::Allocator* allocator, ResourceStateTracker tracker, const std::string& path);

	UINT mCbvSrvUavDescriptorSize;

private:
	unordered_map<string, vector<shared_ptr<Mesh>>> mCachedMeshes;
	unordered_map<string, shared_ptr<Texture>> mCachedTextures;

	//EVERY Texture will store here for THE TECHNIQUE OF THE GOD (I mean, Bindless Resources.)
	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap;
};