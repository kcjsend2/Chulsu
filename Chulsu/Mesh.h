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

private:
	vector<SubMesh> mSubMeshes;
	AccelerationStructureBuffers mBLAS;
};