#pragma once
#include "stdafx.h"
#include "Mesh.h"
#include "Texture.h"

class AssetManager
{
public:
	AssetManager(ID3D12Device* device, int numDescriptor);

	void LoadModel(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList,
		D3D12MA::Allocator* allocator, ResourceStateTracker& tracker, const std::string& path);

	D3D12_CPU_DESCRIPTOR_HANDLE GetIndexedCPUHandle(const UINT& index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetIndexedGPUHandle(const UINT& index);

	void LoadTexture(ID3D12Device5* device,
		ID3D12GraphicsCommandList4* cmdList,
		D3D12MA::Allocator* d3dAllocator,
		ResourceStateTracker& tracker,
		const std::wstring& filePath,
		D3D12_RESOURCE_STATES resourceStates);

	UINT mCbvSrvUavDescriptorSize = 0;

private:
	unordered_map<string, vector<shared_ptr<Mesh>>> mMeshes;
	vector<shared_ptr<Texture>> mTextures;

	//EVERY Texture will store here for THE TECHNIQUE OF THE GOD (I mean, Bindless Resources.)
	ComPtr<ID3D12DescriptorHeap> mDescriptorHeap;
	UINT mHeapCurrentIndex = 0;
};