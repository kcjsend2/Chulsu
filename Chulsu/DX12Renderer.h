#pragma once
#include "Renderer.h"
#include "ResourceStateTracker.h"

class DX12Renderer : public Renderer
{
public:
	DX12Renderer() {}
	~DX12Renderer();

	virtual void Init(HWND winHandle, uint32_t winWidth, uint32_t winHeight) override;
	virtual void Draw() override;

private:
	void WaitUntilGPUComplete();

	ComPtr<IDXGISwapChain3> CreateDxgiSwapChain(ComPtr<IDXGIFactory4> pFactory, HWND hwnd, uint32_t width, uint32_t height, DXGI_FORMAT format, ComPtr<ID3D12CommandQueue> pCommandQueue);
	ComPtr<ID3D12Device5> CreateDevice(ComPtr<IDXGIFactory4> pDxgiFactory);
	ComPtr<ID3D12CommandQueue> CreateCommandQueue(ComPtr<ID3D12Device5> pDevice);
	ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device5> pDevice, uint32_t count, D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisible);
	D3D12_CPU_DESCRIPTOR_HANDLE CreateRTV(ComPtr<ID3D12Device5> pDevice, ComPtr<ID3D12Resource> pResource, ComPtr<ID3D12DescriptorHeap> pHeap, uint32_t& usedHeapEntries, DXGI_FORMAT format);

	ComPtr<ID3D12Device5> mD3dDevice;
	ComPtr<ID3D12GraphicsCommandList4> mD3dCmdList;
	ComPtr<ID3D12CommandQueue> mCommandQueue;
	ComPtr<ID3D12CommandAllocator> mCmdAllocator;
	ComPtr<IDXGIFactory4> mDxgiFactory;
	ComPtr<IDXGISwapChain3> mSwapChain;

	const int mDefaultSwapChainBufferCount = 2;
	UINT mCurrBackBufferIndex = 0;

	D3D12MA::Allocator* mMemAllocator = NULL;

	const UINT mRtvHeapSize = 2;
	static const UINT mSwapChainBufferCount = 2;

	ComPtr<ID3D12Fence> mFence;
	UINT64 mFenceValues[mSwapChainBufferCount] = {0, 0};
	HANDLE mFenceEvent = NULL;

	//ResourceStateTracker mResourceTracker;

	struct HeapData
	{
		ComPtr<ID3D12DescriptorHeap> pHeap;
		uint32_t usedEntries = 0;
	};
	HeapData mRtvHeap;

	struct FrameObject
	{
		ComPtr<ID3D12Resource> pSwapChainBuffer;
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	};
	FrameObject mFrameObjects[mSwapChainBufferCount];
};