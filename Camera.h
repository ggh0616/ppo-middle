#pragma once

#include "d3dUtil.h"

using namespace DirectX;

class Camera
{
public:
    Camera();
    ~Camera();

	// ���� ���� ī�޶� ��ġ ��ȸ, ����
	XMVECTOR GetPosition()const;
	XMFLOAT3 GetPosition3f()const;
	void SetPosition(float x, float y, float z);
	void SetPosition(const XMFLOAT3& v);

	// ī�޶� �������� ��ȸ, ����
	XMVECTOR GetRight()const { return XMLoadFloat3(&mRight); };
	XMFLOAT3 GetRight3f()const { return mRight; };
	XMVECTOR GetUp()const { return XMLoadFloat3(&mUp); };
	XMFLOAT3 GetUp3f()const { return mUp; };
	XMVECTOR GetLook()const { return XMLoadFloat3(&mLook); };
	XMFLOAT3 GetLook3f()const { return mLook; };
	XMFLOAT3 GetOffsetPosition()const { return mOffsetPosition; }

	// ����ü �Ӽ� ��ȸ
	float GetNearZ()const { return mNearZ; };
	float GetFarZ()const { return mFarZ; };
	float GetAspectRatio()const { return mAspect; };
	float GetFovY()const { return mFovY; };
	float GetFovX()const { return (2.0f * atan((0.5f * GetNearWindowWidth()) / mNearZ)); };

	// �þ� ���� ���� ����� �� ��� ��ȸ
	float GetNearWindowWidth()const { return mAspect * mNearWindowHeight; };
	float GetNearWindowHeight()const { return mNearWindowHeight; };
	float GetFarWindowWidth()const { return mAspect * mFarWindowHeight; };
	float GetFarWindowHeight()const { return mFarWindowHeight; };

	// �þ� ����ü ����
	void SetLens(float fovY, float aspect, float zn, float zf);
	void SetLens(float fovY, float aspect);

	// ī�޶� ��ġ, �ü� ����, ���� ���ͷ� ī�޶� ��ǥ�踦 �����Ѵ�.
	void LookAt(FXMVECTOR pos, FXMVECTOR target, FXMVECTOR worldUp);
	void LookAt(const XMFLOAT3& pos, const XMFLOAT3& target, const XMFLOAT3& up);

	void SetOffset(XMFLOAT3 position, float pitch, float yaw, float roll);

	// �þ� ��İ� ���� ��� ��ȸ
	XMMATRIX GetView()const;
	XMMATRIX GetProj()const;

	XMFLOAT4X4 GetView4x4f()const;
	XMFLOAT4X4 GetProj4x4f()const;

	// ī�޶� �̵�
	void Strafe(float d);
	void Walk(float d);
	void Up(float d);

	// ī�޶� ȸ��
	void Pitch(float angle);
	void RotateY(float angle);

	// �þ� ��� ������Ʈ
	void UpdateViewMatrix();

private:
	XMFLOAT3 mPosition = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 mRight = { 1.0f, 0.0f, 0.0f };
	XMFLOAT3 mUp = { 0.0f, 1.0f, 0.0f };
	XMFLOAT3 mLook = { 0.0f, 0.0f, 1.0f };
	
	float mMaxPitch = 89.5f;
	float mCurrentPitch = 0.0f;

	// ����ü �Ӽ�
	float mNearZ = 0.0f;
	float mFarZ = 0.0f;
	float mAspect = 0.0f;
	float mFovY = 0.0f;
	float mNearWindowHeight = 0.0f;
	float mFarWindowHeight = 0.0f;

	bool mViewDirty = true;

	XMFLOAT4X4 mOffsetMat = MathHelper::Identity4x4();
	XMFLOAT3 mOffsetPosition = { 0.0f, 0.0f, 0.0f };

	// �þ� ���, ���� ���;
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();
};

