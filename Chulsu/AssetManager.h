#pragma once
#include "stdafx.h"
#include "Mesh.h"
#include "Texture.h"

class AssetManager
{
public:
	AssetManager(ID3D12Device* device, int numDescriptor);

	void LoadModel(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList,
		D3D12MA::Allocator* allocator, ResourceStateTracker tracker, const std::string& path);

	// NOT YET IMPLEMETNED!!!!!!!
	void LoadTexture();

	UINT mCbvSrvUavDescriptorSize = 0;

private:
	unordered_map<string, vector<shared_ptr<Mesh>>> mMeshes;
	vector<shared_ptr<Texture>> mTextures;

	//EVERY Texture will store here for THE TECHNIQUE OF THE GOD (I mean, Bindless Resources.)
	ComPtr<ID3D12DescriptorHeap> mDescriptorHeap;
};