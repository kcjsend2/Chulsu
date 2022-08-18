#include "Pipeline.h"
#include "SubObject.h"
#include "AssetManager.h"

using namespace SubObject;

void Pipeline::CreatePipelineState(ComPtr<ID3D12Device5> device, const WCHAR* filename)
{
    std::array<D3D12_STATE_SUBOBJECT, 10> subobjects;
    uint32_t index = 0;

    // Create the DXIL library
    DxilLibrary dxilLib = CreateDxilLibrary(filename);
    subobjects[index++] = dxilLib.stateSubobject; // 0 Library

    HitProgram hitProgram(nullptr, kClosestHitShader, kHitGroup);
    subobjects[index++] = hitProgram.subObject; // 1 Hit Group

    // Create the ray-gen root-signature and association
    LocalRootSignature rgsRootSignature(device, CreateRayGenRootDesc().desc);
    subobjects[index] = rgsRootSignature.subobject; // 2 RayGen Root Sig

    uint32_t rgsRootIndex = index++; // 2
    ExportAssociation rgsRootAssociation(&kRayGenShader, 1, &(subobjects[rgsRootIndex]));
    subobjects[index++] = rgsRootAssociation.subobject; // 3 Associate Root Sig to RGS

    // Create the miss- and hit-programs root-signature and association
    D3D12_ROOT_SIGNATURE_DESC emptyDesc = {};
    emptyDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
    LocalRootSignature hitMissRootSignature(device, emptyDesc);
    subobjects[index] = hitMissRootSignature.subobject; // 4 Root Sig to be shared between Miss and CHS

    uint32_t hitMissRootIndex = index++; // 4
    const WCHAR* missHitExportName[] = { kMissShader, kClosestHitShader };
    ExportAssociation missHitRootAssociation(missHitExportName, arraysize(missHitExportName), &(subobjects[hitMissRootIndex]));
    subobjects[index++] = missHitRootAssociation.subobject; // 5 Associate Root Sig to Miss and CHS

    // Bind the payload size to the programs
    ShaderConfig shaderConfig(sizeof(float) * 2, sizeof(float) * 3);
    subobjects[index] = shaderConfig.subobject; // 6 Shader Config

    uint32_t shaderConfigIndex = index++; // 6
    const WCHAR* shaderExports[] = { kMissShader, kClosestHitShader, kRayGenShader };
    ExportAssociation configAssociation(shaderExports, arraysize(shaderExports), &(subobjects[shaderConfigIndex]));
    subobjects[index++] = configAssociation.subobject; // 7 Associate Shader Config to Miss, CHS, RGS

    // Create the pipeline config
    PipelineConfig config(1);
    subobjects[index++] = config.subobject; // 8

    // Create the global root signature and store the empty signature
    GlobalRootSignature root(device, {});
    mEmptyRootSig = root.pRootSig;
    subobjects[index++] = root.subobject; // 9

    // Create the state
    D3D12_STATE_OBJECT_DESC desc;
    desc.NumSubobjects = index; // 10
    desc.pSubobjects = subobjects.data();
    desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

    ThrowIfFailed(device->CreateStateObject(&desc, IID_PPV_ARGS(&mPipelineState)));
}

void Pipeline::CreateShaderTable(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, ComPtr<D3D12MA::Allocator> alloc,
    ResourceStateTracker tracker, AssetManager& assetMgr)
{
    /** The shader-table layout is as follows:
        Entry 0 - Ray-gen program
        Entry 1 - Miss program
        Entry 2 - Hit program
        All entries in the shader-table must have the same size, so we will choose it base on the largest required entry.
        The ray-gen program requires the largest entry - sizeof(program identifier) + 8 bytes for a descriptor-table.
        The entry size must be aligned up to D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT
    */

    // Calculate the size and create the buffer
    mShaderTableEntrySize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    mShaderTableEntrySize += 8; // The ray-gen's descriptor table
    mShaderTableEntrySize = align_to(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT, mShaderTableEntrySize);
    uint32_t shaderTableSize = mShaderTableEntrySize * 3;

    mShaderTable = assetMgr.CreateResource(device, cmdList, alloc, tracker, NULL,
        shaderTableSize, 1, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_DIMENSION_BUFFER,
        DXGI_FORMAT_UNKNOWN, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE, D3D12_HEAP_TYPE_UPLOAD);

    // Map the buffer
    uint8_t* pData;
    mShaderTable->GetResource()->Map(0, nullptr, (void**)&pData);

    ComPtr<ID3D12StateObjectProperties> pRtsoProps;
    mPipelineState->QueryInterface(IID_PPV_ARGS(&pRtsoProps));

    // Entry 0 - ray-gen program ID and descriptor data
    memcpy(pData, pRtsoProps->GetShaderIdentifier(kRayGenShader), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    uint64_t heapStart = assetMgr.GetIndexedGPUHandle(0).ptr;
    *(uint64_t*)(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) = heapStart;

    // Entry 1 - miss program
    memcpy(pData + mShaderTableEntrySize, pRtsoProps->GetShaderIdentifier(kMissShader), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    // Entry 2 - hit program
    uint8_t* pHitEntry = pData + mShaderTableEntrySize * 2; // +2 skips the ray-gen and miss entries
    memcpy(pHitEntry, pRtsoProps->GetShaderIdentifier(kHitGroup), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    // Unmap
    mShaderTable->GetResource()->Unmap(0, nullptr);
}