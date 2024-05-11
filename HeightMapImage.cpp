#include "HeightMapImage.h"

HeightMapImage::HeightMapImage()
{
	mHeightMapPixels = nullptr;
	mWidth = 0;
	mLength = 0;
	mYScale = 1.0f;
}

HeightMapImage::~HeightMapImage()
{
	if (mHeightMapPixels) 
		delete[] mHeightMapPixels;
	mHeightMapPixels = NULL;
}

void HeightMapImage::LoadHeightMapImage(const wchar_t* filepath, int width, int length, float yScale)
{
	mWidth = width;
	mLength = length;
	mYScale = yScale;

	std::ifstream file(filepath, std::ios::binary | std::ios::ate);

	if (!file.is_open()) {
		std::wcerr << L"Failed to open file: " << filepath << std::endl;
		return;
	}

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	if (size % sizeof(uint16_t) != 0) {
		std::cerr << "File size is not a multiple of 16-bit values." << std::endl;
		return;
	}

	uint16_t* heightMapPixels = new uint16_t[mWidth * mLength];
	if (!file.read(reinterpret_cast<char*>(heightMapPixels), size)) {
		std::cerr << "Failed to read file." << std::endl;
		return;
	}

	mHeightMapPixels = new uint16_t[mWidth * mLength];
	for (int y = 0; y < mLength; y++)
	{
		for (int x = 0; x < mWidth; x++)
		{
			mHeightMapPixels[x + (y * mWidth)] =
				heightMapPixels[x + (y * mWidth)] * yScale;
		}
	}

	delete[] heightMapPixels;

	return;
}

#define _WITH_APPROXIMATE_OPPOSITE_CORNER

float HeightMapImage::GetHeight(float fx, float fz)
{
	if ((fx < 0.0f) || (fz < 0.0f) || (fx >= mWidth) || (fz >= mLength)) 
		return(0.0f);
	//높이 맵의 좌표의 정수 부분과 소수 부분을 계산한다. 

	int x = (int)fx;
	int z = (int)fz;
	float fxPercent = fx - x;
	float fzPercent = fz - z;

	float fBottomLeft = (float)mHeightMapPixels[x + (z * mWidth)];
	float fBottomRight = (float)mHeightMapPixels[(x + 1) + (z * mWidth)];
	float fTopLeft = (float)mHeightMapPixels[x + ((z + 1) * mWidth)];
	float fTopRight = (float)mHeightMapPixels[(x + 1) + ((z + 1) * mWidth)];

#ifdef _WITH_APPROXIMATE_OPPOSITE_CORNER
	//z-좌표가 1, 3, 5, ...인 경우 인덱스가 오른쪽에서 왼쪽으로 나열된다. 
	bool bRightToLeft = ((z % 2) != 0);
	if (bRightToLeft)
	{
		/*지형의 삼각형들이 오른쪽에서 왼쪽 방향으로 나열되는 경우이다. 
		다음 그림의 오른쪽은 (fzPercent < fxPercent)인 경우이다. 
		이 경우 TopLeft의 픽셀 값은 (fTopLeft = fTopRight + (fBottomLeft - fBottomRight))로 근사한다. 
		다음 그림의 왼쪽은 (fzPercent ≥ fxPercent)인 경우이다. 
		이 경우 BottomRight의 픽셀 값은 (fBottomRight = fBottomLeft + (fTopRight - fTopLeft))로 근사한다.
		삼각형이 오른쪽에서 왼쪽으로 나열되는 경우*/
			if (fzPercent >= fxPercent)
				fBottomRight = fBottomLeft + (fTopRight - fTopLeft);
			else
				fTopLeft = fTopRight + (fBottomLeft - fBottomRight);
	}
	else
	{
		/*지형의 삼각형들이 왼쪽에서 오른쪽 방향으로 나열되는 경우이다. 
		다음 그림의 왼쪽은 (fzPercent < (1.0f - fxPercent))인 경우이다. 
		이 경우 TopRight의 픽셀 값은 (fTopRight = fTopLeft + (fBottomRight - fBottomLeft))로 근사한다. 
		다음 그림의 오른쪽은 (fzPercent ≥ (1.0f - fxPercent))인 경우이다. 
		이 경우 BottomLeft의 픽셀 값은 (fBottomLeft = fTopLeft + (fBottomRight - fTopRight))로 근사한다.
		삼각형이 왼쪽에서 오른쪽으로 나열되는 경우*/
			if (fzPercent < (1.0f - fxPercent))
				fTopRight = fTopLeft + (fBottomRight - fBottomLeft);
			else
				fBottomLeft = fTopLeft + (fBottomRight - fTopRight);
	}
#endif

	//사각형의 네 점을 보간하여 높이(픽셀 값)를 계산한다. 
	float fTopHeight = fTopLeft * (1 - fxPercent) + fTopRight * fxPercent;
	float fBottomHeight = fBottomLeft * (1 - fxPercent) + fBottomRight * fxPercent;
	float fHeight = fBottomHeight * (1 - fzPercent) + fTopHeight * fzPercent;

	return(fHeight);
}

DirectX::XMFLOAT3 HeightMapImage::GetHeightMapNormal(int x, int z, float dx, float dz)
{
	//x-좌표와 z-좌표가 높이 맵의 범위를 벗어나면 지형의 법선 벡터는 y-축 방향 벡터이다. 
	if ((x < 0.0f) || (z < 0.0f) || (x >= mWidth) || (z >= mLength))
		return(XMFLOAT3(0.0f, 1.0f, 0.0f));

	/*높이 맵에서 (x, z) 좌표의 픽셀 값과 인접한 두 개의 점 (x+1, z), (z, z+1)에
	대한 픽셀 값을 사용하여 법선 벡터를 계산한다.*/
	int nHeightMapIndex = x + (z * mWidth);
	int xHeightMapAdd = (x < (mWidth - 1)) ? 1 : -1;
	int zHeightMapAdd = (z < (mLength - 1)) ? mWidth : -mWidth;

	//(x, z), (x+2, z), (z, z+2)의 픽셀에서 지형의 높이를 구한다. 
	float y1 = (float)mHeightMapPixels[nHeightMapIndex];
	float y2 = (float)mHeightMapPixels[nHeightMapIndex + xHeightMapAdd];
	float y3 = (float)mHeightMapPixels[nHeightMapIndex + zHeightMapAdd];

	//xmf3Edge1은 (0, y3, m_xmf3Scale.z) - (0, y1, 0) 벡터이다. 
	XMFLOAT3 xmf3Edge1 = XMFLOAT3(0.0f, y3 - y1, dz);
	//xmf3Edge2는 (m_xmf3Scale.x, y2, 0) - (0, y1, 0) 벡터이다. 
	XMFLOAT3 xmf3Edge2 = XMFLOAT3(dx, y2 - y1, 0.0f);
	//법선 벡터는 xmf3Edge1과 xmf3Edge2의 외적을 정규화하면 된다. 
	XMFLOAT3 xmf3Normal = Vector3::CrossProduct(xmf3Edge1, xmf3Edge2, true);

	return(xmf3Normal);
}
