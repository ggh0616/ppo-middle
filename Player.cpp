#include "Player.h"

XMFLOAT3 MultipleVelocity(const XMFLOAT3& dir, const XMFLOAT3& scalar)
{
	return XMFLOAT3(dir.x * scalar.x, dir.y * scalar.y, dir.z * scalar.z);
}

Player::Player() :
	GameObject()
{
	InitPlayer();
}

void Player::ChangeState(PlayerState* nextState)
{
	if (nextState->ID() == mCurrentState->ID())
		return;

	if ((UINT)nextState->ID() < 0 || (UINT)nextState->ID() >= (UINT)StateId::Count)
		return;

	mCurrentState->Exit(*this);
	delete mCurrentState;
	mCurrentState = nextState;
	mCurrentState->Enter(*this);
}

Player::Player(const string name, XMFLOAT4X4 world, XMFLOAT4X4 texTransform) : 
    GameObject(name, world, texTransform)
{
	InitPlayer();
}

Player::Player(const string name, XMMATRIX world, XMMATRIX texTransform) :
    GameObject(name, world, texTransform)
{
	InitPlayer();
}

Player::~Player()
{
	if (mCamera)
		delete mCamera;
}

void Player::Update(const GameTimer& gt)
{
	float deltaTime = gt.DeltaTime();

	// 추락
	mVelocity.y -= mGravity * deltaTime;
	float fallSpeed = sqrt(mVelocity.y * mVelocity.y);
	if ((mVelocity.y * mVelocity.y) > (mMaxVelocityY * mMaxVelocityY) && mVelocity.y < 0) {
		mVelocity.y = -mMaxVelocityY;
	}

	// 최대 속도 제한
	float maxVelocityXZ = mIsRun ? mMaxRunVelocityXZ : mMaxWalkVelocityXZ;
	float groundSpeed = sqrt(mVelocity.x * mVelocity.x + mVelocity.z * mVelocity.z);
	if (groundSpeed > maxVelocityXZ) {
		mVelocity.x *= maxVelocityXZ / groundSpeed;
		mVelocity.z *= maxVelocityXZ / groundSpeed;
	}

	// 마찰
	XMFLOAT3 friction;
	XMStoreFloat3(&friction, -XMVector3Normalize(XMVectorSet(mVelocity.x, 0.0f, mVelocity.z, 0.0f)) * mFriction * deltaTime);
	mVelocity.x = (mVelocity.x >= 0.0f) ? max(0.0f, mVelocity.x + friction.x) : min(0.0f, mVelocity.x + friction.x);
	mVelocity.z = (mVelocity.z >= 0.0f) ? max(0.0f, mVelocity.z + friction.z) : min(0.0f, mVelocity.z + friction.z);

	SetPosition(Vector3::Add(GetPosition(), mVelocity));

	if (GetPosition().y < 0) {
		SetPosition(GetPosition().x, 0.0f, GetPosition().z);
		mVelocity.y = 0.0f;
		mIsFalling = false;
	}

	// 카메라 이동
	UpdateCamera();
	UpdateState(deltaTime);

	mAnimationTime += deltaTime;

	SetFrameDirty();
}

void Player::UpdateCamera()
{
	if (mMode == THIRD_PERSON) {
		XMVECTOR cameraLook = XMVector3TransformNormal(XMLoadFloat3(&GetLook()), XMMatrixRotationAxis(XMLoadFloat3(&GetRight()), mPitch));

		XMVECTOR playerPosition = XMLoadFloat3(&GetPosition()) + XMLoadFloat3(&mCameraOffsetPosition) + XMVectorSet(0.0f, 50.0f, 0.f, 0.0f);
		XMVECTOR cameraPosition = playerPosition - cameraLook * mZoomFactor;// + XMVectorSet(0.0f, 50.0f, 0.f, 0.0f); // distance는 카메라와 플레이어 사이의 거리

		XMMATRIX viewMatrix = XMMatrixLookAtLH(cameraPosition, playerPosition, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
		mCamera->LookAt(cameraPosition, playerPosition, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

		mCamera->UpdateViewMatrix();
	}
	
	if (mMode == FIRST_PERSON) {
		// 플레이어의 위치와 시선 벡터를 가져옵니다.
		XMVECTOR playerPosition = XMLoadFloat3(&GetPosition()) + XMVectorSet(0.0f, 180.0f, 0.f, 0.0f);;
		XMVECTOR playerLook = XMLoadFloat3(&GetLook());

		// 카메라의 위치를 플레이어의 위치로 설정합니다.
		XMVECTOR cameraPosition = playerPosition;

		// 카메라의 시야를 플레이어가 바라보는 방향으로 설정합니다.
		XMVECTOR cameraLook = playerPosition + playerLook;

		// 카메라의 뷰 매트릭스를 생성합니다.
		XMMATRIX viewMatrix = XMMatrixLookAtLH(cameraPosition, cameraLook, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

		// 카메라의 시야를 설정합니다.
		mCamera->LookAt(cameraPosition, cameraLook, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

		// 뷰 매트릭스를 업데이트합니다.
		mCamera->UpdateViewMatrix();
	}
	
}

void Player::UpdateState(const float deltaTime)
{
	if (GetPosition().y > 0 && !mIsFalling) {
		mIsFalling = true;
		ChangeState(new PlayerStateFall);
	}

	if (!mIsFalling) {
		float groundSpeed = sqrt(mVelocity.x * mVelocity.x + mVelocity.z * mVelocity.z);
		if (groundSpeed <= 0.1f) {
			if (GetStateId() != (UINT)StateId::Land && GetStateId() != (UINT)StateId::Fall)
				ChangeState(new PlayerStateIdle);
		}
		else if (groundSpeed > mMaxWalkVelocityXZ) {
			ChangeState(new PlayerStateRun);
		}
		else {
			ChangeState(new PlayerStateWalk);
		}
	}

	mCurrentState->Update(*this, deltaTime);
}

void Player::KeyboardInput(float dt)
{
	mIsRun = false;
	if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
		mIsRun = true;

	XMFLOAT3 velocity;

	if (mIsFalling)
		velocity = Vector3::ScalarProduct(mAcceleration, dt / 2, false);
	else if (mIsRun)
		velocity = Vector3::ScalarProduct(mAcceleration, dt * 2, false);
	else
		velocity = Vector3::ScalarProduct(mAcceleration, dt, false);

	if (GetAsyncKeyState('W') & 0x8000)
		mVelocity = Vector3::Add(mVelocity, MultipleVelocity(GetLook(), velocity));

	if (GetAsyncKeyState('S') & 0x8000)
		mVelocity = Vector3::Add(mVelocity, MultipleVelocity(Vector3::ScalarProduct(GetLook(), -1), velocity));

	if (GetAsyncKeyState('D') & 0x8000)
		mVelocity = Vector3::Add(mVelocity, MultipleVelocity(GetRight(), velocity));

	if (GetAsyncKeyState('A') & 0x8000)
		mVelocity = Vector3::Add(mVelocity, MultipleVelocity(Vector3::ScalarProduct(GetRight(), -1), velocity));

	if (GetAsyncKeyState('I') & 0x8000 && mZoomFactor > 100.f && mMode == THIRD_PERSON) {
		float in = mZoomFactor;
		in -= 5.f;
		SetZoomFactor(in);
	}
	if (GetAsyncKeyState('O') & 0x8000 && mZoomFactor < 510.f && mMode == THIRD_PERSON) {
		float in = mZoomFactor;
		in += 5.f;
		SetZoomFactor(in);
	}
	if (GetAsyncKeyState('T') & 0x8000)
		mMode = THIRD_PERSON;
	if (GetAsyncKeyState('F') & 0x8000)
		mMode = FIRST_PERSON;
}

void Player::OnKeyboardMessage(UINT nMessageID, WPARAM wParam)
{
	switch (nMessageID)
	{
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_SPACE:
			if (!mIsFalling) {
				mVelocity.y += mJumpForce;
				mIsFalling = true;
				ChangeState(new PlayerStateJump);
			}
			break;
		}
		break;
	}
}

void Player::MouseInput(float dx, float dy)
{
	Rotate(0.f, dx, 0.f);

	float maxPitchRaidan = XMConvertToRadians(MAX_PLAYER_CAMERA_PITCH);
	mPitch += dy;
	mPitch = (mPitch < -maxPitchRaidan) ? -maxPitchRaidan : (maxPitchRaidan < mPitch) ? maxPitchRaidan : mPitch;
}

void Player::InitPlayer()
{
	// Set Camera
	mCamera = new Camera();

	mCameraOffsetPosition = XMFLOAT3(0.0f, 100.0f, 0.0f);
	UpdateCamera();

	mAnimationIndex[0] = 0;
	mAnimationIndex[1] = 0;
	mAnimationIndex[2] = 1;
	mAnimationIndex[3] = 2;
	mAnimationIndex[4] = 3;
	mAnimationIndex[5] = 4;
	mAnimationIndex[6] = 5;

	mCurrentState = new PlayerStateIdle;
	mCurrentState->Enter(*this);
}
