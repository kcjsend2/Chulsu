#include "ResourceStateTracker.h"

ResourceStateTracker::~ResourceStateTracker()
{
    mResourceStateMap.clear();
}

void ResourceStateTracker::TransitionBarrier(ComPtr<ID3D12GraphicsCommandList4> CmdList, ID3D12Resource* resource, D3D12_RESOURCE_STATES state)
{
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, mResourceStateMap[resource], state, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
    CmdList->ResourceBarrier(1, &barrier);
}
