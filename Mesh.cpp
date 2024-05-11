#include "Mesh.h"

Mesh::Mesh()
{
}

Mesh::~Mesh()
{
}

Submesh Mesh::GetSubmesh(string name)
{
	for (int i = 0; i < mSubmeshes.size(); i++)
	{
		if (mSubmeshes[i].name == name)
			return mSubmeshes[i];
	}

	return Submesh();
}

void Mesh::AddSubmesh(const string name, UINT numIndices, UINT baseVertex, UINT baseIndex, UINT materialIndex)
{
	Submesh submesh;
	
	submesh.name = name;
	submesh.numIndices = numIndices;
	submesh.baseVertex = baseVertex;
	submesh.baseIndex = baseIndex;
	submesh.materialIndex = materialIndex;
	
	mSubmeshes.push_back(submesh);
}

void Mesh::CreateBlob(const vector<Vertex>& vertices, const vector<UINT>& indices)
{
	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(UINT);

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &mVertexBufferCPU));
	CopyMemory(mVertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &mIndexBufferCPU));
	CopyMemory(mIndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);
}

void Mesh::UploadBuffer(ID3D12Device* d3dDevice, ID3D12GraphicsCommandList* commandList,
	vector<Vertex> vertices, vector<UINT> indices)
{
	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(UINT);

	mVertexBufferGPU = d3dUtil::CreateDefaultBuffer(d3dDevice, commandList, 
		vertices.data(), vbByteSize, mVertexBufferUploader);

	mIndexBufferGPU = d3dUtil::CreateDefaultBuffer(d3dDevice, commandList, 
		indices.data(), ibByteSize, mIndexBufferUploader);

	mVertexByteStride = sizeof(Vertex);
	mVertexBufferByteSize = vbByteSize;
	mIndexFormat = DXGI_FORMAT_R32_UINT;
	mIndexBufferByteSize = ibByteSize;
}

D3D12_VERTEX_BUFFER_VIEW Mesh::VertexBufferView() const
{
	if (mVertexBufferGPU != nullptr) {
		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = mVertexBufferGPU->GetGPUVirtualAddress();
		vbv.StrideInBytes = mVertexByteStride;
		vbv.SizeInBytes = mVertexBufferByteSize;

		return vbv;
	}
}

D3D12_INDEX_BUFFER_VIEW Mesh::IndexBufferView() const
{
	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = mIndexBufferGPU->GetGPUVirtualAddress();
	ibv.Format = mIndexFormat;
	ibv.SizeInBytes = mIndexBufferByteSize;

	return ibv;
}

// 자료를 GPU에 모두 올린 후에는 메모리를 해제해도 된다.
void Mesh::DisposeUploaders()
{
	mVertexBufferUploader = nullptr;
	mIndexBufferUploader = nullptr;
}

