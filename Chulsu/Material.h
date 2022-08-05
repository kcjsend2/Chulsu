#pragma once
#include "stdafx.h"
#include "Texture.h"

class Material
{
private:
	map<aiTextureType, Texture> mTextureMap;
};