#pragma once

#include "d3dUtil.h"
#include "iostream"

using namespace DirectX;

class HeightMapImage
{
public:
	HeightMapImage();
	~HeightMapImage();

	void LoadHeightMapImage(const wchar_t* filepath, int width, int length, float scale);

	//���� �� �̹������� (x, z) ��ġ�� �ȼ� ���� ����� ������ ���̸� ��ȯ�Ѵ�. 
	float GetHeight(float x, float z);
	//���� �� �̹������� (x, z) ��ġ�� ���� ���͸� ��ȯ�Ѵ�. 
	XMFLOAT3 GetHeightMapNormal(int x, int z, float dx, float dz);
	
	uint16_t* GetHeightMapPixels() { return mHeightMapPixels; }
	int GetHeightMapWidth() { return mWidth; }
	int GetHeightMapLength() { return mLength; }

private:
	uint16_t*	mHeightMapPixels;

	int			mWidth;
	int			mLength;
	float		mYScale;
};

