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
	std::vector<shared_ptr<Mesh>> Meshes;

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

	Meshes.reserve(pAiScene->mNumMeshes);
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
		Meshes.push_back(mesh);
	}

	mMeshes[path] = Meshes;
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
