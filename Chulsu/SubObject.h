#pragma once

namespace SubObject
{
    struct RootSignatureDesc
    {
        D3D12_ROOT_SIGNATURE_DESC desc = {};
        std::vector<D3D12_DESCRIPTOR_RANGE> range;
        std::vector<D3D12_ROOT_PARAMETER> rootParams;
    };

    RootSignatureDesc CreateGlobalRootDesc()
    {
        // Create the root-signature
        RootSignatureDesc desc;
        desc.rootParams.resize(2);

        desc.rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        desc.rootParams[0].Constants.RegisterSpace = 0;
        desc.rootParams[0].Constants.ShaderRegister = 0;
        desc.rootParams[0].Constants.Num32BitValues = 1;

        desc.rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        desc.rootParams[1].Descriptor.RegisterSpace = 0;
        desc.rootParams[1].Descriptor.ShaderRegister = 0;

        // Create the desc
        desc.desc.NumParameters = desc.rootParams.size();
        desc.desc.pParameters = desc.rootParams.data();
        desc.desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;

        return desc;
    }

    RootSignatureDesc CreateHitRootDesc()
    {
        RootSignatureDesc desc;
        desc.rootParams.resize(1);
        desc.rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        desc.rootParams[0].Constants.RegisterSpace = 0;
        desc.rootParams[0].Constants.ShaderRegister = 1;

        desc.desc.NumParameters = 1;
        desc.desc.pParameters = desc.rootParams.data();
        desc.desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

        return desc;
    }

    struct DxilLibrary
    {
        DxilLibrary(ComPtr<IDxcBlob> pBlob, const WCHAR* entryPoint[], uint32_t entryPointCount) : pShaderBlob(pBlob)
        {
            stateSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
            stateSubobject.pDesc = &dxilLibDesc;

            dxilLibDesc = {};
            exportDesc.resize(entryPointCount);
            exportName.resize(entryPointCount);
            if (pBlob)
            {
                dxilLibDesc.DXILLibrary.pShaderBytecode = pBlob->GetBufferPointer();
                dxilLibDesc.DXILLibrary.BytecodeLength = pBlob->GetBufferSize();
                dxilLibDesc.NumExports = entryPointCount;
                dxilLibDesc.pExports = exportDesc.data();

                for (uint32_t i = 0; i < entryPointCount; i++)
                {
                    exportName[i] = entryPoint[i];
                    exportDesc[i].Name = exportName[i].c_str();
                    exportDesc[i].Flags = D3D12_EXPORT_FLAG_NONE;
                    exportDesc[i].ExportToRename = nullptr;
                }
            }
        };

        DxilLibrary() : DxilLibrary(nullptr, nullptr, 0) {}

        D3D12_DXIL_LIBRARY_DESC dxilLibDesc = {};
        D3D12_STATE_SUBOBJECT stateSubobject{};
        ComPtr<IDxcBlob> pShaderBlob;
        std::vector<D3D12_EXPORT_DESC> exportDesc;
        std::vector<std::wstring> exportName;
    };

    static const WCHAR* kRayGenShader = L"RayGen";

    static const WCHAR* kMissShader = L"Miss";
    static const WCHAR* kClosestHitShader = L"ClosestHit";
    static const WCHAR* kHitGroup = L"HitGroup";

    static const WCHAR* kShadowMissShader = L"ShadowMiss";
    static const WCHAR* kShadowClosestHitShader = L"ShadowClosestHit";
    static const WCHAR* kShadowHitGroup = L"ShadowHitGroup";

    ComPtr<IDxcBlob> CompileLibrary(const WCHAR* filename, const WCHAR* targetString)
    {
        // Initialize the helper
        ThrowIfFailed(gDxcDllSupport.Initialize());
        IDxcCompiler* compiler;
        IDxcLibrary* library;

        ThrowIfFailed(gDxcDllSupport.CreateInstance(CLSID_DxcCompiler, &compiler));
        ThrowIfFailed(gDxcDllSupport.CreateInstance(CLSID_DxcLibrary, &library));

        // Open and read the file
        std::ifstream shaderFile(filename);
        if (shaderFile.good() == false)
        {
            return nullptr;
        }
        std::stringstream strStream;
        strStream << shaderFile.rdbuf();
        std::string shader = strStream.str();

        // Create blob from the string
        ComPtr<IDxcBlobEncoding> textBlob;
        ThrowIfFailed(library->CreateBlobWithEncodingFromPinned((LPBYTE)shader.c_str(), (uint32_t)shader.size(), 0, &textBlob));

        // Compile
        ComPtr<IDxcOperationResult> result;
        compiler->Compile(textBlob.Get(), filename, L"", targetString, nullptr, 0, nullptr, 0, nullptr, &result);

        compiler->Release();
        library->Release();

        // Verify the result
        HRESULT resultCode;
        result->GetStatus(&resultCode);
        if (FAILED(resultCode))
        {
            ComPtr<IDxcBlobEncoding> pError;
            result->GetErrorBuffer(&pError);
            std::string log = ConvertBlobToString(pError.Get());
            return nullptr;
        }

        ComPtr<IDxcBlob> blob;
        result->GetResult(&blob);

        return blob;
    }

    DxilLibrary CreateDxilLibrary(const WCHAR* filename, const WCHAR* entryPoints[], uint32_t arraySize)
    {
        // Compile the shader
        ComPtr<IDxcBlob> pDxilLib = CompileLibrary(filename, L"lib_6_6");
        return DxilLibrary(pDxilLib, entryPoints, arraySize);
    }


    struct HitProgram
    {
        HitProgram(LPCWSTR ahsExport, LPCWSTR chsExport, const std::wstring& name) : exportName(name)
        {
            desc = {};
            desc.AnyHitShaderImport = ahsExport;
            desc.ClosestHitShaderImport = chsExport;
            desc.HitGroupExport = exportName.c_str();

            subObject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
            subObject.pDesc = &desc;
        }

        std::wstring exportName;
        D3D12_HIT_GROUP_DESC desc;
        D3D12_STATE_SUBOBJECT subObject;
    };

    struct ExportAssociation
    {
        ExportAssociation(const WCHAR* exportNames[], uint32_t exportCount, const D3D12_STATE_SUBOBJECT* pSubobjectToAssociate)
        {
            association.NumExports = exportCount;
            association.pExports = exportNames;
            association.pSubobjectToAssociate = pSubobjectToAssociate;

            subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
            subobject.pDesc = &association;
        }

        D3D12_STATE_SUBOBJECT subobject = {};
        D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION association = {};
    };

    ComPtr<ID3D12RootSignature> CreateRootSignature(ComPtr<ID3D12Device5> pDevice, const D3D12_ROOT_SIGNATURE_DESC& desc)
    {
        ComPtr<ID3DBlob> pSigBlob;
        ComPtr<ID3DBlob> pErrorBlob;
        HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &pSigBlob, &pErrorBlob);
        if (FAILED(hr))
        {
            std::string msg = ConvertBlobToString(pErrorBlob.Get());
            return nullptr;
        }
        ComPtr<ID3D12RootSignature> pRootSig;
        pDevice->CreateRootSignature(0, pSigBlob->GetBufferPointer(), pSigBlob->GetBufferSize(), IID_PPV_ARGS(&pRootSig));
        return pRootSig;
    }

    struct LocalRootSignature
    {
        LocalRootSignature(ComPtr<ID3D12Device5> pDevice, const D3D12_ROOT_SIGNATURE_DESC& desc)
        {
            pRootSig = CreateRootSignature(pDevice, desc);
            pInterface = pRootSig.Get();
            subobject.pDesc = &pInterface;
            subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
        }
        ComPtr<ID3D12RootSignature> pRootSig;
        ID3D12RootSignature* pInterface = nullptr;
        D3D12_STATE_SUBOBJECT subobject = {};
    };

    struct GlobalRootSignature
    {
        GlobalRootSignature(ComPtr<ID3D12Device5> pDevice, const D3D12_ROOT_SIGNATURE_DESC& desc)
        {
            pRootSig = CreateRootSignature(pDevice, desc);
            pInterface = pRootSig.Get();
            subobject.pDesc = &pInterface;
            subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
        }
        ComPtr<ID3D12RootSignature> pRootSig;
        ID3D12RootSignature* pInterface = nullptr;
        D3D12_STATE_SUBOBJECT subobject = {};
    };

    struct ShaderConfig
    {
        ShaderConfig(uint32_t maxAttributeSizeInBytes, uint32_t maxPayloadSizeInBytes)
        {
            shaderConfig.MaxAttributeSizeInBytes = maxAttributeSizeInBytes;
            shaderConfig.MaxPayloadSizeInBytes = maxPayloadSizeInBytes;

            subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
            subobject.pDesc = &shaderConfig;
        }

        D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
        D3D12_STATE_SUBOBJECT subobject = {};
    };

    struct PipelineConfig
    {
        PipelineConfig(uint32_t maxTraceRecursionDepth)
        {
            config.MaxTraceRecursionDepth = maxTraceRecursionDepth;

            subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
            subobject.pDesc = &config;
        }

        D3D12_RAYTRACING_PIPELINE_CONFIG config = {};
        D3D12_STATE_SUBOBJECT subobject = {};
    };
}
