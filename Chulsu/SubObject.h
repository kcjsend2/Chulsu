#pragma once

namespace SubObject
{
    struct RootSignatureDesc
    {
        D3D12_ROOT_SIGNATURE_DESC desc = {};
        std::vector<D3D12_DESCRIPTOR_RANGE> range;
        std::vector<D3D12_ROOT_PARAMETER> rootParams;
    };

    RootSignatureDesc CreateRayGenRootDesc()
    {
        // Create the root-signature
        RootSignatureDesc desc;
        desc.range.resize(2);

        // gRtScene
        desc.range[0].BaseShaderRegister = 0;
        desc.range[0].NumDescriptors = 1;
        desc.range[0].RegisterSpace = 0;
        desc.range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        desc.range[0].OffsetInDescriptorsFromTableStart = 0;

        // gOutput
        desc.range[1].BaseShaderRegister = 0;
        desc.range[1].NumDescriptors = 1;
        desc.range[1].RegisterSpace = 0;
        desc.range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        desc.range[1].OffsetInDescriptorsFromTableStart = 0;
        
        desc.rootParams.resize(1);
        desc.rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        desc.rootParams[0].DescriptorTable.NumDescriptorRanges = UINT(desc.range.size());
        desc.rootParams[0].DescriptorTable.pDescriptorRanges = desc.range.data();

        // Create the desc
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

    static const WCHAR* kRayGenShader = L"rayGen";
    static const WCHAR* kMissShader = L"miss";
    static const WCHAR* kClosestHitShader = L"chs";
    static const WCHAR* kHitGroup = L"HitGroup";

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

    DxilLibrary CreateDxilLibrary(const WCHAR* filename)
    {
        // Compile the shader
        ComPtr<IDxcBlob> pDxilLib = CompileLibrary(filename, L"lib_6_6");
        const WCHAR* entryPoints[] = { kRayGenShader, kMissShader, kClosestHitShader };
        return DxilLibrary(pDxilLib, entryPoints, arraysize(entryPoints));
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
