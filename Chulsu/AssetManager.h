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

	shared_ptr<Texture> LoadTexture(ID3D12Device5* device,
		ID3D12GraphicsCommandList4* cmdList,
		D3D12MA::Allocator* d3dAllocator,
		ResourceStateTracker& tracker,
		const std::wstring& filePath,
		const D3D12_RESOURCE_STATES& resourceStates,
		const D3D12_SRV_DIMENSION& dimension);

	UINT mCbvSrvUavDescriptorSize = 0;

private:
	unordered_map<string, vector<shared_ptr<Mesh>>> mMeshes;
	vector<shared_ptr<Texture>> mTextures;

	//EVERY Texture will store here for Bindless Resources Technique.
	ComPtr<ID3D12DescriptorHeap> mDescriptorHeap;
	UINT mHeapCurrentIndex = 0;
};