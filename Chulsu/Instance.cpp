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
	InstanceConstant instanceConst = { mAlbedoTextureIndex, mMetalicTextureIndex, mRoughnessTextureIndex, mNormalMapTextureIndex,
		mMesh->GetVertexAttribIndex(), mMesh->GetIndexBufferIndex() };

	mInstanceCB = std::make_shared<ConstantBuffer<InstanceConstant>>(device, cmdList, 1, alloc, tracker, assetMgr);
	mInstanceCB->CopyData(0, instanceConst);
}

