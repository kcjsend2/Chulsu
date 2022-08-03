#pragma once
#include "stdafx.h"
#include "Mesh.h"

class AssetManager
{
public:
	void LoadModel(const std::string& path);

private:
	unordered_map<string, shared_ptr<Mesh>> mCachedMeshes;
	//unordered_map<string, shared_ptr<Texture>> mCachedTextures;
};