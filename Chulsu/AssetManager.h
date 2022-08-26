#pragma once
#include "stdafx.h"

class Texture;
class Instance;
class SubMesh;
class Mesh;
struct Vertex;

struct TextureHeapIndex
{
	bool init = false;

	UINT AlbedoTextureIndex = UINT_MAX;
	UINT MetalicTextureIndex = UINT_MAX;
	UINT RoughnessTextureIndex = UINT_MAX;
	UINT NormalMapTextureIndex = UINT_MAX;
	UINT OpacityMapTextureIndex = UINT_MAX;
};

struct AccelerationStructureBuffers
{
	ComPtr<D3D12MA::Allocation> mScratch = NULL;
	ComPtr<D3D12MA::Allocation> mResult = NULL;
	ComPtr<D3D12MA::Allocation> mInstanceDesc = NULL;    // Used only for top-level AS
};

enum FLAG_TEXTURE_LOAD
{
	FLAG_WIC,
	FLAG_DDS
};

class AssetManager
{
public:
	AssetManager();

	void Init(ID3D12Device* device, int numDescriptor);

	ComPtr<D3D12MA::Allocation> CreateResource(ID3D12Device5* device,
		ID3D12GraphicsCommandList4* cmdList,
		ComPtr<D3D12MA::Allocator> alloc,
		ResourceStateTracker& tracker, const void* initData, UINT64 width, UINT64 height,
		D3D12_RESOURCE_STATES initialState, D3D12_RESOURCE_DIMENSION dimension,
		DXGI_FORMAT format, D3D12_TEXTURE_LAYOUT layout, D3D12_RESOURCE_FLAGS flag,D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT);

	void LoadAssimpScene(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, D3D12MA::Allocator* alloc, ResourceStateTracker& tracker, const std::string& path);

	void CreateInstance(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList,
		D3D12MA::Allocator* alloc, ResourceStateTracker& tracker, const std::string& path,
		XMFLOAT3 position, XMFLOAT3 rotation, XMFLOAT3 scale);

	void LoadTestInstance(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList,
		ComPtr<D3D12MA::Allocator> alloc, ResourceStateTracker& tracker);

	D3D12_CPU_DESCRIPTOR_HANDLE GetIndexedCPUHandle(const UINT& index);
	D3D12_GPU_DESCRIPTOR_HANDLE GetIndexedGPUHandle(const UINT& index);

	void LoadTexture(ID3D12Device5* device,
		ID3D12GraphicsCommandList4* cmdList,
		D3D12MA::Allocator* alloc,
		ResourceStateTracker& tracker,
		const std::wstring& filePath,
		const D3D12_RESOURCE_STATES& resourceStates,
		const D3D12_SRV_DIMENSION& srvDimension, const D3D12_UAV_DIMENSION& uavDimension,
		bool isSRV, bool isUAV, FLAG_TEXTURE_LOAD flag);

	void SetTexture(ID3D12Device5* device,
		ID3D12GraphicsCommandList4* cmdList,
		ComPtr<D3D12MA::Allocation> alloc,
		const wstring& textureName,
		const D3D12_SRV_DIMENSION& srvDimension, const D3D12_UAV_DIMENSION& uavDimension,
		bool isSRV, bool isUAV);

	// for Structured Buffer
	UINT SetShaderResource(ID3D12Device5* device,
		ID3D12GraphicsCommandList4* cmdList,
		ComPtr<D3D12MA::Allocation> alloc,
		const D3D12_SHADER_RESOURCE_VIEW_DESC& desc);

	void SetConstantBuffer(ID3D12Device5* device, const D3D12_CONSTANT_BUFFER_VIEW_DESC desc);

	void BuildAccelerationStructure(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, ComPtr<D3D12MA::Allocator> alloc, ResourceStateTracker& tracker);

	void BuildBLAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, ComPtr<D3D12MA::Allocator> alloc, ResourceStateTracker& tracker);

	// We will move this function to scene class later.
	void BuildTLAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, ComPtr<D3D12MA::Allocator> alloc, ResourceStateTracker& tracker, UINT& tlasSize);

	void PushUploadBuffer(ComPtr<D3D12MA::Allocation> alloc) { mUploadBuffers.push_back(alloc); }
	void FreeUploadBuffers() { mUploadBuffers.clear(); }

	const vector<shared_ptr<Instance>>& GetInstances() { return mInstances; }

	AccelerationStructureBuffers GetTLAS() { return mTLAS; }

	ComPtr<ID3D12DescriptorHeap> GetDescriptorHeap() { return mDescriptorHeap; }

	UINT GetCurrentHeapIndex() { return mHeapCurrentIndex; }

	void AddCurrentHeapIndex() { mHeapCurrentIndex++; }

	const TextureHeapIndex& GetMaterialIndices(UINT key) { return mTextureIndices[key]; }

	UINT mCbvSrvUavDescriptorSize = 0;

private:
	map<string, shared_ptr<Mesh>> mMeshMap;
	vector<shared_ptr<Instance>> mInstances;
	unordered_map<wstring, shared_ptr<Texture>> mTextures;

	AccelerationStructureBuffers mTLAS;
	UINT mTLASSize = 0;

	vector<ComPtr<D3D12MA::Allocation>> mUploadBuffers;

	//EVERY SRV/UAV/CBV will store here for Bindless Resources Technique.
	ComPtr<ID3D12DescriptorHeap> mDescriptorHeap;
	UINT mHeapCurrentIndex = 0;

	unordered_map<UINT, TextureHeapIndex> mTextureIndices;

	ComPtr<IDStorageQueue> mTextureQueue;
	ComPtr<IDStorageFactory> mTextureFactory;
};