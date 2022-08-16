#pragma once
#include "stdafx.h"
#include "SubMesh.h"

class Instance
{
public:
	Instance() = default;
	Instance(XMFLOAT3 position, XMFLOAT3 rotation, XMFLOAT3 scale);

	shared_ptr<Mesh>& GetMesh() { return mMesh; }
	void SetMesh(shared_ptr<Mesh> mesh) { mMesh = mesh; }

	void SetPosition(XMFLOAT3 position) { mPosition = position; }
	void SetRotation(XMFLOAT3 rotation) { mRotation = rotation; }
	void SetScale(XMFLOAT3 scale) { mScale = scale; }

	void Update();

private:
	XMFLOAT4X4 mWorld = {};

	XMFLOAT3 mPosition = {0, 0, 0};
	XMFLOAT3 mRotation = { 0, 0, 0 };
	XMFLOAT3 mScale = { 1, 1, 1 };

	UINT mAlbedoTextureIndex = UINT_MAX;
	UINT mMetalicTextureIndex = UINT_MAX;
	UINT mRoughnessTextureIndex = UINT_MAX;
	UINT mNormalMapTextureIndex = UINT_MAX;

	shared_ptr<Mesh> mMesh;
};