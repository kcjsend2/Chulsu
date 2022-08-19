#include "DX12Renderer.h"
#include "Pipeline.h"

DX12Renderer::DX12Renderer()
{
}

DX12Renderer::~DX12Renderer()
{
    if (mDevice) WaitUntilGPUComplete();
    if (mFenceEvent) CloseHandle(mFenceEvent);
}

ComPtr<IDXGISwapChain3> DX12Renderer::CreateDxgiSwapChain(ComPtr<IDXGIFactory4> pFactory, HWND hwnd, uint32_t width, uint32_t height, DXGI_FORMAT format, ComPtr<ID3D12CommandQueue> pCommandQueue)
{
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = mSwapChainBufferCount;
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = format;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> pSwapChain;
    ThrowIfFailed(pFactory->CreateSwapChainForHwnd(pCommandQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, &pSwapChain));

    ComPtr<IDXGISwapChain3> pSwapChain3;
    pSwapChain->QueryInterface(IID_PPV_ARGS(&pSwapChain3));
    return pSwapChain3;
}

ComPtr<ID3D12Device5> DX12Renderer::CreateDevice(ComPtr<IDXGIFactory4> pDxgiFactory)
{    
    // Find the HW adapter
    ComPtr<IDXGIAdapter1> pAdapter;

    for (uint32_t i = 0; DXGI_ERROR_NOT_FOUND != pDxgiFactory->EnumAdapters1(i, &pAdapter); i++)
    {
        DXGI_ADAPTER_DESC1 desc;
        pAdapter->GetDesc1(&desc);

        // Skip SW adapters
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
#ifdef _DEBUG
        ComPtr<ID3D12Debug> pDx12Debug;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pDx12Debug))))
        {
            pDx12Debug->EnableDebugLayer();
        }
#endif
        // Create the device
        ComPtr<ID3D12Device> pDevice;
        D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&pDevice));

        D3D12_FEATURE_DATA_D3D12_OPTIONS5 features5;
        HRESULT hr = pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &features5, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));
        if (SUCCEEDED(hr) && features5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
        {
            D3D12MA::ALLOCATOR_DESC allocatorDesc = {};

            allocatorDesc.pDevice = pDevice.Get();

            IDXGIAdapter* allocatorAdapter;
            pAdapter->QueryInterface(IID_PPV_ARGS(&allocatorAdapter));
            allocatorDesc.pAdapter = allocatorAdapter;

            ThrowIfFailed(D3D12MA::CreateAllocator(&allocatorDesc, &mMemAllocator));

            ComPtr<ID3D12Device5> device5;
            pDevice->QueryInterface(IID_PPV_ARGS(&device5));

            return device5;
        }
    }

    exit(1);
    return nullptr;
}

ComPtr<ID3D12CommandQueue> DX12Renderer::CreateCommandQueue(ComPtr<ID3D12Device5> pDevice)
{
    ComPtr<ID3D12CommandQueue> pQueue;
    D3D12_COMMAND_QUEUE_DESC cqDesc = {};
    cqDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    cqDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    pDevice->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&pQueue));
    return pQueue;
}

ComPtr<ID3D12DescriptorHeap> DX12Renderer::CreateSwapchainDescriptorHeap(ComPtr<ID3D12Device5> pDevice, uint32_t count, D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisible)
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = count;
    desc.Type = type;
    desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    ComPtr<ID3D12DescriptorHeap> pHeap;
    pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pHeap));
    return pHeap;
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12Renderer::CreateSwapchainRTV(ComPtr<ID3D12Device5> pDevice, ComPtr<ID3D12Resource> pResource, ComPtr<ID3D12DescriptorHeap> pHeap, uint32_t& usedHeapEntries, DXGI_FORMAT format)
{
    D3D12_RENDER_TARGET_VIEW_DESC desc = {};
    desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    desc.Format = format;
    desc.Texture2D.MipSlice = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = pHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += usedHeapEntries * pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    usedHeapEntries++;
    pDevice->CreateRenderTargetView(pResource.Get(), &desc, rtvHandle);
    return rtvHandle;
}


void DX12Renderer::Init(HWND winHandle, uint32_t winWidth, uint32_t winHeight)
{
    mWinHandle = winHandle;
    mSwapChainSize = XMFLOAT2(winWidth, winHeight);

    // Initialize the debug layer for debug builds
#ifdef _DEBUG
    ComPtr<ID3D12Debug5> pDebug;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebug))))
    {
        pDebug->EnableDebugLayer();
        pDebug->SetEnableAutoName(true);
    }
#endif

    // Create the DXGI factory
    ComPtr<IDXGIFactory4> pDxgiFactory;
    CreateDXGIFactory1(IID_PPV_ARGS(&pDxgiFactory));
    mDevice = CreateDevice(pDxgiFactory);
    mCmdQueue = CreateCommandQueue(mDevice);
    mSwapChain = CreateDxgiSwapChain(pDxgiFactory, mWinHandle, winWidth, winHeight, DXGI_FORMAT_R8G8B8A8_UNORM, mCmdQueue);

    // Create a RTV descriptor heap
    mRtvHeap.pHeap = CreateSwapchainDescriptorHeap(mDevice, mRtvHeapSize, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false);

    // Create the per-frame objects
    for (uint32_t i = 0; i < mSwapChainBufferCount; i++)
    {
        mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mFrameObjects[i].pCommandAllocator));
        mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mFrameObjects[i].pSwapChainBuffer));
        mFrameObjects[i].rtvHandle = CreateSwapchainRTV(mDevice, mFrameObjects[i].pSwapChainBuffer, mRtvHeap.pHeap, mRtvHeap.usedEntries, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
    }

    mResourceTracker.AddTrackingResource(mFrameObjects[0].pSwapChainBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT);
    // Create the command-list
    mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mFrameObjects[0].pCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&mCmdList));

    // Create a fence and the event
    mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
    mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    mAssetMgr.Init(mDevice.Get(), 256);
    mAssetMgr.mCbvSrvUavDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void DX12Renderer::BuildObjects()
{
    mAssetMgr.LoadTestTriangleInstance(mDevice.Get(), mCmdList.Get(), mMemAllocator, mResourceTracker, mAssetMgr);
    mAssetMgr.BuildAccelerationStructure(mDevice.Get(), mCmdList.Get(), mMemAllocator, mResourceTracker);

    mASIndex = mAssetMgr.GetCurrentHeapIndex() - 1;

    mOutputTexture = mAssetMgr.CreateResource(mDevice.Get(), mCmdList.Get(), mMemAllocator, mResourceTracker,
        NULL, mSwapChainSize.x, mSwapChainSize.y,
        D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    mAssetMgr.SetTexture(mDevice.Get(), mCmdList.Get(), mOutputTexture,
        L"OutputResource", {}, D3D12_UAV_DIMENSION_TEXTURE2D, false, true);

    mOutputTextureIndex = mAssetMgr.GetCurrentHeapIndex() - 1;

    Pipeline pipeline;
    pipeline.CreatePipelineState(mDevice, L"Shaders/DefaultRayTrace.hlsl");
    pipeline.CreateShaderTable(mDevice.Get(), mCmdList.Get(), mMemAllocator, mResourceTracker, mAssetMgr);
    mPipelines["RayTracing"] = pipeline;
}

void DX12Renderer::Draw()
{
    const float clearColor[4] = { 0.4f, 0.6f, 0.2f, 1.0f };
    auto rtvIndex = mSwapChain->GetCurrentBackBufferIndex();

    mResourceTracker.TransitionBarrier(mCmdList, mFrameObjects[rtvIndex].pSwapChainBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
    mCmdList->ClearRenderTargetView(mFrameObjects[rtvIndex].rtvHandle, clearColor, 0, nullptr);
    mResourceTracker.TransitionBarrier(mCmdList, mFrameObjects[rtvIndex].pSwapChainBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT);

    ID3D12DescriptorHeap* heaps[] = { mAssetMgr.GetDescriptorHeap().Get() };
    mCmdList->SetDescriptorHeaps(arraysize(heaps), heaps);

    mResourceTracker.TransitionBarrier(mCmdList, mOutputTexture->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    D3D12_DISPATCH_RAYS_DESC raytraceDesc = {};
    raytraceDesc.Width = mSwapChainSize.x;
    raytraceDesc.Height = mSwapChainSize.y;
    raytraceDesc.Depth = 1;

    UINT entrySize = mPipelines["RayTracing"].GetShaderTableEntrySize();
    auto shaderTable = mPipelines["RayTracing"].GetShaderTable()->GetResource();

    // RayGen is the first entry in the shader-table
    raytraceDesc.RayGenerationShaderRecord.StartAddress = shaderTable->GetGPUVirtualAddress() + 0 * entrySize;
    raytraceDesc.RayGenerationShaderRecord.SizeInBytes = entrySize;

    // Miss is the second entry in the shader-table
    size_t missOffset = 1 * entrySize;
    raytraceDesc.MissShaderTable.StartAddress = mPipelines["RayTracing"].GetShaderTable()->GetResource()->GetGPUVirtualAddress() + missOffset;
    raytraceDesc.MissShaderTable.StrideInBytes = entrySize;
    raytraceDesc.MissShaderTable.SizeInBytes = entrySize;

    // Hit is the third entry in the shader-table
    size_t hitOffset = 2 * entrySize;
    raytraceDesc.HitGroupTable.StartAddress = shaderTable->GetGPUVirtualAddress() + hitOffset;
    raytraceDesc.HitGroupTable.StrideInBytes = entrySize;
    raytraceDesc.HitGroupTable.SizeInBytes = entrySize * 3;

    mCmdList->SetComputeRootSignature(mPipelines["RayTracing"].GetGlobalRootSignature().Get());

    mCmdList->SetComputeRoot32BitConstant(0, mASIndex, 0);
    mCmdList->SetComputeRoot32BitConstant(0, mOutputTextureIndex, 1);

    // Dispatch
    mCmdList->SetPipelineState1(mPipelines["RayTracing"].GetStateObject().Get());
    mCmdList->DispatchRays(&raytraceDesc);

    mResourceTracker.TransitionBarrier(mCmdList, mOutputTexture->GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE);
    mResourceTracker.TransitionBarrier(mCmdList, mFrameObjects[rtvIndex].pSwapChainBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST);

    mCmdList->CopyResource(mFrameObjects[rtvIndex].pSwapChainBuffer.Get(), mOutputTexture->GetResource());

    mResourceTracker.TransitionBarrier(mCmdList, mFrameObjects[rtvIndex].pSwapChainBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT);

    mCmdList->Close();
    ID3D12CommandList* cmdList[] = { mCmdList.Get() };
    mCmdQueue->ExecuteCommandLists(_countof(cmdList), cmdList);
    mFenceValue++;
    mCmdQueue->Signal(mFence.Get(), mFenceValue);

    mSwapChain->Present(0, 0);

    // Prepare the command list for the next frame
    uint32_t bufferIndex = mSwapChain->GetCurrentBackBufferIndex();

    WaitUntilGPUComplete();

    //ThrowIfFailed(mDevice->GetDeviceRemovedReason());

    mFrameObjects[bufferIndex].pCommandAllocator->Reset();
    mCmdList->Reset(mFrameObjects[bufferIndex].pCommandAllocator.Get(), nullptr);

    mAssetMgr.FreeUploadBuffers();
}

void DX12Renderer::WaitUntilGPUComplete()
{
	UINT64 currFenceValue = ++mFenceValue;
	ThrowIfFailed(mCmdQueue->Signal(mFence.Get(), currFenceValue));
	if (mFence->GetCompletedValue() < currFenceValue)
	{
		ThrowIfFailed(mFence->SetEventOnCompletion(currFenceValue, mFenceEvent));
		WaitForSingleObject(mFenceEvent, INFINITE);
	}
}