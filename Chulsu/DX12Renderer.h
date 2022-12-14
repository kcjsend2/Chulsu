#pragma once
#include "stdafx.h"
#include "Renderer.h"
#include "AssetManager.h"
#include "Camera.h"
#include "LightManager.h"

class Pipeline;

class DX12Renderer : public Renderer
{
public:
	DX12Renderer();
	~DX12Renderer();

	virtual void Init(HWND winHandle, uint32_t winWidth, uint32_t winHeight) override;

	virtual void Update() override;
	virtual void Draw() override;
	virtual void BuildObjects() override;

	virtual LRESULT OnProcessMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

protected:
	virtual void OnResize() override;

	virtual void OnProcessMouseDown(WPARAM buttonState, int x, int y) override;
	virtual void OnProcessMouseUp(WPARAM buttonState, int x, int y) override;
	virtual void OnProcessMouseMove(WPARAM buttonState, int x, int y) override;
	virtual void OnProcessKeyInput(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	virtual void OnPreciseKeyInput() override;

private:
	void WaitUntilGPUComplete();
	ComPtr<IDXGISwapChain3> CreateDxgiSwapChain(ComPtr<IDXGIFactory4> pFactory, HWND hwnd, uint32_t width, uint32_t height, DXGI_FORMAT format, ComPtr<ID3D12CommandQueue> pCommandQueue);
	ComPtr<ID3D12Device5> CreateDevice(ComPtr<IDXGIFactory4> pDxgiFactory);
	ComPtr<ID3D12CommandQueue> CreateCommandQueue(ComPtr<ID3D12Device5> pDevice);
	ComPtr<ID3D12DescriptorHeap> CreateSwapchainDescriptorHeap(ComPtr<ID3D12Device5> pDevice, uint32_t count, D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisible);
	D3D12_CPU_DESCRIPTOR_HANDLE CreateSwapchainRTV(ComPtr<ID3D12Device5> pDevice, ComPtr<ID3D12Resource> pResource, ComPtr<ID3D12DescriptorHeap> pHeap, uint32_t& usedHeapEntries, DXGI_FORMAT format);

	UINT mCurrBackBufferIndex = 0;
	const UINT mRtvHeapSize = 2;
	static const UINT mSwapChainBufferCount = 2;

	ComPtr<ID3D12Device5> mDevice;
	ComPtr<ID3D12GraphicsCommandList4> mCmdList;
	ComPtr<ID3D12CommandQueue> mCmdQueue;
	ComPtr<IDXGIFactory4> mDxgiFactory;
	ComPtr<IDXGISwapChain3> mSwapChain;

	ComPtr<D3D12MA::Allocator> mAllocator = NULL;

	ComPtr<ID3D12Fence> mFence;
	UINT64 mFenceValue = 0;
	HANDLE mFenceEvent = NULL;

	ResourceStateTracker mResourceTracker;

	struct HeapData
	{
		ComPtr<ID3D12DescriptorHeap> pHeap;
		uint32_t usedEntries = 0;
	};
	HeapData mRtvHeap;

	struct FrameObject
	{
		ComPtr<ID3D12CommandAllocator> pCommandAllocator;
		ComPtr<ID3D12Resource> pSwapChainBuffer;
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = {};
	};
	FrameObject mFrameObjects[mSwapChainBufferCount];

	AssetManager mAssetMgr;
	LightManager mLightMgr;

	unordered_map<string, Pipeline> mPipelines;

	ComPtr<D3D12MA::Allocation> mOutputTexture;

	UINT mOutputTextureIndex = UINT_MAX;
	UINT mLightIndex = UINT_MAX;

	Camera mCamera;

	XMFLOAT3 mSunDirection = {0, 1, 0};
};