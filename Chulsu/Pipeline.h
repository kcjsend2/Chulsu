#pragma once
#include "stdafx.h"
#include "Shader.h"

namespace SubObject
{
    struct DxilLibrary
    {
        DxilLibrary(ComPtr<ID3DBlob> pBlob, const WCHAR* entryPoint[], uint32_t entryPointCount) : pShaderBlob(pBlob)
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
        ComPtr<ID3DBlob> pShaderBlob;
        std::vector<D3D12_EXPORT_DESC> exportDesc;
        std::vector<std::wstring> exportName;
    };

    static const WCHAR* kRayGenShader = L"rayGen";
    static const WCHAR* kMissShader = L"miss";
    static const WCHAR* kClosestHitShader = L"chs";
    static const WCHAR* kHitGroup = L"HitGroup";

    ComPtr<ID3DBlob> CompileLibrary(const WCHAR* filename, const WCHAR* targetString)
    {
        // Initialize the helper
        gDxcDllSupport.Initialize();
        IDxcCompiler* compiler;
        IDxcLibrary* library;

        gDxcDllSupport.CreateInstance(CLSID_DxcCompiler, &compiler);
        gDxcDllSupport.CreateInstance(CLSID_DxcLibrary, &library);

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
        library->CreateBlobWithEncodingFromPinned((LPBYTE)shader.c_str(), (uint32_t)shader.size(), 0, &textBlob);

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

        ComPtr<ID3DBlob> d3dBlob = blob.Get();
        return d3dBlob;
    }

    DxilLibrary CreateDxilLibrary()
    {
        // Compile the shader
        ComPtr<ID3DBlob> pDxilLib = CompileLibrary(L"Data/06-Shaders.hlsl", L"lib_6_3");
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

}

class Pipeline
{
public:
	void CreatePipelineState();
	void CreateDXILLibrary();

protected:
	vector<D3D12_STATE_SUBOBJECT> mSubObjects;
	ComPtr<ID3D12StateObject> mPipelineState;
};