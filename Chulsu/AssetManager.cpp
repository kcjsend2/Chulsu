#include "AssetManager.h"
#include "Mesh.h"

AssetManager::AssetManager(ID3D12Device* device, int numDescriptor)
{
	auto descHeapDescriptor = DescriptorHeapDesc(
		numDescriptor,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		0);

	ThrowIfFailed(device->CreateDescriptorHeap(
		&descHeapDescriptor,
		IID_PPV_ARGS(&mSRVUAVDescriptorHeap)));
}

ComPtr<D3D12MA::Allocation> AssetManager::CreateBufferResource(
	ID3D12Device5* device,
	ID3D12GraphicsCommandList4* cmdList,
	ComPtr<D3D12MA::Allocator> allocator,
	ResourceStateTracker& tracker,
	const void* initData, UINT64 width, UINT64 height,
	D3D12_RESOURCE_STATES initialState,
	D3D12_RESOURCE_DIMENSION dimension,
	DXGI_FORMAT format,
	D3D12_TEXTURE_LAYOUT layout,
	D3D12_RESOURCE_FLAGS flag,
	D3D12_HEAP_TYPE heapType)
{
	D3D12MA::ALLOCATION_DESC allocationDesc = {};
	allocationDesc.HeapType = heapType;

	auto resourceDesc = CD3DX12_RESOURCE_DESC(dimension, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
		width, 1, 1, 1, format, 1, 0, layout, flag);

	auto resourceState = initData != NULL ? D3D12_RESOURCE_STATE_COPY_DEST : initialState;
	ComPtr<D3D12MA::Allocation> defaultAllocation;
	allocator->CreateResource(
		&allocationDesc,
		&resourceDesc,
		resourceState,
		NULL,
		&defaultAllocation,
		IID_NULL, NULL);


	tracker.AddTrackingResource(defaultAllocation->GetResource(), resourceState);

	if (initData != NULL)
	{
		ComPtr<D3D12MA::Allocation> UploadAlloc = nullptr;

		allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
		allocator->CreateResource(
			&allocationDesc,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			NULL,
			&UploadAlloc,
			IID_NULL, NULL);

		//UploadAlloc->GetResource()->SetName(L"Upload Buffer");

		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = initData;
		subresourceData.RowPitch = width;
		subresourceData.SlicePitch = width;

		UpdateSubresources(cmdList, defaultAllocation->GetResource(),
			UploadAlloc->GetResource(), 0, 0, 1, &subresourceData);

		tracker.TransitionBarrier(cmdList, defaultAllocation->GetResource(), initialState);
		PushUploadBuffer(UploadAlloc);
	}

	return defaultAllocation;
}

void AssetManager::LoadModel(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList,
	D3D12MA::Allocator* allocator, ResourceStateTracker& tracker, const std::string& path)
{
	std::vector<shared_ptr<Mesh>> meshes;

	Assimp::Importer Importer;
	constexpr uint32_t ImporterFlags =
		aiProcess_ConvertToLeftHanded |
		aiProcess_JoinIdenticalVertices |
		aiProcess_Triangulate |
		aiProcess_SortByPType |
		aiProcess_GenNormals |
		aiProcess_GenUVCoords |
		aiProcess_OptimizeMeshes |
		aiProcess_ValidateDataStructure |
		aiProcess_CalcTangentSpace;

	const aiScene* pAiScene = Importer.ReadFile(path.data(), ImporterFlags);

	if (!pAiScene || !pAiScene->HasMeshes())
	{
		__debugbreak();
	}

	meshes.reserve(pAiScene->mNumMeshes);
	for (unsigned m = 0; m < pAiScene->mNumMeshes; ++m)
	{
		// Assimp object
		const aiMesh* pAiMesh = pAiScene->mMeshes[m];

		std::vector<Vertex> Vertices;
		Vertices.reserve(pAiMesh->mNumVertices);
		for (unsigned int v = 0; v < pAiMesh->mNumVertices; ++v)
		{
			Vertex& vertex = Vertices.emplace_back();
			vertex.position = { pAiMesh->mVertices[v].x, pAiMesh->mVertices[v].y, pAiMesh->mVertices[v].z };

			if (pAiMesh->HasTextureCoords(0))
			{
				vertex.texCoord = { pAiMesh->mTextureCoords[0][v].x, pAiMesh->mTextureCoords[0][v].y };
			}

			if (pAiMesh->HasNormals())
			{
				vertex.normal = { pAiMesh->mNormals[v].x, pAiMesh->mNormals[v].y, pAiMesh->mNormals[v].z };
			}

			if (pAiMesh->HasTangentsAndBitangents())
			{
				vertex.biTangent = { pAiMesh->mBitangents[v].x, pAiMesh->mBitangents[v].y, pAiMesh->mBitangents[v].z };
				vertex.tangent = { pAiMesh->mTangents[v].x, pAiMesh->mTangents[v].y, pAiMesh->mTangents[v].z };
			}
		}

		std::vector<uint32_t> Indices;
		Indices.reserve(static_cast<size_t>(pAiMesh->mNumFaces) * 3);
		std::span Faces = { pAiMesh->mFaces, pAiMesh->mNumFaces };
		for (const auto& Face : Faces)
		{
			Indices.push_back(Face.mIndices[0]);
			Indices.push_back(Face.mIndices[1]);
			Indices.push_back(Face.mIndices[2]);
		}

		auto mesh = make_shared<Mesh>();
		mesh->InitializeMeshBuffers(device, cmdList, allocator, tracker, this, sizeof(Vertex), sizeof(UINT),
			D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, Vertices.data(), (UINT)Vertices.size(), Indices.data(), (UINT)Indices.size());
		meshes.push_back(mesh);
	}

	mMeshMap[path] = meshes;
}

void AssetManager::LoadTestTriangleModel(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, ComPtr<D3D12MA::Allocator> allocator, ResourceStateTracker& tracker)
{
	Vertex v1, v2, v3;
	v1.position = { 0, 1, 0 };
	v1.position = { 0.866f, -0.5f, 0 };
	v1.position = { -0.866f, -0.5f, 0 };

	const Vertex vertices[] = { v1, v2, v3 };

	auto mesh = make_shared<Mesh>();
	mesh->InitializeMeshBuffers(device, cmdList, allocator, tracker, this, sizeof(Vertex), NULL, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, vertices, 3, NULL, 0);

	std::vector<shared_ptr<Mesh>> meshes;
	meshes.push_back(mesh);
	mMeshMap["Triangle"] = meshes;
}

D3D12_CPU_DESCRIPTOR_HANDLE AssetManager::GetIndexedCPUHandle(const UINT& index)
{
	auto cpuStart = mSRVUAVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	cpuStart.ptr += mCbvSrvUavDescriptorSize * index;

	return cpuStart;
}

D3D12_GPU_DESCRIPTOR_HANDLE AssetManager::GetIndexedGPUHandle(const UINT& index)
{
	auto gpuStart = mSRVUAVDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

	gpuStart.ptr += mCbvSrvUavDescriptorSize * index;

	return gpuStart;
}

shared_ptr<Texture> AssetManager::LoadTexture(ID3D12Device5* device,
	ID3D12GraphicsCommandList4* cmdList,
	D3D12MA::Allocator* d3dAllocator,
	ResourceStateTracker& tracker,
	const std::wstring& filePath,
	const D3D12_RESOURCE_STATES& resourceStates,
	const D3D12_SRV_DIMENSION& srvDimension, const D3D12_UAV_DIMENSION& uavDimension,
	bool isSRV, bool isUAV)
{
	shared_ptr<Texture> newTexture = make_shared<Texture>();
	newTexture->LoadTextureFromDDS(device, cmdList, d3dAllocator, tracker, filePath, resourceStates);

	auto textureCPUHandle = GetIndexedCPUHandle(mHeapCurrentIndex);
	auto textureGPUHandle = GetIndexedGPUHandle(mHeapCurrentIndex);

	if (isSRV)
	{
		auto srv = newTexture->ShaderResourceView();
		device->CreateShaderResourceView(newTexture->GetResource(), &srv, textureCPUHandle);
		newTexture->SetSRVDescriptorHeapInfo(textureCPUHandle, textureGPUHandle, mHeapCurrentIndex);

		mHeapCurrentIndex++;
		
		textureCPUHandle = GetIndexedCPUHandle(mHeapCurrentIndex);
		textureGPUHandle = GetIndexedGPUHandle(mHeapCurrentIndex);
	}
	if (isUAV)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uav = {};
		uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		device->CreateUnorderedAccessView(newTexture->GetResource(), nullptr, &uav, textureCPUHandle);

		mHeapCurrentIndex++;
	}

	newTexture->SetSRVDimension(srvDimension);
	newTexture->SetUAVDimension(uavDimension);

	mTextures[filePath] = newTexture;


	return newTexture;
}

void AssetManager::SetTexture(ID3D12Device5* device,
	ID3D12GraphicsCommandList4* cmdList,
	ComPtr<D3D12MA::Allocation> alloc,
	const wstring& textureName,
	const D3D12_SRV_DIMENSION& srvDimension,
	const D3D12_UAV_DIMENSION& uavDimension, bool isSRV, bool isUAV)
{
	shared_ptr<Texture> newTexture = make_shared<Texture>();
	newTexture->SetResource(alloc);

	auto textureCPUHandle = GetIndexedCPUHandle(mHeapCurrentIndex);
	auto textureGPUHandle = GetIndexedGPUHandle(mHeapCurrentIndex);

	if (isSRV)
	{
		auto srv = newTexture->ShaderResourceView();
		device->CreateShaderResourceView(newTexture->GetResource(), &srv, textureCPUHandle);
		newTexture->SetSRVDescriptorHeapInfo(textureCPUHandle, textureGPUHandle, mHeapCurrentIndex);

		mHeapCurrentIndex++;

		textureCPUHandle = GetIndexedCPUHandle(mHeapCurrentIndex);
		textureGPUHandle = GetIndexedGPUHandle(mHeapCurrentIndex);
	}
	if (isUAV)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uav = {};
		uav.ViewDimension = uavDimension;
		device->CreateUnorderedAccessView(newTexture->GetResource(), nullptr, &uav, textureCPUHandle);
		newTexture->SetUAVDescriptorHeapInfo(textureCPUHandle, textureGPUHandle, mHeapCurrentIndex);

		mHeapCurrentIndex++;
	}

	newTexture->SetSRVDimension(srvDimension);
	newTexture->SetUAVDimension(uavDimension);

	mTextures[textureName] = newTexture;
}

void AssetManager::BuildAccelerationStructure(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, ComPtr<D3D12MA::Allocator> d3dAllocator, ResourceStateTracker tracker)
{
	for(auto i = mMeshMap.begin(); i != mMeshMap.end(); ++i)
		BuildBLAS(device, cmdList, d3dAllocator, tracker, i->second);
	BuildTLAS(device, cmdList, d3dAllocator, tracker, mTLASSize);
}

void AssetManager::BuildBLAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, ComPtr<D3D12MA::Allocator> d3dAllocator, ResourceStateTracker tracker, const vector<shared_ptr<Mesh>>& meshes)
{
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geomDescs;
	geomDescs.reserve(mMeshMap.size());

	for (auto i = meshes.begin(); i != meshes.end(); ++i)
	{
		D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
		geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geomDesc.Triangles.VertexBuffer.StartAddress = (*i)->GetVertexBufferAlloc()->GetResource()->GetGPUVirtualAddress();
		geomDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
		geomDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		geomDesc.Triangles.VertexCount = (*i)->GetVertexCount();

		if ((*i)->GetIndexCount() > 0)
		{
			geomDesc.Triangles.IndexBuffer = (*i)->GetIndexBufferAlloc()->GetResource()->GetGPUVirtualAddress();
			geomDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
			geomDesc.Triangles.IndexCount = (*i)->GetIndexCount();
		}
		geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

		geomDescs.push_back(geomDesc);
	}

	// Get the size requirements for the scratch and AS buffers
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	inputs.NumDescs = geomDescs.size();
	inputs.pGeometryDescs = geomDescs.data();
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
	device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	// Create the buffers. They need to support UAV, and since we are going to immediately use them, we create them with an unordered-access state
	AccelerationStructureBuffers buffers;
	buffers.mScratch = CreateBufferResource(device, cmdList, d3dAllocator, tracker, NULL, info.ScratchDataSizeInBytes, 1,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_DIMENSION_BUFFER, DXGI_FORMAT_UNKNOWN, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	buffers.mResult = CreateBufferResource(device, cmdList, d3dAllocator, tracker, NULL, info.ResultDataMaxSizeInBytes, 1,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_DIMENSION_BUFFER, DXGI_FORMAT_UNKNOWN, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	// Create the bottom-level AS
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = inputs;
	asDesc.DestAccelerationStructureData = buffers.mResult->GetResource()->GetGPUVirtualAddress();
	asDesc.ScratchAccelerationStructureData = buffers.mScratch->GetResource()->GetGPUVirtualAddress();

	cmdList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	// We need to insert a UAV barrier before using the acceleration structures in a raytracing operation
	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = buffers.mResult->GetResource();
	cmdList->ResourceBarrier(1, &uavBarrier);

	mBLAS.push_back(buffers);
}

void AssetManager::BuildTLAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, ComPtr<D3D12MA::Allocator> d3dAllocator, ResourceStateTracker tracker, UINT& tlasSize)
{
	// First, get the size of the TLAS buffers and create them
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	inputs.NumDescs = mBLAS.size();
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
	device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	// Create the buffers
	AccelerationStructureBuffers buffers;
	buffers.mScratch = CreateBufferResource(device, cmdList, d3dAllocator, tracker, NULL, info.ScratchDataSizeInBytes, 1,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_DIMENSION_BUFFER, DXGI_FORMAT_UNKNOWN, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	buffers.mResult = CreateBufferResource(device, cmdList, d3dAllocator, tracker, NULL, info.ResultDataMaxSizeInBytes, 1,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_DIMENSION_BUFFER, DXGI_FORMAT_UNKNOWN, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	tlasSize = info.ResultDataMaxSizeInBytes;

	// The instance desc should be inside a buffer, create and map the buffer
	buffers.mInstanceDesc = CreateBufferResource(device, cmdList, d3dAllocator, tracker, NULL, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * mBLAS.size(), 1,
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_DIMENSION_BUFFER, DXGI_FORMAT_UNKNOWN, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE, D3D12_HEAP_TYPE_UPLOAD);

	D3D12_RAYTRACING_INSTANCE_DESC* instanceDescs;
	buffers.mInstanceDesc->GetResource()->Map(0, nullptr, (void**)&instanceDescs);
	ZeroMemory(instanceDescs, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * 3);

	// The transformation matrices for the instances
	XMFLOAT4X4 transform;
	transform = Matrix4x4::Identity4x4(); // Identity

	for (uint32_t i = 0; i < mBLAS.size(); i++)
	{
		instanceDescs[i].InstanceID = i; // This value will be exposed to the shader via InstanceID()
		instanceDescs[i].InstanceContributionToHitGroupIndex = i;  // This is the offset inside the shader-table. Since we have unique constant-buffer for each instance, we need a different offset
		instanceDescs[i].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		XMFLOAT4X4 m = Matrix4x4::Transpose(transform);
		memcpy(instanceDescs[i].Transform, &m, sizeof(instanceDescs[i].Transform));
		instanceDescs[i].AccelerationStructure = mBLAS[i].mResult->GetResource()->GetGPUVirtualAddress();
		instanceDescs[i].InstanceMask = 0xFF;
	}

	// Unmap
	buffers.mInstanceDesc->GetResource()->Unmap(0, nullptr);

	// Create the TLAS
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = inputs;
	asDesc.Inputs.InstanceDescs = buffers.mInstanceDesc->GetResource()->GetGPUVirtualAddress();
	asDesc.DestAccelerationStructureData = buffers.mResult->GetResource()->GetGPUVirtualAddress();
	asDesc.ScratchAccelerationStructureData = buffers.mScratch->GetResource()->GetGPUVirtualAddress();

	cmdList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	// We need to insert a UAV barrier before using the acceleration structures in a raytracing operation
	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = buffers.mResult->GetResource();
	cmdList->ResourceBarrier(1, &uavBarrier);

	mTLAS = buffers;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.RaytracingAccelerationStructure.Location = mTLAS.mResult->GetResource()->GetGPUVirtualAddress();

	//Currently just do this...
	device->CreateShaderResourceView(nullptr, &srvDesc, GetIndexedCPUHandle(mHeapCurrentIndex));
	mHeapCurrentIndex++;
}
