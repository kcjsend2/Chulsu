#include "Pipeline.h"

void Pipeline::CreatePipelineState()
{
}

void Pipeline::CreateDXILLibrary()
{
    // Compile the shader
    ID3DBlobPtr pDxilLib = compileLibrary(L"Data/06-Shaders.hlsl", L"lib_6_3");
    const WCHAR* entryPoints[] = { kRayGenShader, kMissShader, kClosestHitShader };
    return DxilLibrary(pDxilLib, entryPoints, arraysize(entryPoints));
}
