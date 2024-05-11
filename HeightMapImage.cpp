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
	//���� ���� ��ǥ�� ���� �κа� �Ҽ� �κ��� ����Ѵ�. 

	int x = (int)fx;
	int z = (int)fz;
	float fxPercent = fx - x;
	float fzPercent = fz - z;

	float fBottomLeft = (float)mHeightMapPixels[x + (z * mWidth)];
	float fBottomRight = (float)mHeightMapPixels[(x + 1) + (z * mWidth)];
	float fTopLeft = (float)mHeightMapPixels[x + ((z + 1) * mWidth)];
	float fTopRight = (float)mHeightMapPixels[(x + 1) + ((z + 1) * mWidth)];

#ifdef _WITH_APPROXIMATE_OPPOSITE_CORNER
	//z-��ǥ�� 1, 3, 5, ...�� ��� �ε����� �����ʿ��� �������� �����ȴ�. 
	bool bRightToLeft = ((z % 2) != 0);
	if (bRightToLeft)
	{
		/*������ �ﰢ������ �����ʿ��� ���� �������� �����Ǵ� ����̴�. 
		���� �׸��� �������� (fzPercent < fxPercent)�� ����̴�. 
		�� ��� TopLeft�� �ȼ� ���� (fTopLeft = fTopRight + (fBottomLeft - fBottomRight))�� �ٻ��Ѵ�. 
		���� �׸��� ������ (fzPercent �� fxPercent)�� ����̴�. 
		�� ��� BottomRight�� �ȼ� ���� (fBottomRight = fBottomLeft + (fTopRight - fTopLeft))�� �ٻ��Ѵ�.
		�ﰢ���� �����ʿ��� �������� �����Ǵ� ���*/
			if (fzPercent >= fxPercent)
				fBottomRight = fBottomLeft + (fTopRight - fTopLeft);
			else
				fTopLeft = fTopRight + (fBottomLeft - fBottomRight);
	}
	else
	{
		/*������ �ﰢ������ ���ʿ��� ������ �������� �����Ǵ� ����̴�. 
		���� �׸��� ������ (fzPercent < (1.0f - fxPercent))�� ����̴�. 
		�� ��� TopRight�� �ȼ� ���� (fTopRight = fTopLeft + (fBottomRight - fBottomLeft))�� �ٻ��Ѵ�. 
		���� �׸��� �������� (fzPercent �� (1.0f - fxPercent))�� ����̴�. 
		�� ��� BottomLeft�� �ȼ� ���� (fBottomLeft = fTopLeft + (fBottomRight - fTopRight))�� �ٻ��Ѵ�.
		�ﰢ���� ���ʿ��� ���������� �����Ǵ� ���*/
			if (fzPercent < (1.0f - fxPercent))
				fTopRight = fTopLeft + (fBottomRight - fBottomLeft);
			else
				fBottomLeft = fTopLeft + (fBottomRight - fTopRight);
	}
#endif

	//�簢���� �� ���� �����Ͽ� ����(�ȼ� ��)�� ����Ѵ�. 
	float fTopHeight = fTopLeft * (1 - fxPercent) + fTopRight * fxPercent;
	float fBottomHeight = fBottomLeft * (1 - fxPercent) + fBottomRight * fxPercent;
	float fHeight = fBottomHeight * (1 - fzPercent) + fTopHeight * fzPercent;

	return(fHeight);
}

DirectX::XMFLOAT3 HeightMapImage::GetHeightMapNormal(int x, int z, float dx, float dz)
{
	//x-��ǥ�� z-��ǥ�� ���� ���� ������ ����� ������ ���� ���ʹ� y-�� ���� �����̴�. 
	if ((x < 0.0f) || (z < 0.0f) || (x >= mWidth) || (z >= mLength))
		return(XMFLOAT3(0.0f, 1.0f, 0.0f));

	/*���� �ʿ��� (x, z) ��ǥ�� �ȼ� ���� ������ �� ���� �� (x+1, z), (z, z+1)��
	���� �ȼ� ���� ����Ͽ� ���� ���͸� ����Ѵ�.*/
	int nHeightMapIndex = x + (z * mWidth);
	int xHeightMapAdd = (x < (mWidth - 1)) ? 1 : -1;
	int zHeightMapAdd = (z < (mLength - 1)) ? mWidth : -mWidth;

	//(x, z), (x+2, z), (z, z+2)�� �ȼ����� ������ ���̸� ���Ѵ�. 
	float y1 = (float)mHeightMapPixels[nHeightMapIndex];
	float y2 = (float)mHeightMapPixels[nHeightMapIndex + xHeightMapAdd];
	float y3 = (float)mHeightMapPixels[nHeightMapIndex + zHeightMapAdd];

	//xmf3Edge1�� (0, y3, m_xmf3Scale.z) - (0, y1, 0) �����̴�. 
	XMFLOAT3 xmf3Edge1 = XMFLOAT3(0.0f, y3 - y1, dz);
	//xmf3Edge2�� (m_xmf3Scale.x, y2, 0) - (0, y1, 0) �����̴�. 
	XMFLOAT3 xmf3Edge2 = XMFLOAT3(dx, y2 - y1, 0.0f);
	//���� ���ʹ� xmf3Edge1�� xmf3Edge2�� ������ ����ȭ�ϸ� �ȴ�. 
	XMFLOAT3 xmf3Normal = Vector3::CrossProduct(xmf3Edge1, xmf3Edge2, true);

	return(xmf3Normal);
}
