#pragma once
#include "stdafx.h"
#include "SubMesh.h"

class Mesh
{
public:
	Mesh() = default;
	Mesh(vector<SubMesh>& subMeshes) { mSubMeshes = subMeshes; }

	void SetBLAS(AccelerationStructureBuffers& blas) { mBLAS = blas; }
	AccelerationStructureBuffers& GetBLAS() { return mBLAS; }

	void SetSubMesh(SubMesh subMesh) { mSubMeshes.push_back(subMesh); }
	vector<SubMesh>& GetSubMeshes() { return mSubMeshes; }
	void SetVertexAttribIndex(UINT attribIndex) { mVertexAttribIndex = attribIndex; }
	void SetIndexBufferIndex(UINT attribIndex) { mIndexBufferIndex = attribIndex; }

	UINT GetVertexAttribIndex() { return mVertexAttribIndex; }
	UINT GetIndexBufferIndex() { return mIndexBufferIndex; }

	const UINT& GetSubMeshCount() { return mSubMeshes.size(); }

private:
	vector<SubMesh> mSubMeshes;
	AccelerationStructureBuffers mBLAS;

	UINT mVertexAttribIndex = UINT_MAX;
	UINT mIndexBufferIndex = UINT_MAX;

};