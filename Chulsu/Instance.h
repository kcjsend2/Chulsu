#pragma once
#include "stdafx.h"
#include "Mesh.h"
#include "SubMesh.h"
#include "ConstantBuffer.h"

struct InstanceConstant
{
	UINT AlbedoTextureIndex;
	UINT MetalicTextureIndex;
	UINT RoughnessTextureIndex;
	UINT NormalMapTextureIndex;

	UINT VertexAttribIndex;
	UINT IndexBufferIndex;
};

class Instance
{
public:
	Instance() = default;
	Instance(XMFLOAT3 position, XMFLOAT3 rotation, XMFLOAT3 scale);

	shared_ptr<Mesh>& GetMesh() { return mMesh; }
	void SetMesh(shared_ptr<Mesh> mesh) { mMesh = mesh; }

	void SetPosition(XMFLOAT3 position) { mPosition = position; }
	void SetRotation(XMFLOAT3 rotation) { mRotation = rotation; }
	void SetScale(XMFLOAT3 scale) { mScale = scale; }
	void SetHitGroup(UINT hitGroupIndex) { mHitGroupIndex = hitGroupIndex; }

	void Update();

	void BuildConstantBuffer(
		ID3D12Device5* device,
		ID3D12GraphicsCommandList4* cmdList,
		ComPtr<D3D12MA::Allocator> alloc,
		ResourceStateTracker& tracker,
		AssetManager& assetMgr);

	std::shared_ptr<ConstantBuffer<InstanceConstant>> GetInstanceCB() { return mInstanceCB; }

	const XMFLOAT4X4& GetWorldMatrix() { return mWorld; }
	const UINT& GetHitGroupIndex() { return mHitGroupIndex; }

private:
	ComPtr<D3D12MA::Allocation> mConstantBufferAlloc;

	std::shared_ptr<ConstantBuffer<InstanceConstant>> mInstanceCB;

	XMFLOAT4X4 mWorld = {};

	XMFLOAT3 mPosition = {0, 0, 0};
	XMFLOAT3 mRotation = { 0, 0, 0 };
	XMFLOAT3 mScale = { 1, 1, 1 };

	UINT mAlbedoTextureIndex = UINT_MAX;
	UINT mMetalicTextureIndex = UINT_MAX;
	UINT mRoughnessTextureIndex = UINT_MAX;
	UINT mNormalMapTextureIndex = UINT_MAX;

	UINT mHitGroupIndex = UINT_MAX;

	shared_ptr<Mesh> mMesh;
};