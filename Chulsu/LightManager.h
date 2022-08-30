#pragma once
#include "stdafx.h"
#include "UploadBuffer.h"

enum class LightType : UINT
{
    DIRECTIONAL_LIGHT = 0,
    SPOT_LIGHT,
    POINT_LIGHT,
};

struct Light
{
    XMFLOAT3 position;
    int active;

    XMFLOAT3 direction;
    float range;

    XMFLOAT3 color;
    LightType type = LightType::DIRECTIONAL_LIGHT;

    float outerCosine;
    float innerCosine;
    int castShadows;
};

class LightManager
{
public:
    LightManager() = default;
    ~LightManager() = default;

    void Init(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList,
        ComPtr<D3D12MA::Allocator> alloc, ResourceStateTracker& tracker, AssetManager& assetMgr, UINT lightReserve);

    shared_ptr<UploadBuffer<Light>> GetLightSB() { return mLightSB; }
    vector<Light> GetLights() { return mLights; }
    void AddLight(Light light) { mLights.push_back(light); }
    void AddLight(XMFLOAT3 position, XMFLOAT3 direction, XMFLOAT3 color,
        bool active, float range, LightType type, float outerCosine, float innerCosine, bool castShadows);

    void SetControlableLight(Light light) { mControlableLight = light; }

    void Update();

private:
    UINT mNumReservedLights;

    Light mControlableLight;
    vector<Light> mLights;
    shared_ptr<UploadBuffer<Light>> mLightSB;
};