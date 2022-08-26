#include "AssetManager.h"
#include "SubMesh.h"
#include "Texture.h"
#include "Instance.h"
#include "Mesh.h"

AssetManager::AssetManager()
{
}

void AssetManager::Init(ID3D12Device* device, int numDescriptor)
{
	ThrowIfFailed(DStorageGetFactory(IID_PPV_ARGS(&mTextureFactory)));

	DSTORAGE_QUEUE_DESC queueDesc{};
	queueDesc.Capacity = DSTORAGE_MAX_QUEUE_CAPACITY;
	queueDesc.Priority = DSTORAGE_PRIORITY_NORMAL;
	queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
	queueDesc.Device = device;

	ThrowIfFailed(mTextureFactory->CreateQueue(&queueDesc, IID_PPV_ARGS(&mTextureQueue)));


	auto descHeapDescriptor = DescriptorHeapDesc(
		numDescriptor,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		0);

	ThrowIfFailed(device->CreateDescriptorHeap(
		&descHeapDescriptor,
		IID_PPV_ARGS(&mDescriptorHeap)));
}

ComPtr<D3D12MA::Allocation> AssetManager::CreateResource(
	ID3D12Device5* device,
	ID3D12GraphicsCommandList4* cmdList,
	ComPtr<D3D12MA::Allocator> alloc,
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
		width, height, 1, 1, format, 1, 0, layout, flag);

	auto resourceState = initData != NULL ? D3D12_RESOURCE_STATE_COPY_DEST : initialState;
	ComPtr<D3D12MA::Allocation> defaultAllocation;
	alloc->CreateResource(
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
		alloc->CreateResource(
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

void AssetManager::LoadAssimpScene(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList,
	D3D12MA::Allocator* alloc, ResourceStateTracker& tracker, const std::string& path)
{
	//Hard coded, fix it later.
	aiString dirPath = aiString("Contents/Sponza/");

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

	vector<SubMesh> subMeshes;
	subMeshes.reserve(pAiScene->mNumMeshes);
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

		auto matIndex = pAiMesh->mMaterialIndex;
		aiMaterial* pAiMaterial = pAiScene->mMaterials[matIndex];

		if(!mTextureIndices[matIndex].init)
		{
			mTextureIndices[matIndex].init = true;
			int textureNum = 0;
			for (int i = 1; i < aiTextureType_UNKNOWN + 1; ++i)
			{
				aiString path;
				if (pAiMaterial->GetTexture((aiTextureType)i, 0, &path) == AI_SUCCESS)
				{
					textureNum++;
					aiString fullPath = dirPath;
					fullPath.Append(path.data);

					wstring wPath = stringTowstring(string(fullPath.C_Str()));

					if (mTextures[wPath] == nullptr)
					{
						LoadTexture(device, cmdList, alloc, tracker, wPath,
							D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_SRV_DIMENSION_TEXTURE2D, D3D12_UAV_DIMENSION_UNKNOWN, true, false, FLAG_WIC);

						switch (aiTextureType(i))
						{
						case aiTextureType_DIFFUSE:
							mTextureIndices[matIndex].AlbedoTextureIndex = mHeapCurrentIndex - 1;
							break;

						case aiTextureType_AMBIENT:
							mTextureIndices[matIndex].MetalicTextureIndex = mHeapCurrentIndex - 1;
							break;

						case aiTextureType_HEIGHT:
							mTextureIndices[matIndex].NormalMapTextureIndex = mHeapCurrentIndex - 1;
							break;

						case aiTextureType_SHININESS:
							mTextureIndices[matIndex].RoughnessTextureIndex = mHeapCurrentIndex - 1;
							break;

						case aiTextureType_OPACITY:
							mTextureIndices[matIndex].OpacityMapTextureIndex = mHeapCurrentIndex - 1;
							break;

						}
					}
				}
			}
		}

		SubMesh subMesh;
		subMesh.SetMaterialIndex(matIndex);
		subMesh.SetName(pAiMesh->mName.C_Str());
		subMesh.InitializeBuffers(device, cmdList, alloc, tracker, *this, sizeof(Vertex), sizeof(UINT),
			D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, Vertices.data(), (UINT)Vertices.size(), Indices.data(), (UINT)Indices.size());
		subMeshes.push_back(subMesh);
	}
	shared_ptr<Mesh> mesh = make_shared<Mesh>(subMeshes);

	UINT vertexBufferIndex = UINT_MAX;
	for (int i = 0; i < subMeshes.size(); ++i)
	{
		if (i == 0)
		{
			vertexBufferIndex = SetShaderResource(device, cmdList, subMeshes[i].GetVertexBufferAlloc(), subMeshes[i].VertexShaderResourceView());
		}
		else
			SetShaderResource(device, cmdList, subMeshes[i].GetVertexBufferAlloc(), subMeshes[i].VertexShaderResourceView());

		mHeapCurrentIndex++;
	}

	UINT IndexBufferIndex = UINT_MAX;
	for (int i = 0; i < subMeshes.size(); ++i)
	{
		if (i == 0)
		{
			IndexBufferIndex = SetShaderResource(device, cmdList, subMeshes[i].GetIndexBufferAlloc(), subMeshes[i].IndexShaderResourceView());
		}
		else
			SetShaderResource(device, cmdList, subMeshes[i].GetIndexBufferAlloc(), subMeshes[i].IndexShaderResourceView());

		mHeapCurrentIndex++;
	}

	mesh->SetVertexAttribIndex(vertexBufferIndex);
	mesh->SetIndexBufferIndex(IndexBufferIndex);
	mMeshMap[path] = mesh;
}


void AssetManager::CreateInstance(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, D3D12MA::Allocator* alloc,
	ResourceStateTracker& tracker, const std::string& path,
	XMFLOAT3 position, XMFLOAT3 rotation, XMFLOAT3 scale)
{
	if(mMeshMap[path] == NULL)
		LoadAssimpScene(device, cmdList, alloc, tracker, path);

	shared_ptr<Instance> instance = make_shared<Instance>(position, rotation, scale);
	instance->SetMesh(mMeshMap[path]);
	instance->BuildConstantBuffer(device, cmdList, alloc, tracker, *this);
	instance->BuildStructuredBuffer(device, cmdList, alloc, tracker, *this);
	instance->Update();

	mInstances.push_back(instance);
}


void AssetManager::LoadTestInstance(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, ComPtr<D3D12MA::Allocator> alloc, ResourceStateTracker& tracker)
{
	Vertex v1, v2, v3;
	v1.position = { 0, 1, 0 };
	v2.position = { 0.866f, -0.5f, 0 };
	v3.position = { -0.866f, -0.5f, 0 };

	const Vertex vertices[] = { v1, v2, v3 };
	const XMFLOAT3 positions[] = { {0, 0, 0}, {-2, 0, 0}, {2, 0, 0} };

	shared_ptr<Instance> instance[4] = { make_shared<Instance>(), make_shared<Instance>(), make_shared<Instance>(), make_shared<Instance>() };

	SubMesh subMesh;
	subMesh.InitializeBuffers(device, cmdList, alloc, tracker, *this, sizeof(Vertex), NULL, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, vertices, 3, NULL, 0);

	vector<SubMesh> subMeshes;
	subMeshes.push_back(subMesh);

	auto mesh = make_shared<Mesh>(subMeshes);
	mMeshMap["Triangle"] = mesh;

	for (int i = 0; i < 3; ++i)
	{
		instance[i]->SetMesh(mMeshMap["Triangle"]);
		instance[i]->BuildConstantBuffer(device, cmdList, alloc, tracker, *this);
		instance[i]->SetPosition(positions[i]);
		instance[i]->Update();
		mInstances.push_back(instance[i]);
	}

	UINT vertexBufferIndex = SetShaderResource(device, cmdList, subMesh.GetVertexBufferAlloc(), subMesh.VertexShaderResourceView());
	mMeshMap["Triangle"]->SetVertexAttribIndex(vertexBufferIndex);

	Vertex v4, v5, v6, v7, v8, v9;
		
	v4.position = { -100, -1, -2 };
	v5.position = { 100, -1,  100 };
	v6.position = { -100, -1,  100 };
	v7.position = { -100, -1,  -2 };
	v8.position = { 100, -1,  -2 };
	v9.position = { 100, -1,  100 };

	const Vertex planeVertices[] = { v4, v5, v6, v7, v8, v9 };

	SubMesh planeSubMesh;
	planeSubMesh.InitializeBuffers(device, cmdList, alloc, tracker, *this, sizeof(Vertex), NULL, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, planeVertices, 6, NULL, 0);

	vector<SubMesh> planeSubMeshes;
	planeSubMeshes.push_back(planeSubMesh);

	auto planeMesh = make_shared<Mesh>(planeSubMeshes);
	mMeshMap["Plane"] = planeMesh;

	instance[3]->SetMesh(mMeshMap["Plane"]);
	instance[3]->BuildConstantBuffer(device, cmdList, alloc, tracker, *this);
	instance[3]->SetPosition(positions[0]);
	instance[3]->Update();
	mInstances.push_back(instance[3]);
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

void AssetManager::LoadTexture(ID3D12Device5* device,
	ID3D12GraphicsCommandList4* cmdList,
	D3D12MA::Allocator* alloc,
	ResourceStateTracker& tracker,
	const std::wstring& filePath,
	const D3D12_RESOURCE_STATES& resourceStates,
	const D3D12_SRV_DIMENSION& srvDimension, const D3D12_UAV_DIMENSION& uavDimension,
	bool isSRV, bool isUAV, FLAG_TEXTURE_LOAD flag)
{
	shared_ptr<Texture> newTexture = make_shared<Texture>();

	//ComPtr<IDStorageFile> textureFile;
	//mTextureFactory->OpenFile(filePath.c_str(), IID_PPV_ARGS(&textureFile));

	//BY_HANDLE_FILE_INFORMATION fileInfo{};
	//ThrowIfFailed(textureFile->GetFileInformation(&fileInfo));
	//uint32_t fileSize = fileInfo.nFileSizeLow;

	//ComPtr<D3D12MA::Allocation> textureAlloc;
	//D3D12MA::ALLOCATION_DESC allocationDesc = {};
	//allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

	//auto resourceDesc = CD3DX12_RESOURCE_DESC(D3D12_RESOURCE_DIMENSION_BUFFER,
	//	D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, fileSize, 1, 1,
	//	1, DXGI_FORMAT_UNKNOWN, 1, 0, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE);

	//alloc->CreateResource(
	//	&allocationDesc,
	//	&resourceDesc,
	//	D3D12_RESOURCE_STATE_GENERIC_READ,
	//	NULL,
	//	&textureAlloc,
	//	IID_NULL, NULL);

	//DSTORAGE_REQUEST textureRequest = {};
	//textureRequest.Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
	//textureRequest.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_BUFFER;
	//textureRequest.Source.File.Source = textureFile.Get();
	//textureRequest.Source.File.Offset = 0;
	//textureRequest.Source.File.Size = fileSize;
	//textureRequest.UncompressedSize = fileSize;
	//textureRequest.Destination.Buffer.Resource = textureAlloc->GetResource();
	//textureRequest.Destination.Buffer.Offset = 0;
	//textureRequest.Destination.Buffer.Size = textureRequest.Source.File.Size;

	//mTextureQueue->EnqueueRequest(&textureRequest);

	//// Need Fence event.
	//mTextureQueue->Submit();


	if(flag == FLAG_WIC)
		newTexture->LoadTextureFromWIC(device, cmdList, alloc, tracker,*this, filePath, resourceStates);

	else if(flag == FLAG_DDS)
		newTexture->LoadTextureFromDDS(device, cmdList, alloc, tracker, *this, filePath, resourceStates);

	auto textureCPUHandle = GetIndexedCPUHandle(mHeapCurrentIndex);
	auto textureGPUHandle = GetIndexedGPUHandle(mHeapCurrentIndex);

	if (isSRV)
	{
		newTexture->SetSRVDimension(D3D12_SRV_DIMENSION_TEXTURE2D);
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

UINT AssetManager::SetShaderResource(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, ComPtr<D3D12MA::Allocation> alloc, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc)
{
	auto bufferCPUHandle = GetIndexedCPUHandle(mHeapCurrentIndex);

	device->CreateShaderResourceView(alloc->GetResource(), &desc, bufferCPUHandle);

	return mHeapCurrentIndex;
}

void AssetManager::SetConstantBuffer(ID3D12Device5* device, const D3D12_CONSTANT_BUFFER_VIEW_DESC desc)
{
	auto bufferCPUHandle = GetIndexedCPUHandle(mHeapCurrentIndex);

	device->CreateConstantBufferView(&desc, bufferCPUHandle);

	mHeapCurrentIndex++;
}

void AssetManager::BuildAccelerationStructure(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, ComPtr<D3D12MA::Allocator> alloc, ResourceStateTracker& tracker)
{
	BuildBLAS(device, cmdList, alloc, tracker);
	BuildTLAS(device, cmdList, alloc, tracker, mTLASSize);
}

void AssetManager::BuildBLAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, ComPtr<D3D12MA::Allocator> alloc, ResourceStateTracker& tracker)
{

	for (auto i = mMeshMap.begin(); i != mMeshMap.end(); ++i)
	{
		std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geomDescs;

		auto mesh = i->second;
		auto subMeshes = mesh->GetSubMeshes();
		for (auto j = subMeshes.begin(); j != subMeshes.end(); ++j)
		{
			D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
			geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
			geomDesc.Triangles.VertexBuffer.StartAddress = (*j).GetVertexBufferAlloc()->GetResource()->GetGPUVirtualAddress();
			geomDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
			geomDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
			geomDesc.Triangles.VertexCount = (*j).GetVertexCount();

			if ((*j).GetIndexCount() > 0)
			{
				geomDesc.Triangles.IndexBuffer = (*j).GetIndexBufferAlloc()->GetResource()->GetGPUVirtualAddress();
				geomDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
				geomDesc.Triangles.IndexCount = (*j).GetIndexCount();
			}

			if (GetMaterialIndices(j->GetMaterialIndex()).OpacityMapTextureIndex == UINT_MAX)
				geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
			else
				geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;

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
		buffers.mScratch = CreateResource(device, cmdList, alloc, tracker, NULL, info.ScratchDataSizeInBytes, 1,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_DIMENSION_BUFFER, DXGI_FORMAT_UNKNOWN, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		buffers.mResult = CreateResource(device, cmdList, alloc, tracker, NULL, info.ResultDataMaxSizeInBytes, 1,
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

		mesh->SetBLAS(buffers);
	}
}

void AssetManager::BuildTLAS(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, ComPtr<D3D12MA::Allocator> alloc, ResourceStateTracker& tracker, UINT& tlasSize)
{
	// First, get the size of the TLAS buffers and create them
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	inputs.NumDescs = mInstances.size();
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
	device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	AccelerationStructureBuffers buffers;
	buffers.mScratch = CreateResource(device, cmdList, alloc, tracker, NULL, info.ScratchDataSizeInBytes, 1,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_DIMENSION_BUFFER, DXGI_FORMAT_UNKNOWN, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	buffers.mResult = CreateResource(device, cmdList, alloc, tracker, NULL, info.ResultDataMaxSizeInBytes, 1,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_RESOURCE_DIMENSION_BUFFER, DXGI_FORMAT_UNKNOWN, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	tlasSize = info.ResultDataMaxSizeInBytes;

	buffers.mInstanceDesc = CreateResource(device, cmdList, alloc, tracker, NULL, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * mInstances.size(), 1,
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_DIMENSION_BUFFER, DXGI_FORMAT_UNKNOWN, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE, D3D12_HEAP_TYPE_UPLOAD);

	D3D12_RAYTRACING_INSTANCE_DESC* instanceDescs;
	buffers.mInstanceDesc->GetResource()->Map(0, nullptr, (void**)&instanceDescs);
	ZeroMemory(instanceDescs, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * mInstances.size());

	for (uint32_t i = 0; i < mInstances.size(); i++)
	{
		XMFLOAT4X4 transform;
		transform = mInstances[i]->GetWorldMatrix();

		instanceDescs[i].InstanceID = i;
		mInstances[i]->SetHitGroup(i * 2);
		instanceDescs[i].InstanceContributionToHitGroupIndex = i * 2;
		instanceDescs[i].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		XMFLOAT4X4 m = Matrix4x4::Transpose(transform);
		memcpy(instanceDescs[i].Transform, &m, sizeof(instanceDescs[i].Transform));
		instanceDescs[i].AccelerationStructure = mInstances[i]->GetMesh()->GetBLAS().mResult->GetResource()->GetGPUVirtualAddress();
		instanceDescs[i].InstanceMask = 0xFF;
	}

	buffers.mInstanceDesc->GetResource()->Unmap(0, nullptr);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = inputs;
	asDesc.Inputs.InstanceDescs = buffers.mInstanceDesc->GetResource()->GetGPUVirtualAddress();
	asDesc.DestAccelerationStructureData = buffers.mResult->GetResource()->GetGPUVirtualAddress();
	asDesc.ScratchAccelerationStructureData = buffers.mScratch->GetResource()->GetGPUVirtualAddress();

	cmdList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = buffers.mResult->GetResource();
	cmdList->ResourceBarrier(1, &uavBarrier);

	mTLAS = buffers;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.RaytracingAccelerationStructure.Location = mTLAS.mResult->GetResource()->GetGPUVirtualAddress();
}
