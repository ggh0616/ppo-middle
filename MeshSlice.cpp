#include "MeshSlice.h"

template<typename VertexType>
inline void ConvertBlobToVertexVector(std::vector<VertexType>& vertices, ID3DBlob* blob)
{
    BYTE* dataBegin = static_cast<BYTE*>(blob->GetBufferPointer());
    SIZE_T dataSize = blob->GetBufferSize();

    size_t vertexCount = dataSize / sizeof(VertexType);
    vertices.resize(vertexCount);
    memcpy(vertices.data(), dataBegin, dataSize);
}

template<typename VertexType>
inline void ConvertBlobToVertexVector(std::vector<VertexType>& vertices, const Submesh submesh, ID3DBlob* blob)
{
    BYTE* dataBegin = static_cast<BYTE*>(blob->GetBufferPointer());
    SIZE_T dataSize = blob->GetBufferSize();
    size_t baseVertexSize = submesh.baseVertex * sizeof(VertexType);

    size_t vertexCount = dataSize / sizeof(VertexType);
    vertices.resize(vertexCount - submesh.baseVertex);
    memcpy(vertices.data(), dataBegin + baseVertexSize, dataSize - baseVertexSize);
}

template<typename VertexType>
inline void ConvertBlobToVertexVector(std::vector<VertexType>& vertices, const Submesh submesh, UINT maxIndex, ID3DBlob* blob)
{
    BYTE* dataBegin = static_cast<BYTE*>(blob->GetBufferPointer());
    SIZE_T dataSize = (maxIndex + 1) * sizeof(VertexType);
    assert(dataSize <= blob->GetBufferSize());
    size_t baseVertexSize = submesh.baseVertex * sizeof(VertexType);

    size_t vertexCount = dataSize / sizeof(VertexType);
    vertices.resize(vertexCount - submesh.baseVertex);
    memcpy(vertices.data(), dataBegin + baseVertexSize, dataSize);
}

void ConvertBlobToIndexVector(std::vector<UINT>& indices, ID3DBlob* blob)
{
    BYTE* dataBegin = static_cast<BYTE*>(blob->GetBufferPointer());
    SIZE_T dataSize = blob->GetBufferSize();

    size_t indexCount = dataSize / sizeof(UINT);
    indices.resize(indexCount);
    memcpy(indices.data(), dataBegin, dataSize);
}

void ConvertBlobToIndexVector(std::vector<UINT>& indices, const Submesh submesh, ID3DBlob* blob)
{
    BYTE* dataBegin = static_cast<BYTE*>(blob->GetBufferPointer());
    SIZE_T dataSize = blob->GetBufferSize();
    size_t baseIndexSize = submesh.baseIndex * sizeof(UINT);

    indices.resize(submesh.numIndices);
    memcpy(indices.data(), dataBegin + baseIndexSize, submesh.numIndices * sizeof(UINT));
}

Vertex InterpolationVertexNormalAndTexC(const XMVECTOR plane, const Vertex vertex1, const Vertex vertex2)
{
    Vertex vertex;
    XMStoreFloat3(&vertex.Pos, XMPlaneIntersectLine(plane, XMLoadFloat3(&vertex1.Pos), XMLoadFloat3(&vertex2.Pos)));

    float t = Vector3::DistanceBetweenPoints(vertex1.Pos, vertex.Pos) / Vector3::DistanceBetweenPoints(vertex1.Pos, vertex2.Pos);
    
    vertex.Normal.x = vertex1.Normal.x * (1 - t) + vertex2.Normal.x * t;
    vertex.Normal.y = vertex1.Normal.y * (1 - t) + vertex2.Normal.y * t;
    vertex.Normal.z = vertex1.Normal.z * (1 - t) + vertex2.Normal.z * t;

    vertex.TexC.x = vertex1.TexC.x * (1 - t) + vertex2.TexC.x * t;
    vertex.TexC.y = vertex1.TexC.y * (1 - t) + vertex2.TexC.y * t;

    return vertex;
}

int MeshSlice::MeshCompleteSlice(const Mesh* targetMesh, const Submesh submesh, const XMFLOAT4 plane, vector<vector<Vertex>>& outVertices, vector<vector<UINT>>& outIndices)
{
    XMVECTOR planeVector = XMLoadFloat4(&plane);

    // targetMesh의 vertex/index data 얻기
    vector<Vertex>vertices;
    vector<UINT>indices;
    ConvertBlobToIndexVector(indices, submesh, targetMesh->mIndexBufferCPU.Get());
    int maxIndex = *max_element(indices.begin(), indices.end());
    ConvertBlobToVertexVector<Vertex>(vertices, submesh, maxIndex, targetMesh->mVertexBufferCPU.Get());

    vector<Vertex> slicingVertices[2]; // half space = slicingMesh1, oppoiste half space = slicingMesh2
    vector<UINT> slicingIndices[2]; // slicingMesh1, slicingMesh2
    unordered_map<Vertex, array<UINT, 2>, Vertex::Hash> addedVertices;
    vector<array<UINT, 2>> cuttingSurfaceIndices[2];  // 절단면을 기준으로 분리 (index, windingOrder에 따른 다음 index)

    int numVertices = (int)vertices.size();
    int numIndices = (int)indices.size();

    vector<array<int, 2>> newIndices(numVertices);  // targetMesh의 index -> slicingMesh1, slicingMesh2의 index
    // slicingMesh index에 -1이 들어갈 경우 해당 메시에 포함되지 않는 정점이다.

    // 정점 분리
    for (int i = 0; i < numVertices; i++)
    {
        Vertex vertex = vertices[i];

        // 절단 면을 기준으로 정점의 위치를 구분
        float dp = vertex.Pos.x * plane.x + vertex.Pos.y * plane.y + vertex.Pos.z * plane.z + plane.w;
        if (dp > 0) { // half space
            newIndices[i] = { (int)slicingVertices[0].size(), -1 };
            slicingVertices[0].push_back(vertex);
        }
        else if (dp < 0) { // oppoiste half space
            newIndices[i] = { -1, (int)slicingVertices[1].size() };
            slicingVertices[1].push_back(vertex);
        }
        else {  // 절단면 위에 존재
            newIndices[i] = { (int)slicingVertices[0].size(), (int)slicingVertices[1].size() };
            slicingVertices[0].push_back(vertex);
            slicingVertices[1].push_back(vertex);
        }
    }

    int numFace = numIndices / 3;
    // targetMesh의 각 면에 포함된 정점의 위치에 따라 정점을 분리한다.
    for (int i = 0; i < numFace; i++)
    {
        UINT index[3] = { indices[i * 3], indices[i * 3 + 1], indices[i * 3 + 2] };
        int vertexPos[3];   // 정점 위치에 따라 분류. 둘다 포함 = 0, slicingMesh1 = 1, slicingMesh2 = 2
        for (int j = 0; j < 3; j++)
            vertexPos[j] = (newIndices[index[j]][0] != -1) ? (newIndices[index[j]][1] != -1) ? 0 : 1 : 2;

        // 3점이 같은 공간에 존재할 경우 해당 면을 전부 추가
        if (vertexPos[0] == vertexPos[1] && vertexPos[1] == vertexPos[2]) {
            if (vertexPos[0] == 1) { // slicingMesh1
                slicingIndices[0].push_back(newIndices[index[0]][0]);
                slicingIndices[0].push_back(newIndices[index[1]][0]);
                slicingIndices[0].push_back(newIndices[index[2]][0]);
            }
            else if (vertexPos[0] == 2) { // slicingMesh2
                slicingIndices[1].push_back(newIndices[index[0]][1]);
                slicingIndices[1].push_back(newIndices[index[1]][1]);
                slicingIndices[1].push_back(newIndices[index[2]][1]);
            }
            else {  // slicingMesh1 & slicingMesh2
                slicingIndices[0].push_back(newIndices[index[0]][0]);
                slicingIndices[0].push_back(newIndices[index[1]][0]);
                slicingIndices[0].push_back(newIndices[index[2]][0]);

                slicingIndices[1].push_back(newIndices[index[0]][1]);
                slicingIndices[1].push_back(newIndices[index[1]][1]);
                slicingIndices[1].push_back(newIndices[index[2]][1]);
            }
        }
        // 2점이 같은 공간에 존재
        else if (vertexPos[0] == vertexPos[1] || vertexPos[0] == vertexPos[2] || vertexPos[1] == vertexPos[2]) {
            // 한 정점이라도 절단면 위에 존재하는 경우 면 절단이 일어나지 않는다.
            if (vertexPos[0] == 0 || vertexPos[1] == 0 || vertexPos[2] == 0) {
                if (vertexPos[0] == 1 || vertexPos[1] == 1 || vertexPos[2] == 1) {  // slicingMesh1
                    slicingIndices[0].push_back(newIndices[index[0]][0]);
                    slicingIndices[0].push_back(newIndices[index[1]][0]);
                    slicingIndices[0].push_back(newIndices[index[2]][0]);
                }
                else {  // slicingMesh2
                    slicingIndices[1].push_back(newIndices[index[0]][1]);
                    slicingIndices[1].push_back(newIndices[index[1]][1]);
                    slicingIndices[1].push_back(newIndices[index[2]][1]);
                }
            }
            // 모든 정점이 절단면 위에 존재하지 않으며 한 점이 다른 두점과 서로 다른 공간에 존재 -> 면 절단, 새로운 정점 2개 생성
            else {
                int sameIndex[2], otherIndex;
                int otherVertexPos = 0;  // otherVertexPos = otherIndex의 정점이 위치한 공간
                (vertexPos[1] == vertexPos[2]) ?    
                    // otherVertex = index0 -> sameVertex0, 1 = index1, 2
                    (otherVertexPos = vertexPos[0], otherIndex = index[0], sameIndex[0] = index[1], sameIndex[1] = index[2]) :
                    (vertexPos[0] == vertexPos[2]) ?    
                        // otherVertex = index1 -> sameVertex0, 1 = index2, 0
                        (otherVertexPos = vertexPos[1], otherIndex = index[1], sameIndex[0] = index[2], sameIndex[1] = index[0]) :
                        // otherVertex = index2 -> sameVertex0, 1 = index0, 1
                        (otherVertexPos = vertexPos[2], otherIndex = index[2], sameIndex[0] = index[0], sameIndex[1] = index[1]);

                Vertex newVertex[2] = { 
                    InterpolationVertexNormalAndTexC(planeVector, vertices[otherIndex], vertices[sameIndex[0]]),
                    InterpolationVertexNormalAndTexC(planeVector, vertices[otherIndex], vertices[sameIndex[1]]) };

                UINT newVertexIndices[2][2]; // vertexId, space 

                bool vertexAdded = false;
                for (int vi = 0; vi < 2; vi++)
                {
                    auto item = addedVertices.find(newVertex[vi]);
                    // 새로운 정점이 추가되있지 않았다면? 정점을 추가하고 인덱스를 부여한다.
                    if (item == addedVertices.end()) {
                        vertexAdded = true;
                        for (int space = 0; space < 2; space++)
                        {
                            newVertexIndices[vi][space] = (UINT)slicingVertices[space].size();
                            slicingVertices[space].push_back(newVertex[vi]);
                        }
                        //--
                        addedVertices[newVertex[vi]] = { newVertexIndices[vi][0], newVertexIndices[vi][1] };
                    }
                    // 새로운 정점이 이미 추가되어 있다면? 해당 정점의 인덱스만 가져온다.
                    else {
                        for (int space = 0; space < 2; space++)
                            newVertexIndices[vi][space] = item->second[space];
                    }
                }
                
                // 절단면 정점 생성
                if (vertexAdded) {
                    Vertex cuttingSurfaceNewVertex[2] = { newVertex[0], newVertex[1] };
                    XMVECTOR normalVector = XMVector4Normalize(XMVectorSet(plane.x, plane.y, plane.z, 0.0f));

                    // otherVertex 가 있는 공간에선 newVertex[1] -> newVertex[0] 순서로 면이 생성 됨
                    int otherSpace = otherVertexPos == 1 ? 0 : 1;
                    int space = 1 - otherSpace;
                    
                    // otherSpace 절단면 정점 채우기
                    XMStoreFloat3(&cuttingSurfaceNewVertex[0].Normal, normalVector);
                    XMStoreFloat3(&cuttingSurfaceNewVertex[1].Normal, normalVector);
                    slicingVertices[otherSpace].push_back(cuttingSurfaceNewVertex[0]);
                    slicingVertices[otherSpace].push_back(cuttingSurfaceNewVertex[1]);
                    cuttingSurfaceIndices[otherSpace].push_back({ (UINT)slicingVertices[otherSpace].size() - 1, (UINT)slicingVertices[otherSpace].size() - 2 });

                    // space 절단면 정점 채우기
                    XMStoreFloat3(&cuttingSurfaceNewVertex[0].Normal, -normalVector);
                    XMStoreFloat3(&cuttingSurfaceNewVertex[1].Normal, -normalVector);
                    slicingVertices[space].push_back(cuttingSurfaceNewVertex[0]);
                    slicingVertices[space].push_back(cuttingSurfaceNewVertex[1]);
                    cuttingSurfaceIndices[space].push_back({ (UINT)slicingVertices[space].size() - 2, (UINT)slicingVertices[space].size() - 1 });
                }

                int otherSpace = otherVertexPos == 1 ? 0 : 1;
                int space = 1 - otherSpace;

                // otherVertex가 있는 위치의 새로운 면 생성 face = {ohterVertex, newVertex0, newVertex1}
                slicingIndices[otherSpace].push_back(newIndices[otherIndex][otherSpace]);
                slicingIndices[otherSpace].push_back(newVertexIndices[0][otherSpace]);
                slicingIndices[otherSpace].push_back(newVertexIndices[1][otherSpace]);

                // sameVertex가 있는 위치의 새로운 면 생성 
                // face = {newVertex0, sameVertex1, newVertex1}
                slicingIndices[space].push_back(newVertexIndices[0][space]);
                slicingIndices[space].push_back(newIndices[sameIndex[1]][space]);
                slicingIndices[space].push_back(newVertexIndices[1][space]);
                // face = {sameVertex0, sameVertex1, newVertex0}
                slicingIndices[space].push_back(newIndices[sameIndex[0]][space]);
                slicingIndices[space].push_back(newIndices[sameIndex[1]][space]);
                slicingIndices[space].push_back(newVertexIndices[0][space]);
            }
        }
        // 3개의 점이 다른 공간에 존재
        else {
            int otherIndex[2], middleIndex;
            int otherVertexPos = 0;  // otherVertexPos = otherIndex[0]의 정점이 위치한 공간
            (vertexPos[0] == 0) ?
                otherVertexPos = vertexPos[1], middleIndex = index[0], otherIndex[0] = index[1], otherIndex[1] = index[2] :
                (vertexPos[1] == 0) ?
                    otherVertexPos = vertexPos[2], middleIndex = index[1], otherIndex[0] = index[2], otherIndex[1] = index[0] :
                    otherVertexPos = vertexPos[0], middleIndex = index[2], otherIndex[0] = index[0], otherIndex[1] = index[1];

            Vertex newVertex = InterpolationVertexNormalAndTexC(planeVector, vertices[otherIndex[0]], vertices[otherIndex[0]]);

            UINT newVertexIndices[2]; // space
            auto item = addedVertices.find(newVertex);
            //새로운 정점이 추가되있지 않았다면? 정점을 추가하고 인덱스를 부여한다.
            if (item == addedVertices.end()) {
                for (int space = 0; space < 2; space++)
                {
                    newVertexIndices[space] = (UINT)slicingVertices[space].size();
                    slicingVertices[space].push_back(newVertex);
                }
                addedVertices[newVertex] = { newVertexIndices[0], newVertexIndices[1] };
            }
            // 새로운 정점이 이미 추가되어 있다면? 해당 정점의 인덱스만 가져온다.
            else {
                for (int space = 0; space < 2; space++)
                    newVertexIndices[space] = item->second[space];
            }

            int space = otherVertexPos == 1 ? 0 : 1;
            int otherSpace = 1 - space;

            slicingIndices[space].push_back(newIndices[space][middleIndex]); // middleIndex
            slicingIndices[space].push_back(newVertexIndices[space]); // newVertex
            slicingIndices[space].push_back(newIndices[space][otherIndex[1]]); // index[1]

            slicingIndices[otherSpace].push_back(newVertexIndices[otherSpace]); // newVertex
            slicingIndices[otherSpace].push_back(newIndices[otherSpace][middleIndex]); // middleIndex
            slicingIndices[otherSpace].push_back(newIndices[otherSpace][otherIndex[0]]); // index[0]
        }
    }

    for (int space = 0; space < 2; space++)
    {
        UINT numCuttingSurfaceFaces = cuttingSurfaceIndices[space].size();
        Vertex centroidVertex;
        XMFLOAT3 normalVector;
        XMStoreFloat3(&normalVector, XMVector4Normalize(XMVectorSet(plane.x, plane.y, plane.z, 0.0f)) * ((space == 0) ? -1 : 1));

        XMFLOAT3 addedPos = XMFLOAT3(0.f, 0.f, 0.f);
        for (int i = 0; i < numCuttingSurfaceFaces; i++)
        {  
            addedPos = Vector3::Add(addedPos, slicingVertices[space][cuttingSurfaceIndices[space][i][0]].Pos);
            addedPos = Vector3::Add(addedPos, slicingVertices[space][cuttingSurfaceIndices[space][i][1]].Pos);
        }
        XMStoreFloat3(&addedPos, XMLoadFloat3(&addedPos) / (numCuttingSurfaceFaces * 2.0f) );
        centroidVertex.Pos = addedPos;
        centroidVertex.Normal = normalVector;
        centroidVertex.TexC.x = 0.5f;
        centroidVertex.TexC.y = 0.5f;

        UINT centroidIndex = slicingVertices[space].size();
        slicingVertices[space].push_back(centroidVertex);
        for (int i = 0; i < numCuttingSurfaceFaces; i++)
        {
            int index[2] = { cuttingSurfaceIndices[space][i][0], cuttingSurfaceIndices[space][i][1] };
            slicingVertices[space][index[0]].Normal = normalVector;
            slicingVertices[space][index[1]].Normal = normalVector;

            slicingIndices[space].push_back(index[0]);
            slicingIndices[space].push_back(index[1]);
            slicingIndices[space].push_back(centroidIndex);
        }
    }

    outVertices.push_back(slicingVertices[0]);
    outVertices.push_back(slicingVertices[1]);

    outIndices.push_back(slicingIndices[0]);
    outIndices.push_back(slicingIndices[1]);

    return 2;
}
