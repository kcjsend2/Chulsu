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
        D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&pDevice));

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

    mAssetMgr.Init(mDevice.Get(), 2048);
    mAssetMgr.mCbvSrvUavDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void DX12Renderer::BuildObjects()
{
    mCamera.SetLens(0.25f * PI, mSwapChainSize.x / mSwapChainSize.y, 1.0f, 20000.0f);
    mCamera.LookAt(XMFLOAT3(0.0f, 100.0f, 0.0f), XMFLOAT3(0.0f, 100.0f, 150.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));

    mAssetMgr.CreateInstance(mDevice.Get(), mCmdList.Get(), mMemAllocator.Get(), mResourceTracker, "Contents/Sponza/Sponza.fbx", XMFLOAT3(), XMFLOAT3(), XMFLOAT3(1, 1, 1));
    
    mAssetMgr.BuildAccelerationStructure(mDevice.Get(), mCmdList.Get(), mMemAllocator, mResourceTracker);

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

LRESULT DX12Renderer::OnProcessMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_ACTIVATE:
        break;

    case WM_SIZE:
        mSwapChainSize.x = LOWORD(lParam);
        mSwapChainSize.y = HIWORD(lParam);
        if (wParam == SIZE_MAXIMIZED)
        {
            OnResize();
        }
        else if (wParam == SIZE_RESTORED)
        {
            if (mDevice) {
                OnResize();
            }
        }
        break;

    case WM_ENTERSIZEMOVE:
        break;

    case WM_EXITSIZEMOVE:
        OnResize();
        break;

    case WM_GETMINMAXINFO:
        reinterpret_cast<MINMAXINFO*>(lParam)->ptMinTrackSize = { 200, 200 };
        break;

    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
        OnProcessMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        break;

    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
        OnProcessMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        break;

    case WM_MOUSEMOVE:
        OnProcessMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        break;

    case WM_KEYDOWN:
    case WM_KEYUP:
        if (wParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
            return 0;
        }
        OnProcessKeyInput(msg, wParam, lParam);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void DX12Renderer::OnResize()
{
}

void DX12Renderer::OnProcessMouseDown(WPARAM buttonState, int x, int y)
{
    if ((buttonState) && !GetCapture())
    {
        SetCapture(mWinHandle);
        mLastMousePos.x = x;
        mLastMousePos.y = y;
    }
}

void DX12Renderer::OnProcessMouseUp(WPARAM buttonState, int x, int y)
{
    ReleaseCapture();
}

void DX12Renderer::OnProcessMouseMove(WPARAM buttonState, int x, int y)
{
    if ((buttonState & MK_LBUTTON) && GetCapture())
    {
        float dx = static_cast<float>(x - mLastMousePos.x);
        float dy = static_cast<float>(y - mLastMousePos.y);

        mLastMousePos.x = x;
        mLastMousePos.y = y;

        mCamera.Pitch(0.25f * dy);
        mCamera.RotateY(0.25f * dx);
    }
}

void DX12Renderer::OnProcessKeyInput(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_KEYUP:
        switch (wParam)
        {
        case VK_ESCAPE:
            PostQuitMessage(0);
            return;
            break;
        }
        break;
    }
}

void DX12Renderer::OnPreciseKeyInput()
{
    float dist = 500.0f;

    if (GetAsyncKeyState(VK_LSHIFT) & 0x8000)
        dist = 1000.0f;

    if (GetAsyncKeyState('A') & 0x8000)
        mCamera.Strafe(-dist * mDeltaTime);

    if (GetAsyncKeyState('D') & 0x8000)
        mCamera.Strafe(dist * mDeltaTime);

    if (GetAsyncKeyState('W') & 0x8000)
        mCamera.Walk(dist * mDeltaTime);

    if (GetAsyncKeyState('S') & 0x8000)
        mCamera.Walk(-dist * mDeltaTime);

    if (GetAsyncKeyState(VK_SPACE) & 0x8000)
        mCamera.Upward(dist * mDeltaTime);

    if (GetAsyncKeyState(VK_LCONTROL) & 0x8000)
        mCamera.Upward(-dist * mDeltaTime);


    if (GetAsyncKeyState('Q') & 0x8000)
        mShadowDirection = Vector3::Transform(mShadowDirection, XMMatrixRotationX(0.001));

    if (GetAsyncKeyState('E') & 0x8000)
        mShadowDirection = Vector3::Transform(mShadowDirection, XMMatrixRotationX(-0.001));
}

void DX12Renderer::Update()
{
    OnPreciseKeyInput();
    mCamera.Update(mDeltaTime);
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
    raytraceDesc.MissShaderTable.StartAddress = shaderTable->GetGPUVirtualAddress() + missOffset;
    raytraceDesc.MissShaderTable.StrideInBytes = entrySize;
    raytraceDesc.MissShaderTable.SizeInBytes = entrySize * 2;

    // Hit is the third entry in the shader-table
    size_t hitOffset = 3 * entrySize;
    raytraceDesc.HitGroupTable.StartAddress = shaderTable->GetGPUVirtualAddress() + hitOffset;
    raytraceDesc.HitGroupTable.StrideInBytes = entrySize;
    raytraceDesc.HitGroupTable.SizeInBytes = entrySize * mAssetMgr.GetInstances().size() * 2;

    mCmdList->SetComputeRootSignature(mPipelines["RayTracing"].GetGlobalRootSignature().Get());

    mCmdList->SetComputeRoot32BitConstant(0, mOutputTextureIndex, 0);

    auto mat = Matrix4x4::Transpose(Matrix4x4::Inverse(Matrix4x4::Multiply(mCamera.GetView(), mCamera.GetProj())));
    mCmdList->SetComputeRoot32BitConstants(0, 16, &mat, 4);

    mCmdList->SetComputeRoot32BitConstants(0, 3, &mCamera.GetPosition(), 20);
    mCmdList->SetComputeRoot32BitConstants(0, 3, &mShadowDirection, 24);

    mCmdList->SetComputeRootShaderResourceView(1, mAssetMgr.GetTLAS().mResult->GetResource()->GetGPUVirtualAddress());

    // Dispatch
    mCmdList->SetPipelineState1(mPipelines["RayTracing"].GetStateObject().Get());

    if(raytraceDesc.Width > 0 && raytraceDesc.Height > 0)
        mCmdList->DispatchRays(&raytraceDesc);

    mResourceTracker.TransitionBarrier(mCmdList, mOutputTexture->GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE);
    mResourceTracker.TransitionBarrier(mCmdList, mFrameObjects[rtvIndex].pSwapChainBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST);

    mCmdList->CopyResource(mFrameObjects[rtvIndex].pSwapChainBuffer.Get(), mOutputTexture->GetResource());

    mResourceTracker.TransitionBarrier(mCmdList, mFrameObjects[rtvIndex].pSwapChainBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT);

    mCmdList->Close();
    ID3D12CommandList* cmdList[] = { mCmdList.Get() };
    mCmdQueue->ExecuteCommandLists(_countof(cmdList), cmdList);

    WaitUntilGPUComplete();

    mSwapChain->Present(0, 0);

    // Prepare the command list for the next frame
    uint32_t bufferIndex = mSwapChain->GetCurrentBackBufferIndex();


    ThrowIfFailed(mDevice->GetDeviceRemovedReason());

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