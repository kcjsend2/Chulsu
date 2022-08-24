#include "Instance.h"

Instance::Instance(XMFLOAT3 position, XMFLOAT3 rotation, XMFLOAT3 scale) : mPosition(position), mRotation(rotation), mScale(scale)
{
}

void Instance::Update()
{
	mWorld = Matrix4x4::CalulateWorldTransform(mPosition, mRotation, mScale);
}

void Instance::BuildConstantBuffer(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, ComPtr<D3D12MA::Allocator> alloc, ResourceStateTracker& tracker, AssetManager& assetMgr)
{
	InstanceConstant instanceConst = { assetMgr.GetCurrentHeapIndex(), mMesh->GetVertexAttribIndex(), mMesh->GetIndexBufferIndex() };

	mInstanceCB = std::make_shared<UploadBuffer<InstanceConstant>>(device, cmdList, 1, alloc, tracker, assetMgr, true);
	mInstanceCB->CopyData(0, instanceConst);
}

void Instance::BuildStructuredBuffer(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, ComPtr<D3D12MA::Allocator> alloc, ResourceStateTracker& tracker, AssetManager& assetMgr)
{
	mGeometrySB = std::make_shared<UploadBuffer<GeometryInfo>>(device, cmdList, mMesh->GetSubMeshCount(), alloc, tracker, assetMgr, false);

	auto&subMeshes = mMesh->GetSubMeshes();

	for (int i = 0; i < subMeshes.size(); ++i)
	{
		auto materialIndeices = assetMgr.GetMaterialIndices(subMeshes[i].GetMaterialIndex());
		mGeometrySB->CopyData(i, GeometryInfo{
			materialIndeices.AlbedoTextureIndex, materialIndeices.MetalicTextureIndex,
			materialIndeices.RoughnessTextureIndex, materialIndeices.NormalMapTextureIndex,
			materialIndeices.OpacityMapTextureIndex });
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R32_UINT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;

	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = subMeshes.size() * 5;
	srvDesc.Buffer.StructureByteStride = 0;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	assetMgr.SetShaderResource(device, cmdList, mGeometrySB->GetUploadAllocation(), srvDesc);

	assetMgr.AddCurrentHeapIndex();
}

