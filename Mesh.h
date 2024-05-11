#pragma once
#include "d3dUtil.h"
#include "FrameResource.h"

#define VERTEXT_POSITION				0x01
#define VERTEXT_COLOR					0x02
#define VERTEXT_NORMAL					0x04
#define VERTEXT_TANGENT					0x08
#define VERTEXT_TEXTURE_COORD0			0x10
#define VERTEXT_TEXTURE_COORD1			0x20

#define VERTEXT_TEXTURE					(VERTEXT_POSITION | VERTEXT_TEXTURE_COORD0)
#define VERTEXT_DETAIL					(VERTEXT_POSITION | VERTEXT_TEXTURE_COORD0 | VERTEXT_TEXTURE_COORD1)
#define VERTEXT_NORMAL_TEXTURE			(VERTEXT_POSITION | VERTEXT_NORMAL | VERTEXT_TEXTURE_COORD0)
#define VERTEXT_NORMAL_TANGENT_TEXTURE	(VERTEXT_POSITION | VERTEXT_NORMAL | VERTEXT_TANGENT | VERTEXT_TEXTURE_COORD0)
#define VERTEXT_NORMAL_DETAIL			(VERTEXT_POSITION | VERTEXT_NORMAL | VERTEXT_TEXTURE_COORD0 | VERTEXT_TEXTURE_COORD1)
#define VERTEXT_NORMAL_TANGENT__DETAIL	(VERTEXT_POSITION | VERTEXT_NORMAL | VERTEXT_TANGENT | VERTEXT_TEXTURE_COORD0 | VERTEXT_TEXTURE_COORD1)

using namespace std;
using namespace DirectX;

struct Submesh
{
	string name;

	UINT baseVertex = 0;
	UINT baseIndex = 0;
	UINT numIndices = 0;
	UINT materialIndex = -1;

	// ����޽��� �ٿ�� �ڽ�
	BoundingBox bounds;
};

// �޽��� �̸�, ����, �ε����� ����
class Mesh
{
public:
	Mesh();
	~Mesh();

	// �޽ø� �̸����� ��ȸ
	string mName;

	UINT mType = 0x00;

	BoundingBox mBounds;

	D3D12_PRIMITIVE_TOPOLOGY mPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	int mNumVertices = 0;
	int mNumIndices = 0;

	vector<XMFLOAT3> mPositions;
	vector<XMFLOAT3> mNormals;
	vector<XMFLOAT2> mTexCoords;
	vector<uint32_t> mIndices;

	vector<Material> mMaterials;

	// �ý��� �޸� ���纻.
	// ����/���� ������ �������� �� �����Ƿ� ID3DBlob�� ����Ѵ�.
	Microsoft::WRL::ComPtr<ID3DBlob> mVertexBufferCPU = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> mIndexBufferCPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> mVertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mIndexBufferGPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> mVertexBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mIndexBufferUploader = nullptr;

	// ���۵鿡 ���� �ڷ�
	UINT mVertexByteStride = 0;
	UINT mVertexBufferByteSize = 0;

	DXGI_FORMAT mIndexFormat = DXGI_FORMAT_R32_UINT;
	unsigned int mIndexBufferByteSize = 0;

	// �� Mesh �ν��ͽ��� �� ����/���� ���ۿ� �������� ���ϱ����� ���� �� �ִ�.
	// �κ� �޽õ��� ���������� �׸� �� �ֵ���, submesh�� �����̳ʿ� ��Ƶд�.
	vector<Submesh> mSubmeshes;
public:
	Submesh GetSubmesh(string name);

	void AddSubmesh(const string name, UINT numIndices,
		UINT baseVertex = 0, UINT baseIndex = 0, UINT materialIndex = 0);

	void CreateBlob(const vector<Vertex>& vertices, const vector<UINT>& indices);
	void UploadBuffer(ID3D12Device* d3dDevice, ID3D12GraphicsCommandList* commandList, vector<Vertex> vertices, vector<UINT> indices);

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView()const;
	D3D12_INDEX_BUFFER_VIEW IndexBufferView()const;

	void DisposeUploaders();

	//void LoadMeshFromFile(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, FILE* file);
};