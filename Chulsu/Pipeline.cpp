#include "Pipeline.h"
#include "SubObject.h"
#include "AssetManager.h"
#include "Instance.h"

using namespace SubObject;

void Pipeline::CreatePipelineState(ComPtr<ID3D12Device5> device, const WCHAR* filename)
{
    // Need 12 subobjects:
  //  1 for DXIL library
  //  1 for the hit-group
  //  2 for RayGen root-signature (root-signature and the subobject association)
  //  2 for hit-program root-signature (root-signature and the subobject association)
  //  2 for miss-shader root-signature (signature and association)
  //  2 for shader config (shared between all programs. 1 for the config, 1 for association)
  //  1 for pipeline config
  //  1 for the global root signature
    std::array<D3D12_STATE_SUBOBJECT, 9> subobjects;
    uint32_t index = 0;

    // Create the DXIL library
    DxilLibrary dxilLib = CreateDxilLibrary(filename);
    subobjects[index++] = dxilLib.stateSubobject; // 0 Library

    HitProgram hitProgram(nullptr, kClosestHitShader, kHitGroup);
    subobjects[index++] = hitProgram.subObject; // 1 Hit Group

    // Create the miss root-signature and association
    D3D12_ROOT_SIGNATURE_DESC emptyDesc = {};
    emptyDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
    LocalRootSignature emptyRootSignature(device, emptyDesc);
    subobjects[index] = emptyRootSignature.subobject; // 2 Miss Root Sig

    uint32_t emptyRootIndex = index++; // 2
    const WCHAR* emptyRootSigExports[] = { kMissShader, kClosestHitShader, kRayGenShader };
    ExportAssociation missRootAssociation(emptyRootSigExports, 1, &(subobjects[emptyRootIndex]));
    subobjects[index++] = missRootAssociation.subobject; // 3 Associate Miss Root Sig to Miss Shader

    // Bind the payload size to the programs
    ShaderConfig shaderConfig(sizeof(float) * 2, sizeof(float) * 3);
    subobjects[index] = shaderConfig.subobject; // 4 Shader Config

    uint32_t shaderConfigIndex = index++; // 5
    const WCHAR* shaderExports[] = { kMissShader, kClosestHitShader, kRayGenShader };
    ExportAssociation configAssociation(shaderExports, arraysize(shaderExports), &(subobjects[shaderConfigIndex]));
    subobjects[index++] = configAssociation.subobject; // 6 Associate Shader Config to shaders

    // Create the pipeline config
    PipelineConfig config(1);
    subobjects[index++] = config.subobject; // 7

    // Create the global root signature and store the empty signature
    GlobalRootSignature root(device, CreateGlobalRootDesc().desc);
    mGlobalRootSig = root.pRootSig;
    subobjects[index++] = root.subobject; // 8

    // Create the state
    D3D12_STATE_OBJECT_DESC desc;
    desc.NumSubobjects = index; // 9
    desc.pSubobjects = subobjects.data();
    desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

    device->CreateStateObject(&desc, IID_PPV_ARGS(&mPipelineState));
}

void Pipeline::CreateShaderTable(ID3D12Device5* device, ID3D12GraphicsCommandList4* cmdList, ComPtr<D3D12MA::Allocator> alloc,
    ResourceStateTracker& tracker, AssetManager& assetMgr)
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
    uint32_t shaderTableSize = mShaderTableEntrySize * 5;

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

    const std::vector<std::shared_ptr<Instance>> instances = assetMgr.GetInstances();
    // Entries 2-4 - The triangles' hit program. ProgramID and constant-buffer data
    for (uint32_t i = 0; i < instances.size(); i++)
    {
        uint8_t* pHitEntry = pData + mShaderTableEntrySize * (i + 2); // +2 skips the ray-gen and miss entries
        memcpy(pHitEntry, pRtsoProps->GetShaderIdentifier(kHitGroup), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
        uint8_t* pCbDesc = pHitEntry + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;            // The location of the root-descriptor
        assert(((uint64_t)pCbDesc % 8) == 0); // Root descriptor must be stored at an 8-byte aligned address

        *(D3D12_GPU_VIRTUAL_ADDRESS*)pCbDesc = instances[i]->GetInstanceCB()->GetGPUVirtualAddress(0);
    }

    // Unmap
    mShaderTable->GetResource()->Unmap(0, nullptr);
}