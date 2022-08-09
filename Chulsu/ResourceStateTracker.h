#pragma once
#include "stdafx.h"

using Microsoft::WRL::ComPtr;
using namespace std;

class ResourceStateTracker
{
public:
	ResourceStateTracker() {}
	~ResourceStateTracker();
	void TransitionBarrier(ComPtr<ID3D12GraphicsCommandList4> CmdList, ID3D12Resource* resource, D3D12_RESOURCE_STATES state);

	void AddTrackingResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES state) { mResourceStateMap[resource] = state; }
	D3D12_RESOURCE_STATES GetResourceState(ID3D12Resource* resource) { return mResourceStateMap[resource]; }

private:
	unordered_map<ID3D12Resource*, D3D12_RESOURCE_STATES> mResourceStateMap;
};