#pragma once
#include "stdafx.h"

class Shader
{
public:
	ComPtr<ID3DBlob> CompileLibrary(const WCHAR* filename, const WCHAR* targetString);
private:

};