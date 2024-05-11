#pragma once

#define MAX_NUM_BONES_PER_VERTEX 4

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp\cimport.h>
#include "d3dUtil.h"
#include "Mesh.h"
#include <map>

using namespace DirectX;
using namespace std;

// �ִϸ��̼��� �� ������
template <typename TXMFLOAT>
struct Keyframe
{
    float timePos = 0.0f;

    TXMFLOAT value;
};

// Keyframe�� ����Ʈ
// �ð����� �̿��� 2���� keyframe�� ������ ��İ��� ����.
struct BoneAnimation
{   
    vector<Keyframe<XMFLOAT3>> translation;
    vector<Keyframe<XMFLOAT3>> scale;
    vector<Keyframe<XMFLOAT4>> rotationQuat;
};

// �ϳ��� �ִϸ��̼��� ��Ÿ����. ("�޸���", "���" ��)
// ��� ���� ���� BoneAnimation�� ����Ʈ
struct AnimationClip
{
    string name;
    float tickPerSecond = 0.0f;
    float duration = 0.0f;
    bool enableRootMotion = false;

    std::map<string, BoneAnimation> boneAnimations;
};

// �� ������ �� �ִϸ��̼ǿ� �󸶳� ������ �޴��� ����ġ�� ����
// ��� ������ �ִ� MAX_NUM_BONES_PER_VERTEX ���� ���� ������ �޴´�.
struct VertexBoneData
{
    int BoneIDs[MAX_NUM_BONES_PER_VERTEX] = { 0 };
    float Weights[MAX_NUM_BONES_PER_VERTEX] = { 0.0f };

    VertexBoneData() {}

    void AddBoneData(int BoneID, float Weight)
    {
        for (int i = 0; i < MAX_NUM_BONES_PER_VERTEX; i++) {
            if (Weights[i] == 0.0) {
                BoneIDs[i] = BoneID;
                Weights[i] = Weight;
                return;
            }
        }

        //assert(0);
    }
};

struct BoneInfo
{
    XMFLOAT4X4 OffsetMatrix;
    XMFLOAT4X4 FinalTransformation;

    BoneInfo(const XMFLOAT4X4& Offset)
    {
        OffsetMatrix = Offset;
        FinalTransformation = XMFLOAT4X4();
    }
};

struct BasicMeshEntry {
    BasicMeshEntry()
    {
        NumIndices = 0;
        BaseVertex = 0;
        BaseIndex = 0;
        MaterialIndex = 0;
    }

    unsigned int NumIndices;
    unsigned int BaseVertex;
    unsigned int BaseIndex;
    unsigned int MaterialIndex;
};

class SkinnedMesh : public Mesh
{
public:
    SkinnedMesh() {};
    ~SkinnedMesh();

    bool LoadMesh(const std::string& Filename);
    bool LoadAnimations(const std::string& Filename);
    void SetOffsetMatrix(XMFLOAT4X4 offsetMatrix) { mOffsetMatrix = offsetMatrix; }
    void SetOffsetMatrix(XMFLOAT3 axis1, float degree1, XMFLOAT3 axis2, float degree2);

    int NumBones() const { return (int)mBoneNameToIndexMap.size(); }

    //const Material& GetMaterial();

    void GetBoneTransforms(float animationTimeSec, vector<XMFLOAT4X4>& transforms, int animationIndex);

    void CreateBlob(const vector<SkinnedVertex>& vertices, const vector<UINT>& indices);
    void UploadBuffer(ID3D12Device* d3dDevice, ID3D12GraphicsCommandList* commandList, const vector<SkinnedVertex> vertices, const vector<UINT> indices);

    vector<VertexBoneData> mBones;

    string rootNodeName;
    map<string, int> mBoneNameToIndexMap;
    map<string, int> mNodeNameToIndexMap;
    vector<pair<string, vector<int>>> mBoneHierarchy;   // �θ�� �ڽ��� �ε����� ������ �ִ�. ��Ʈ�� 0
    vector<pair<string, vector<string>>> mNodeHierarchy;   // �θ�� �ڽ��� �ε����� ������ �ִ�. ��Ʈ�� 0
    vector<BoneInfo> mBoneInfo;

    vector<AnimationClip> mAnimations;
private:
    void Clear();

    bool InitFromScene(const aiScene* pScene, const std::string& Filename);

    void InitAllMeshes(const aiScene* pScene);
    void InitAllAnimations(const aiScene* pScene);
    void InitSingleMesh(int MeshIndex, const aiMesh* paiMesh, int baseVertex, int baseIndex);
    bool InitMaterials(const aiScene* pScene, const std::string& Filename);

    void LoadTextures(const string& Dir, const aiMaterial* pMaterial, int index);
    void LoadDiffuseTexture(const string& Dir, const aiMaterial* pMaterial, int index);
    void LoadSpecularTexture(const string& Dir, const aiMaterial* pMaterial, int index);
    void LoadColors(const aiMaterial* pMaterial, int index);

    void LoadMeshBones(UINT meshID, const aiMesh* pMesh);
    void LoadSingleBone(UINT meshID, const aiBone* pBone);
    void LoadNodeHierarchy(const aiNode* pNode);
    void LoadBoneHierarchy(const aiNode* pNode);
    void LoadChildren(vector<int>& children, const aiNode* pNode);
    int GetBoneId(const aiBone* pBone);     // ���� ���� ���� ���ο� index�� �߰���.
    int GetBoneId(const string boneName);   // ���� ���� ���� -1 ��ȯ
    int GetNodeId(const string nodeName);   // ���� ���� ���� -1 ��ȯ
    //string GetBoneName(const int boneId);

    void CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
    void CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
    void CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);

    int FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim);
    int FindScaleIndex(float AnimationTime, BoneAnimation boneAnimation);
    int FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);

    XMVECTOR CalcInterpolatedScaling(const float animationTime, const BoneAnimation boneAnimation);
    XMVECTOR CalcInterpolatedRotation(const float animationTime, const BoneAnimation boneAnimation);
    XMVECTOR CalcInterpolatedPosition(const float animationTime, const BoneAnimation boneAnimation);

    int FindRotationIndex(float AnimationTime, BoneAnimation boneAnimation);
    int FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim);
    int FindPositionIndex(float AnimationTime, BoneAnimation boneAnimation);
    const aiNodeAnim* FindNodeAnim(const aiAnimation* pAnimation, const string& NodeName);

    //void ReadNodeHierarchy(float AnimationTimeTicks, const aiNode* pNode, const XMMATRIX& ParentTransform);
    void ReadBoneHierarchy(float AnimationTimeTicks, const int boneId, const int animationId, const XMMATRIX& ParentTransform);
    void ReadBoneHierarchy(float AnimationTimeTicks, const string boneName, const int animationId, const XMMATRIX& ParentTransform);
    
    XMFLOAT4X4 mOffsetMatrix = Matrix4x4::Identity();
};

