#include "Pipeline.h"
#include "SubObject.h"
#include "AssetManager.h"
#include "Instance.h"

using namespace SubObject;

void Pipeline::CreatePipelineState(ComPtr<ID3D12Device5> device, const WCHAR* filename)
{
    std::array<D3D12_STATE_SUBOBJECT, 11> subobjects;
    uint32_t index = 0;

    // Create the DXIL library

    const WCHAR* entryPoints[] = { kAnyHitShader, kRayGenShader, kMissShader, kClosestHitShader, kShadowClosestHitShader, kShadowMissShader };
    DxilLibrary dxilLib = CreateDxilLibrary(filename, entryPoints, arraysize(entryPoints));
    subobjects[index++] = dxilLib.stateSubobject; // 0 Library

    HitProgram hitProgram(kAnyHitShader, kClosestHitShader, kHitGroup);
    subobjects[index++] = hitProgram.subObject; // 1 Hit Group

    HitProgram shadowHitProgram(kAnyHitShader, kShadowClosestHitShader, kShadowHitGroup);
    subobjects[index++] = shadowHitProgram.subObject; // 2 Hit Group

    // Create the miss root-signature and association
    D3D12_ROOT_SIGNATURE_DESC emptyDesc = {};
    emptyDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
    LocalRootSignature emptyRootSignature(device, emptyDesc);
    subobjects[index] = emptyRootSignature.subobject; // 3 Miss Root Sig

    uint32_t emptyRootIndex = index++; // 3
    const WCHAR* emptyRootSigExports[] = { kMissShader, kRayGenShader, kShadowMissShader };
    ExportAssociation emptyRootAssociation(emptyRootSigExports, arraysize(emptyRootSigExports), &(subobjects[emptyRootIndex]));
    subobjects[index++] = emptyRootAssociation.subobject; // 4 Associate Miss Root Sig to Miss Shader

    // Create the hit root-signature and association
    LocalRootSignature hitRootSignature(device, CreateHitRootDesc().desc);
    subobjects[index] = hitRootSignature.subobject; // 5 Hit Root Sig

    uint32_t hitRootIndex = index++; // 6
    const WCHAR* hitRootSigExports[] = { kAnyHitShader, kClosestHitShader };
    ExportAssociation hitRootAssociation(hitRootSigExports, 2, &(subobjects[hitRootIndex]));
    subobjects[index++] = hitRootAssociation.subobject; // 7 Associate Hit Root Sig to Hit Group

    // Bind the payload size to the programs
    ShaderConfig shaderConfig(sizeof(float) * 2, sizeof(float) * 3);
    subobjects[index] = shaderConfig.subobject; // 7 Shader Config

    uint32_t shaderConfigIndex = index++; // 8
    const WCHAR* shaderExports[] = { kAnyHitShader, kMissShader, kClosestHitShader, kRayGenShader, kShadowMissShader, kShadowClosestHitShader };
    ExportAssociation configAssociation(shaderExports, arraysize(shaderExports), &(subobjects[shaderConfigIndex]));
    subobjects[index++] = configAssociation.subobject;

    // Create the pipeline config
    PipelineConfig config(3);
    subobjects[index++] = config.subobject; // 9

    // Create the global root signature and store the empty signature
    GlobalRootSignature root(device, CreateGlobalRootDesc().desc);
    mGlobalRootSig = root.pRootSig;
    subobjects[index++] = root.subobject; // 10

    // Create the state
    D3D12_STATE_OBJECT_DESC desc;
    desc.NumSubobjects = index;
    desc.pSubobjects = subobjects.data();
    desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

    ThrowIfFailed(device->CreateStateObject(&desc, IID_PPV_ARGS(&mPipelineState)));
}

void Pipeline::CreateShaderTable(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, ComPtr<D3D12MA::Allocator> alloc,
    ResourceStateTracker& tracker, AssetManager& assetMgr)
{
    // Calculate the size and create the buffer
    mShaderTableEntrySize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    mShaderTableEntrySize += 8; // The ray-gen's descriptor table
    mShaderTableEntrySize = align_to(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT, mShaderTableEntrySize);
    uint32_t shaderTableSize = mShaderTableEntrySize * 11;

    mShaderTable = assetMgr.CreateResource(device, cmdList, alloc, tracker, NULL,
        shaderTableSize, 1, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_DIMENSION_BUFFER,
        DXGI_FORMAT_UNKNOWN, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE, D3D12_HEAP_TYPE_UPLOAD);

    // Map the buffer
    uint8_t* pData;
    mShaderTable->GetResource()->Map(0, nullptr, (void**)&pData);

    ComPtr<ID3D12StateObjectProperties> pRtsoProps;
    mPipelineState->QueryInterface(IID_PPV_ARGS(&pRtsoProps));

    memcpy(pData, pRtsoProps->GetShaderIdentifier(kRayGenShader), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    uint64_t heapStart = assetMgr.GetIndexedGPUHandle(0).ptr;
    *(uint64_t*)(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = heapStart;

    memcpy(pData + mShaderTableEntrySize, pRtsoProps->GetShaderIdentifier(kMissShader), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    memcpy(pData + mShaderTableEntrySize * 2 , pRtsoProps->GetShaderIdentifier(kShadowMissShader), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    const std::vector<std::shared_ptr<Instance>> instances = assetMgr.GetInstances();
    for (uint32_t i = 0; i < instances.size(); i++)
    {       
        uint8_t* pHitEntry = pData + mShaderTableEntrySize * ((i * 2) + 3);
        memcpy(pHitEntry, pRtsoProps->GetShaderIdentifier(kHitGroup), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        uint8_t* pCbDesc = pHitEntry + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        assert(((uint64_t)pCbDesc % 8) == 0);
        *(D3D12_GPU_VIRTUAL_ADDRESS*)pCbDesc = instances[i]->GetInstanceCB()->GetGPUVirtualAddress(0);

        memcpy(pHitEntry + mShaderTableEntrySize, pRtsoProps->GetShaderIdentifier(kShadowHitGroup), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        uint8_t* pShadowCbDesc = pHitEntry + mShaderTableEntrySize + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
        assert(((uint64_t)pShadowCbDesc % 8) == 0);
        *(D3D12_GPU_VIRTUAL_ADDRESS*)pShadowCbDesc = instances[i]->GetInstanceCB()->GetGPUVirtualAddress(0);
    }

    // Unmap
    mShaderTable->GetResource()->Unmap(0, nullptr);
}