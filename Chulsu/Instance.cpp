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
	InstanceConstant instanceConst = { assetMgr.GetCurrentHeapIndex() + 1, mMesh->GetVertexAttribIndex(), mMesh->GetIndexBufferIndex() };

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
}

