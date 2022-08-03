#pragma once
#include "stdafx.h"

class Mesh	
{
public:
	Mesh();
	virtual ~Mesh() {}

protected:
	ComPtr<ID3D12Resource> mVertexBufferGPU;
	ComPtr<ID3D12Resource> mIndexBufferGPU;

	ComPtr<ID3D12Resource> mVertexUploadBuffer;
	ComPtr<ID3D12Resource> mIndexUploadBuffer;
};