#include "LightManager.h"

void LightManager::Init(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, ComPtr<D3D12MA::Allocator> alloc, ResourceStateTracker& tracker, AssetManager& assetMgr, UINT lightReserve)
{
	mNumReservedLights = lightReserve;
	mLights.reserve(mNumReservedLights);
	mLightSB = std::make_shared<UploadBuffer<Light>>(device, cmdList, mNumReservedLights, alloc, tracker, assetMgr, false);
}

void LightManager::AddLight(XMFLOAT3 position, XMFLOAT3 direction, XMFLOAT3 color, bool active, float range, LightType type, float outerCosine, float innerCosine, bool castShadows)
{
	mLights.emplace_back(position, active, direction, range, color, type, outerCosine, innerCosine, castShadows);
}

void LightManager::Update()
{
	for (int i = 0; i < mLights.size(); ++i)
	{
		mLightSB->CopyData(i, mLights[i]);
	}
}
