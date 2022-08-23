#include "Camera.h"

Camera::Camera()
{
}

Camera::~Camera()
{
}

void Camera::SetPosition(float x, float y, float z)
{
	XMFLOAT3 pos = { x,y,z };
	SetPosition(pos);
}

void Camera::SetPosition(const XMFLOAT3& pos)
{
	mPosition = pos;
	mViewDirty = true;
}

void Camera::SetRotation(const XMFLOAT4& quat)
{
	auto mat = XMMatrixRotationQuaternion(XMLoadFloat4(&quat));
	mLook = Vector3::TransformNormal(mLook, mat);
	mRight = Vector3::TransformNormal(mRight, mat);
	mUp = Vector3::TransformNormal(mUp, mat);
}

void Camera::SetLook(const XMFLOAT3& look)
{
	mLook = look;
	mViewDirty = true;
}

void Camera::SetUp(const XMFLOAT3& up)
{
	mUp = up;
	mViewDirty = true;
}

void Camera::SetLens(float aspect)
{
	SetLens(mFov.y, aspect, mNearZ, mFarZ);
}

void Camera::SetLens(float fovY, float aspect, float zn, float zf)
{
	mNearWindow.y = 2.0f * tanf(fovY * 0.5f) * zn;
	mNearWindow.x = aspect * mNearWindow.y;

	mFarWindow.y = 2.0f * tanf(fovY * 0.5f) * zf;
	mFarWindow.x = aspect * mFarWindow.y;

	mFov.x = 2.0f * atan(tanf(fovY * 0.5f) * aspect);
	mFov.y = fovY;

	mAspect = aspect;
	mNearZ = zn;
	mFarZ = zf;

	XMMATRIX P = XMMatrixPerspectiveFovLH(mFov.y * mFovCoefficient, mAspect * mFovCoefficient, mNearZ * mFovCoefficient, mFarZ * mFovCoefficient);
	XMStoreFloat4x4(&mProj, P);
	BoundingFrustum::CreateFromMatrix(mFrustumView, P);
}

void Camera::SetOrthographicLens(XMFLOAT3& center, float range)
{
	UpdateViewMatrix();

	XMFLOAT3 C = Vector3::TransformCoord(center, mView);
	XMMATRIX P = XMMatrixOrthographicOffCenterLH(
		C.x - range, C.x + range,
		C.y - range, C.y + range,
		C.z - range, C.z + range);
	XMStoreFloat4x4(&mProj, P);
	BoundingFrustum::CreateFromMatrix(mFrustumView, P);
}

void Camera::SetFovCoefficient(float FovCoefficient)
{
	mFovCoefficient = FovCoefficient;
}

void Camera::LookAt(const XMFLOAT3& pos, const XMFLOAT3& target, const XMFLOAT3& up)
{
	mPosition = pos;
	mLook = Vector3::Normalize(Vector3::Subtract(target, pos));
	mRight = Vector3::Normalize(Vector3::Cross(up, mLook));
	mUp = Vector3::Cross(mLook, mRight);

	mViewDirty = true;
}

void Camera::LookAt(const XMFLOAT3& target)
{
	LookAt(mPosition, target, GetUp());
}

void Camera::LookAt(float x, float y, float z)
{
	LookAt(mPosition, XMFLOAT3(x, y, z), GetUp());
}

const XMFLOAT4X4& Camera::GetView() const
{
	assert(!mViewDirty && "Camera -> mView is not updated!!");
	return mView;
}

const XMFLOAT4X4& Camera::GetOldView() const
{
	return mOldView;
}

const XMFLOAT4X4& Camera::GetInverseView() const
{
	return mInvView;
}

const XMFLOAT4X4 Camera::GetInverseProj() const
{
	auto Proj = XMLoadFloat4x4(&GetProj());

	XMFLOAT4X4 invProj;
	auto determinant = XMMatrixDeterminant(Proj);
	XMStoreFloat4x4(&invProj, XMMatrixInverse(&determinant, Proj));

	return invProj;
}

void Camera::Move(float dx, float dy, float dz)
{
	mPosition = Vector3::MultiplyAdd(dx, mRight, mPosition);
	mPosition = Vector3::MultiplyAdd(dy, mUp, mPosition);
	mPosition = Vector3::MultiplyAdd(dz, mLook, mPosition);

	mViewDirty = true;
}

void Camera::Move(const XMFLOAT3& dir, float dist)
{
	mPosition = Vector3::MultiplyAdd(dist, dir, mPosition);
	mViewDirty = true;
}

void Camera::Strafe(float dist)
{
	Move(dist, 0.0f, 0.0f);
}

void Camera::Walk(float dist)
{
	Move(0.0f, 0.0f, dist);
}

void Camera::Upward(float dist)
{
	Move(XMFLOAT3(0.0f, 1.0f, 0.0f), dist);
}

void Camera::Pitch(float angle)
{
	XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&mRight), XMConvertToRadians(angle));
	mUp = Vector3::TransformNormal(mUp, R);
	mLook = Vector3::TransformNormal(mLook, R);

	mViewDirty = true;
}

void Camera::RotateY(float angle)
{
	XMMATRIX R = XMMatrixRotationY(XMConvertToRadians(angle));
	mRight = Vector3::TransformNormal(mRight, R);
	mUp = Vector3::TransformNormal(mUp, R);
	mLook = Vector3::TransformNormal(mLook, R);

	mViewDirty = true;
}

void Camera::Update(const float elapsedTime)
{
	mOldView = mView;
	UpdateViewMatrix();
}

void Camera::UpdateViewMatrix()
{
	if (mViewDirty)
	{
		mLook = Vector3::Normalize(mLook);
		mUp = Vector3::Normalize(Vector3::Cross(mLook, mRight));
		mRight = Vector3::Cross(mUp, mLook);

		mView(0, 0) = mRight.x; mView(0, 1) = mUp.x; mView(0, 2) = mLook.x;
		mView(1, 0) = mRight.y; mView(1, 1) = mUp.y; mView(1, 2) = mLook.y;
		mView(2, 0) = mRight.z; mView(2, 1) = mUp.z; mView(2, 2) = mLook.z;
		mView(3, 0) = -Vector3::Dot(mPosition, mRight);
		mView(3, 1) = -Vector3::Dot(mPosition, mUp);
		mView(3, 2) = -Vector3::Dot(mPosition, mLook);

		XMMATRIX viewMat = XMLoadFloat4x4(&mView);
		auto determinant = XMMatrixDeterminant(viewMat);
		XMMATRIX invViewMat = XMMatrixInverse(&determinant, viewMat);
		XMStoreFloat4x4(&mInvView, invViewMat);

		mFrustumView.Transform(mFrustumWorld, XMLoadFloat4x4(&mInvView));

		mViewDirty = false;
	}
}