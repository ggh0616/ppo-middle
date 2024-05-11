#pragma once
#include "Mesh.h"

template<typename VertexType>
inline void ConvertBlobToVertexVector(std::vector<VertexType>& vertices, ID3DBlob* blob);
template<typename VertexType>
inline void ConvertBlobToVertexVector(std::vector<VertexType>& vertices, const Submesh submesh, ID3DBlob* blob);

void ConvertBlobToIndexVector(std::vector<UINT>& indices, ID3DBlob* blob);
void ConvertBlobToIndexVector(std::vector<UINT>& indices, const Submesh submesh, ID3DBlob* blob);

namespace MeshSlice
{
    int MeshCompleteSlice(const Mesh* targetMesh, const Submesh submesh, const XMFLOAT4 plane, vector<vector<Vertex>>& outVertices, vector<vector<UINT>>& outIndices);
}