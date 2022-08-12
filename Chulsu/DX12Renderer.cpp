#include "DX12Renderer.h"

DX12Renderer::DX12Renderer()
{
}

DX12Renderer::~DX12Renderer()
{
    if (mD3dDevice) WaitUntilGPUComplete();
    if (mFenceEvent) CloseHandle(mFenceEvent);
    mMemAllocator->Release();
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
        ComPtr<ID3D12Device5> pDevice;
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

            return pDevice;
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

ComPtr<ID3D12DescriptorHeap> DX12Renderer::CreateDescriptorHeap(ComPtr<ID3D12Device5> pDevice, uint32_t count, D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisible)
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = count;
    desc.Type = type;
    desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    ComPtr<ID3D12DescriptorHeap> pHeap;
    pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pHeap));
    return pHeap;
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12Renderer::CreateRTV(ComPtr<ID3D12Device5> pDevice, ComPtr<ID3D12Resource> pResource, ComPtr<ID3D12DescriptorHeap> pHeap, uint32_t& usedHeapEntries, DXGI_FORMAT format)
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
    ComPtr<ID3D12Debug> pDebug;
    ComPtr<ID3D12Debug5> pDebug5;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebug))))
    {
        pDebug->QueryInterface(IID_PPV_ARGS(&pDebug5));
        pDebug5->EnableDebugLayer();
        pDebug5->SetEnableAutoName(true);
    }
#endif

    // Create the DXGI factory
    ComPtr<IDXGIFactory4> pDxgiFactory;
    CreateDXGIFactory1(IID_PPV_ARGS(&pDxgiFactory));
    mD3dDevice = CreateDevice(pDxgiFactory);
    mCmdQueue = CreateCommandQueue(mD3dDevice);
    mSwapChain = CreateDxgiSwapChain(pDxgiFactory, mWinHandle, winWidth, winHeight, DXGI_FORMAT_R8G8B8A8_UNORM, mCmdQueue);

    // Create a RTV descriptor heap
    mRtvHeap.pHeap = CreateDescriptorHeap(mD3dDevice, mRtvHeapSize, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false);

    // Create the per-frame objects
    for (uint32_t i = 0; i < mSwapChainBufferCount; i++)
    {
        mD3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mFrameObjects[i].pCommandAllocator));
        mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mFrameObjects[i].pSwapChainBuffer));
        mFrameObjects[i].rtvHandle = CreateRTV(mD3dDevice, mFrameObjects[i].pSwapChainBuffer, mRtvHeap.pHeap, mRtvHeap.usedEntries, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
    }

    mResourceTracker.AddTrackingResource(mFrameObjects[0].pSwapChainBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT);
    // Create the command-list
    mD3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mFrameObjects[0].pCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&mCmdList));

    // Create a fence and the event
    mD3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
    mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    mAssetMgr = make_shared<AssetManager>(mD3dDevice.Get(), 256);
    mAssetMgr->mCbvSrvUavDescriptorSize = mD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    mAssetMgr->LoadTestTriangleModel(mD3dDevice.Get(), mCmdList.Get(), mMemAllocator, mResourceTracker);
    mAssetMgr->BuildAcceleerationStructure(mD3dDevice.Get(), mCmdList.Get(), mMemAllocator, mResourceTracker);
}


void DX12Renderer::Draw()
{
    const float clearColor[4] = { 0.4f, 0.6f, 0.2f, 1.0f };

    auto rtvIndex = mSwapChain->GetCurrentBackBufferIndex();
    mResourceTracker.TransitionBarrier(mCmdList, mFrameObjects[rtvIndex].pSwapChainBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
    mCmdList->ClearRenderTargetView(mFrameObjects[rtvIndex].rtvHandle, clearColor, 0, nullptr);

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

    mFrameObjects[bufferIndex].pCommandAllocator->Reset();
    mCmdList->Reset(mFrameObjects[bufferIndex].pCommandAllocator.Get(), nullptr);

    mAssetMgr->FreeUploadBuffers();
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