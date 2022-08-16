#include "Instance.h"

Instance::Instance(XMFLOAT3 position, XMFLOAT3 rotation, XMFLOAT3 scale) : mPosition(position), mRotation(rotation), mScale(scale)
{
}

void Instance::Update()
{
	mWorld = Matrix4x4::CalulateWorldTransform(mPosition, mRotation, mScale);
}
