#include "Terrain.h"

Terrain::Terrain()
{
}

Terrain::~Terrain()
{
}

void Terrain::LoadHeightMap(const wchar_t* filepath, int width, int length, float yScale)
{
	mHeightImage.LoadHeightMapImage(filepath, width, length, yScale);
}

void Terrain::CreateTerrain(float width, float length, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
{
	int imageWidth = mHeightImage.GetHeightMapWidth();
	int imageLength = mHeightImage.GetHeightMapLength();

	uint32_t vertexCount =  imageWidth * imageLength;

	//
	// Create the vertices.
	//

	float halfWidth = 0.5f * width;
	float halflength = 0.5f * length;

	float dx = width / (imageWidth - 1);
	float dz = length / (imageLength - 1);

	float du = 1.0f / (imageWidth - 1);
	float dv = 1.0f / (imageLength - 1);

	for (uint32_t i = 0; i < imageLength; ++i)
	{
		float z = halflength - i * dz;
		for (uint32_t j = 0; j < imageWidth; ++j)
		{
			float x = -halfWidth + j * dx;
			float y = mHeightImage.GetHeight(j, i);

			vertices[i * imageWidth + j].Pos = XMFLOAT3(x, y, z);
			vertices[i * imageWidth + j].Normal = mHeightImage.GetHeightMapNormal(j, i, dx, dz);
			//vertices[i * imageWidth + j].TangentU = XMFLOAT3(1.0f, 0.0f, 0.0f);

			// Stretch texture over grid.
			vertices[i * imageWidth + j].TexC.x = j * du;
			vertices[i * imageWidth + j].TexC.y = i * dv;
		}
	}
	// terrain y pos °ª ÆòÅºÈ­, normal °ª ÆòÅºÈ­
	int flattening = 3;
	for (UINT i = 2; i < imageLength -2; ++i)
	{
		for (UINT j = 2; j < imageWidth - 2; ++j)
		{
			XMFLOAT3 addNormal = XMFLOAT3(0.0f, 0.0f, 0.0f);
			float addYPos = 0.0f;
			int numAdd = 0;
			for (int x = j - flattening; x <= j + flattening; x++)
			{
				if (x < 3 || x > imageWidth - 4)
					continue;
				for (int y = i - flattening; y <= i + flattening; y++)
				{
					if (y < 3 || y > imageLength -4)
						continue;
					addYPos += vertices[x + imageWidth * y].Pos.y;
					addNormal.x += vertices[x + imageWidth * y].Normal.x;
					addNormal.y += vertices[x + imageWidth * y].Normal.y;
					addNormal.z += vertices[x + imageWidth * y].Normal.z;
					numAdd++;
				}
			}
			vertices[i * imageWidth + j].Pos.y = addYPos / numAdd;
			vertices[i * imageWidth + j].Normal = Vector3::Normalize(addNormal);
		}
	}
	
	//
	// Create the indices.
	//

	uint32_t faceCount = (imageWidth - 1) * (imageLength - 1) * 2;

	// Iterate over each quad and compute indices.
	uint32_t k = 0;
	for (uint32_t i = 0; i < imageLength - 1; ++i)
	{
		for (uint32_t j = 0; j < imageWidth - 1; ++j)
		{
			indices[k] = i * imageWidth + j;
			indices[k + 1] = i * imageWidth + j + 1;
			indices[k + 2] = (i + 1) * imageWidth + j;

			indices[k + 3] = (i + 1) * imageWidth + j;
			indices[k + 4] = i * imageWidth + j + 1;
			indices[k + 5] = (i + 1) * imageWidth + j + 1;

			k += 6; // next quad
		}
	}

	return;
}
