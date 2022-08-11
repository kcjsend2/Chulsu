#pragma once
#include "stdafx.h"
#include "Mesh.h"
#include "Texture.h"

struct AccelerationStructureBuffers
{
	ComPtr<D3D12MA::Allocation> mScratch;
	ComPtr<D3D12MA::Allocation> mResult;
	ComPtr<D3D12MA::Allocation> mInstanceDesc;    // Used only for top-level AS
};

class AssetManager
{
public:
	AssetManager(ID3D12Device* device, int numDescriptor);

	void LoadModel(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList,
		D3D12MA::Allocator* allocator, ResourceStateTracker& tracker, const std::string& path);

	void LoadTestTriangleModel(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList,
		D3D12MA::Allocator* allocator, ResourceStateTracker& tracker);

	D3D12_CPU_DESCRIPTOR_HANDLE GetIndexedCPUHandle(const UINT& index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetIndexedGPUHandle(const UINT& index);

	shared_ptr<Texture> LoadTexture(ID3D12Device5* device,
		ID3D12GraphicsCommandList4* cmdList,
		D3D12MA::Allocator* d3dAllocator,
		ResourceStateTracker& tracker,
		const std::wstring& filePath,
		const D3D12_RESOURCE_STATES& resourceStates,
		const D3D12_SRV_DIMENSION& dimension); 

	void BuildAcceleerationStructure();

	void BuildBLAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, D3D12MA::Allocator* d3dAllocator, ResourceStateTracker tracker, const vector<shared_ptr<Mesh>>& meshes);

	// We will move this function to scene class later.
	void BuildTLAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, D3D12MA::Allocator* d3dAllocator, ResourceStateTracker tracker, uint64_t& tlasSize);

	vector<ComPtr<D3D12MA::Allocation>>& GetBLAS() { return mBLAS; }
	AccelerationStructureBuffers GetTLAS() { return mTLAS; }

	UINT mCbvSrvUavDescriptorSize = 0;

private:
	unordered_map<string, vector<shared_ptr<Mesh>>> mMeshMap;
	vector<shared_ptr<Texture>> mTextures;

	vector<ComPtr<D3D12MA::Allocation>> mBLAS;
	AccelerationStructureBuffers mTLAS;

	//EVERY Texture will store here for Bindless Resources Technique.
	ComPtr<ID3D12DescriptorHeap> mDescriptorHeap;
	UINT mHeapCurrentIndex = 0;
};