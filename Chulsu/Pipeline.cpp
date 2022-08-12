#include "Pipeline.h"
#include "SubObject.h"

void Pipeline::CreatePipelineState(ComPtr<ID3D12Device5> device, const WCHAR* filename)
{
    using namespace SubObject;

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
    ShaderConfig shaderConfig(sizeof(float) * 2, sizeof(float) * 1);
    subobjects[index] = shaderConfig.subobject; // 6 Shader Config

    uint32_t shaderConfigIndex = index++; // 6
    const WCHAR* shaderExports[] = { kMissShader, kClosestHitShader, kRayGenShader };
    ExportAssociation configAssociation(shaderExports, arraysize(shaderExports), &(subobjects[shaderConfigIndex]));
    subobjects[index++] = configAssociation.subobject; // 7 Associate Shader Config to Miss, CHS, RGS

    // Create the pipeline config
    PipelineConfig config(0);
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

    device->CreateStateObject(&desc, IID_PPV_ARGS(&mPipelineState));
}
