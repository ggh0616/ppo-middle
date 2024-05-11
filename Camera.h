#pragma once

#include "d3dUtil.h"

using namespace DirectX;

class Camera
{
public:
    Camera();
    ~Camera();

	// 세계 공간 카메라 위치 조회, 설정
	XMVECTOR GetPosition()const;
	XMFLOAT3 GetPosition3f()const;
	void SetPosition(float x, float y, float z);
	void SetPosition(const XMFLOAT3& v);

	// 카메라 기적벡터 조회, 설정
	XMVECTOR GetRight()const { return XMLoadFloat3(&mRight); };
	XMFLOAT3 GetRight3f()const { return mRight; };
	XMVECTOR GetUp()const { return XMLoadFloat3(&mUp); };
	XMFLOAT3 GetUp3f()const { return mUp; };
	XMVECTOR GetLook()const { return XMLoadFloat3(&mLook); };
	XMFLOAT3 GetLook3f()const { return mLook; };
	XMFLOAT3 GetOffsetPosition()const { return mOffsetPosition; }

	// 절두체 속성 조회
	float GetNearZ()const { return mNearZ; };
	float GetFarZ()const { return mFarZ; };
	float GetAspectRatio()const { return mAspect; };
	float GetFovY()const { return mFovY; };
	float GetFovX()const { return (2.0f * atan((0.5f * GetNearWindowWidth()) / mNearZ)); };

	// 시야 공간 기준 가까운 면 평면 조회
	float GetNearWindowWidth()const { return mAspect * mNearWindowHeight; };
	float GetNearWindowHeight()const { return mNearWindowHeight; };
	float GetFarWindowWidth()const { return mAspect * mFarWindowHeight; };
	float GetFarWindowHeight()const { return mFarWindowHeight; };

	// 시야 절두체 설정
	void SetLens(float fovY, float aspect, float zn, float zf);
	void SetLens(float fovY, float aspect);

	// 카메라 위치, 시선 벡터, 상향 벡터로 카메라 좌표계를 설정한다.
	void LookAt(FXMVECTOR pos, FXMVECTOR target, FXMVECTOR worldUp);
	void LookAt(const XMFLOAT3& pos, const XMFLOAT3& target, const XMFLOAT3& up);

	void SetOffset(XMFLOAT3 position, float pitch, float yaw, float roll);

	// 시야 행렬과 투영 행렬 조회
	XMMATRIX GetView()const;
	XMMATRIX GetProj()const;

	XMFLOAT4X4 GetView4x4f()const;
	XMFLOAT4X4 GetProj4x4f()const;

	// 카메라 이동
	void Strafe(float d);
	void Walk(float d);
	void Up(float d);

	// 카메라 회전
	void Pitch(float angle);
	void RotateY(float angle);

	// 시야 행렬 업데이트
	void UpdateViewMatrix();

private:
	XMFLOAT3 mPosition = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 mRight = { 1.0f, 0.0f, 0.0f };
	XMFLOAT3 mUp = { 0.0f, 1.0f, 0.0f };
	XMFLOAT3 mLook = { 0.0f, 0.0f, 1.0f };
	
	float mMaxPitch = 89.5f;
	float mCurrentPitch = 0.0f;

	// 절두체 속성
	float mNearZ = 0.0f;
	float mFarZ = 0.0f;
	float mAspect = 0.0f;
	float mFovY = 0.0f;
	float mNearWindowHeight = 0.0f;
	float mFarWindowHeight = 0.0f;

	bool mViewDirty = true;

	XMFLOAT4X4 mOffsetMat = MathHelper::Identity4x4();
	XMFLOAT3 mOffsetPosition = { 0.0f, 0.0f, 0.0f };

	// 시야 행렬, 투영 행렬;
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();
};

