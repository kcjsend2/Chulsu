#pragma once
#include "stdafx.h"
class Camera
{
public:
	Camera();
	Camera(const Camera& rhs) = delete;
	Camera& operator=(const Camera& rhs) = delete;
	virtual ~Camera();

	void SetPosition(float x, float y, float z);
	void SetPosition(const XMFLOAT3& pos);

	void SetRotation(const XMFLOAT4& quat);

	void SetOffset(float x, float y, float z) { mOffset = { x,y,z }; }
	void SetOffset(const XMFLOAT3& offset) { mOffset = offset; }

	void SetLook(const XMFLOAT3& look);
	void SetUp(const XMFLOAT3& up);

	void SetPlayer(class Player* player) { mPlayer = player; }
	void SetTimeLag(float lag) { mTimeLag = lag; }

	void SetLens(float aspect);
	void SetLens(float fovY, float aspect, float zn, float zf);
	void SetOrthographicLens(XMFLOAT3& center, float range);

	void SetFovCoefficient(float AspectCoefficient);

	void LookAt(const XMFLOAT3& pos, const XMFLOAT3& target, const XMFLOAT3& up);
	void LookAt(const XMFLOAT3& target);
	void LookAt(float x, float y, float z);

	virtual void Move(float dx, float dy, float dz);
	virtual void Move(const XMFLOAT3& dir, float dist);

	void Strafe(float dist);
	void Walk(float dist);
	void Upward(float dist);

	virtual void Pitch(float angle);
	virtual void RotateY(float angle);

	virtual void Update(const float elapsedTime);
	virtual void UpdateViewMatrix();

public:
	const XMFLOAT3& GetPosition() const { return mPosition; }
	const XMFLOAT3& GetRight() const { return mRight; }
	const XMFLOAT3& GetUp() const { return mUp; }
	const XMFLOAT3& GetLook() const { return mLook; }

	const XMFLOAT3& GetOffset() const { return mOffset; }

	float GetNearZ() const { return mNearZ; }
	float GetFarZ() const { return mFarZ; }
	float GetAspect() const { return mAspect; }

	const XMFLOAT2& GetFov() const { return mFov; }
	const XMFLOAT2& GetNearWindow() const { return mNearWindow; }
	const XMFLOAT2& GetFarWindow() const { return mFarWindow; }

	const XMFLOAT4X4& GetView() const;
	const XMFLOAT4X4& GetOldView() const;
	const XMFLOAT4X4& GetInverseView() const;
	const XMFLOAT4X4& GetProj() const { return mProj; }
	const XMFLOAT4X4 GetInverseProj() const;

	const BoundingFrustum& GetWorldFrustum() const { return mFrustumWorld; }
	const BoundingFrustum& GetViewFrustum() const { return mFrustumView; }

protected:
	bool mViewDirty = false;

	XMFLOAT3 mPosition = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 mRight = { 1.0f, 0.0f, 0.0f };
	XMFLOAT3 mUp = { 0.0f, 1.0f, 0.0f };
	XMFLOAT3 mLook = { 0.0f, 0.0f, 1.0f };

	XMFLOAT3 mOffset = { 0.0f,0.0f,0.0f };
	float mTimeLag = 0.0f;

	float mFarZ = 0.0f;
	float mNearZ = 0.0f;
	float mAspect = 0.0f;

	XMFLOAT2 mFov = { 0.0f, 0.0f };
	XMFLOAT2 mNearWindow = { 0.0f, 0.0f };
	XMFLOAT2 mFarWindow = { 0.0f, 0.0f };

	XMFLOAT4X4 mView = Matrix4x4::Identity4x4();
	XMFLOAT4X4 mProj = Matrix4x4::Identity4x4();
	XMFLOAT4X4 mInvView = Matrix4x4::Identity4x4();
	XMFLOAT4X4 mOldView = Matrix4x4::Identity4x4();

	BoundingFrustum mFrustumView;
	BoundingFrustum mFrustumWorld;

	class Player* mPlayer = nullptr;

	float mFovYNeutral = 0.0f;
	float mFarZNeutral = 0.0f;
	float mNearZNeutral = 0.0f;
	float mAspectNeutral = 0.0f;

	float mFovCoefficient = 1.0f;
};
