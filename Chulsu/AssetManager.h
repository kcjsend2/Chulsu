#pragma once
#include "stdafx.h"
#include "Texture.h"

class Mesh;
struct Vertex;

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


	ComPtr<D3D12MA::Allocation> CreateBufferResource(ID3D12Device5* device,
		ID3D12GraphicsCommandList4* cmdList,
		ComPtr<D3D12MA::Allocator> allocator,
		ResourceStateTracker& tracker, const void* initData, UINT64 width, UINT64 height,
		D3D12_RESOURCE_STATES initialState, D3D12_RESOURCE_FLAGS flag,D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT);

	void LoadModel(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList,
		D3D12MA::Allocator* allocator, ResourceStateTracker& tracker, const std::string& path);

	void LoadTestTriangleModel(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList,
		ComPtr<D3D12MA::Allocator> allocator, ResourceStateTracker& tracker);

	D3D12_CPU_DESCRIPTOR_HANDLE GetIndexedCPUHandle(const UINT& index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetIndexedGPUHandle(const UINT& index);

	shared_ptr<Texture> LoadTexture(ID3D12Device5* device,
		ID3D12GraphicsCommandList4* cmdList,
		D3D12MA::Allocator* d3dAllocator,
		ResourceStateTracker& tracker,
		const std::wstring& filePath,
		const D3D12_RESOURCE_STATES& resourceStates,
		const D3D12_SRV_DIMENSION& srvDimension, const D3D12_UAV_DIMENSION& uavDimension,
		bool isSRV, bool isUAV);

	void SetTexture(ID3D12Device5* device,
		ID3D12GraphicsCommandList4* cmdList,
		ComPtr<D3D12MA::Allocation> alloc,
		const wstring& textureName,
		const D3D12_SRV_DIMENSION& srvDimension, const D3D12_UAV_DIMENSION& uavDimension,
		bool isSRV, bool isUAV);

	void BuildAccelerationStructure(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, ComPtr<D3D12MA::Allocator> d3dAllocator, ResourceStateTracker tracker);

	void BuildBLAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, ComPtr<D3D12MA::Allocator> d3dAllocator, ResourceStateTracker tracker, const vector<shared_ptr<Mesh>>& meshes);

	// We will move this function to scene class later.
	void BuildTLAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, ComPtr<D3D12MA::Allocator> d3dAllocator, ResourceStateTracker tracker, UINT& tlasSize);

	void PushUploadBuffer(ComPtr<D3D12MA::Allocation> alloc) { mUploadBuffers.push_back(alloc); }
	void FreeUploadBuffers() { mUploadBuffers.clear(); }

	vector<AccelerationStructureBuffers>& GetBLAS() { return mBLAS; }
	AccelerationStructureBuffers GetTLAS() { return mTLAS; }

	UINT mCbvSrvUavDescriptorSize = 0;

private:
	unordered_map<string, vector<shared_ptr<Mesh>>> mMeshMap;
	unordered_map<wstring, shared_ptr<Texture>> mTextures;

	vector<AccelerationStructureBuffers> mBLAS;
	AccelerationStructureBuffers mTLAS;
	UINT mTLASSize = 0;

	vector<ComPtr<D3D12MA::Allocation>> mUploadBuffers;

	//EVERY Texture will store here for Bindless Resources Technique.
	ComPtr<ID3D12DescriptorHeap> mSRVUAVDescriptorHeap;
	UINT mHeapCurrentIndex = 0;
};