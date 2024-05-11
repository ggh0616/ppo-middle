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

	//높이 맵 이미지에서 (x, z) 위치의 픽셀 값에 기반한 지형의 높이를 반환한다. 
	float GetHeight(float x, float z);
	//높이 맵 이미지에서 (x, z) 위치의 법선 벡터를 반환한다. 
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

