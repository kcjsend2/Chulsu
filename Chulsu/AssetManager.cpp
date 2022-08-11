#include "AssetManager.h"

AssetManager::AssetManager(ID3D12Device* device, int numDescriptor)
{
	auto descHeapDescriptor = DescriptorHeapDesc(
		numDescriptor,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		0);

	ThrowIfFailed(device->CreateDescriptorHeap(
		&descHeapDescriptor,
		IID_PPV_ARGS(&mDescriptorHeap)));
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
		mesh->InitializeMeshBuffers(device, cmdList, allocator, tracker, sizeof(Vertex), sizeof(UINT),
			D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, Vertices.data(), (UINT)Vertices.size(), Indices.data(), (UINT)Indices.size());
		meshes.push_back(mesh);
	}

	mMeshMap[path] = meshes;
}

void AssetManager::LoadTestTriangleModel(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, D3D12MA::Allocator* allocator, ResourceStateTracker& tracker)
{
	Vertex v1, v2, v3;
	v1.position = { 0, 1, 0 };
	v1.position = { 0.866f, -0.5f, 0 };
	v1.position = { -0.866f, -0.5f, 0 };

	const Vertex vertices[] = { v1, v2, v3 };

	auto mesh = make_shared<Mesh>();
	mesh->InitializeMeshBuffers(device, cmdList, allocator, tracker, sizeof(Vertex), NULL, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, vertices, 3, NULL, 0);

	std::vector<shared_ptr<Mesh>> meshes;
	meshes.push_back(mesh);
	mMeshMap["Triangle"] = meshes;
}

D3D12_CPU_DESCRIPTOR_HANDLE AssetManager::GetIndexedCPUHandle(const UINT& index)
{
	auto cpuStart = mDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	cpuStart.ptr += mCbvSrvUavDescriptorSize * index;

	return cpuStart;
}

D3D12_GPU_DESCRIPTOR_HANDLE AssetManager::GetIndexedGPUHandle(const UINT& index)
{
	auto gpuStart = mDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

	gpuStart.ptr += mCbvSrvUavDescriptorSize * index;

	return gpuStart;
}

shared_ptr<Texture> AssetManager::LoadTexture(ID3D12Device5* device,
	ID3D12GraphicsCommandList4* cmdList,
	D3D12MA::Allocator* d3dAllocator,
	ResourceStateTracker& tracker,
	const std::wstring& filePath,
	const D3D12_RESOURCE_STATES& resourceStates,
	const D3D12_SRV_DIMENSION& dimension)
{
	shared_ptr<Texture> newTexture = make_shared<Texture>();
	newTexture->LoadTextureFromDDS(device, cmdList, d3dAllocator, tracker, filePath, resourceStates);

	auto textureCPUHandle = GetIndexedCPUHandle(mHeapCurrentIndex);
	auto textureGPUHandle = GetIndexedGPUHandle(mHeapCurrentIndex);

	auto srv = newTexture->ShaderResourceView();
	device->CreateShaderResourceView(newTexture->GetResource(), &srv, textureCPUHandle);
	newTexture->SetDescriptorHeapInfo(textureCPUHandle, textureGPUHandle, mHeapCurrentIndex);

	newTexture->SetDimension(dimension);

	mTextures.push_back(newTexture);

	mHeapCurrentIndex++;

	return newTexture;
}

void AssetManager::BuildAcceleerationStructure()
{
}

void AssetManager::BuildBLAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, D3D12MA::Allocator* d3dAllocator, ResourceStateTracker tracker, const vector<shared_ptr<Mesh>>& meshes)
{
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geomDescs;
	geomDescs.reserve(mMeshMap.size());

	for (auto i = meshes.begin(); i != meshes.end(); ++i)
	{
		D3D12_RAYTRACING_GEOMETRY_DESC geomDesc;
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
	buffers.mScratch = CreateBufferResource(device, cmdList, d3dAllocator, tracker, NULL, info.ScratchDataSizeInBytes,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	buffers.mResult = CreateBufferResource(device, cmdList, d3dAllocator, tracker, NULL, info.ResultDataMaxSizeInBytes,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

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

	mBLAS = buffers;
}

void AssetManager::BuildTLAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, ID3D12Resource* pBottomLevelAS[2], uint64_t& tlasSize)
{
	// First, get the size of the TLAS buffers and create them
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	inputs.NumDescs = 3;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
	device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	// Create the buffers
	AccelerationStructureBuffers buffers;
	buffers.mScratch = createBuffer(pDevice, info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kDefaultHeapProps);
	buffers.mResult = createBuffer(pDevice, info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, kDefaultHeapProps);
	tlasSize = info.ResultDataMaxSizeInBytes;

	// The instance desc should be inside a buffer, create and map the buffer
	buffers.mInstanceDesc = createBuffer(pDevice, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * 3, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
	D3D12_RAYTRACING_INSTANCE_DESC* instanceDescs;
	buffers.mInstanceDesc->GetResource()->Map(0, nullptr, (void**)&instanceDescs);
	ZeroMemory(instanceDescs, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * 3);

	// The transformation matrices for the instances
	XMFLOAT4X4 transformation[3];
	transformation[0] = Matrix4x4::Identity4x4(); // Identity
	transformation[1] = Matrix4x4::Identity4x4();
	transformation[2] = Matrix4x4::Identity4x4();

	// Create the desc for the triangle/plane instance
	instanceDescs[0].InstanceID = 0;
	instanceDescs[0].InstanceContributionToHitGroupIndex = 0;
	instanceDescs[0].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
	instanceDescs[0].Transform = NULL;
	instanceDescs[0].AccelerationStructure = pBottomLevelAS[0]->GetGPUVirtualAddress();
	instanceDescs[0].InstanceMask = 0xFF;

	for (uint32_t i = 1; i < 3; i++)
	{
		instanceDescs[i].InstanceID = i; // This value will be exposed to the shader via InstanceID()
		instanceDescs[i].InstanceContributionToHitGroupIndex = i;  // This is the offset inside the shader-table. Since we have unique constant-buffer for each instance, we need a different offset
		instanceDescs[i].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		mat4 m = transpose(transformation[i]); // GLM is column major, the INSTANCE_DESC is row major
		memcpy(instanceDescs[i].Transform, &m, sizeof(instanceDescs[i].Transform));
		instanceDescs[i].AccelerationStructure = pBottomLevelAS[1]->GetGPUVirtualAddress();
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
}
