#pragma once

#include "d3dUtil.h"
#include "MathHelper.h"
#include "UploadBuffer.h"

struct ObjectConstants
{
    DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
    UINT     MaterialIndex;
    UINT     ObjPad0;
    UINT     ObjPad1;
    UINT     ObjPad2;
};

struct SkinnedConstants
{
    DirectX::XMFLOAT4X4 BoneTransforms[96];
};

struct PassConstants
{
    DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
    float cbPerObjectPad1 = 0.0f;
    DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
    DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
    float NearZ = 0.0f;
    float FarZ = 0.0f;
    float TotalTime = 0.0f;
    float DeltaTime = 0.0f;

    DirectX::XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

    // Indices [0, NUM_DIR_LIGHTS) are directional lights;
    // indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
    // indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
    // are spot lights for a maximum of MaxLights per object.
    Light Lights[MaxLights];
};

struct MaterialData
{
    DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
    DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
    float Roughness = 0.5f;

    // Used in texture mapping.
    DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();

    UINT DiffuseMapIndex = 0;
    UINT MaterialPad0;
    UINT MaterialPad1;
    UINT MaterialPad2;
};

struct Vertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT2 TexC;

    // std::hash ���ø� Ư��ȭ
    struct Hash
    {
        size_t operator()(const Vertex& v) const
        {
            size_t hash = 0;

            // Pos �ؽð� ���
            hash_combine(hash, std::hash<float>()(v.Pos.x));
            hash_combine(hash, std::hash<float>()(v.Pos.y));
            hash_combine(hash, std::hash<float>()(v.Pos.z));

            // Normal �ؽð� ���
            /*hash_combine(hash, std::hash<float>()(v.Normal.x));
            hash_combine(hash, std::hash<float>()(v.Normal.y));
            hash_combine(hash, std::hash<float>()(v.Normal.z));*/

            // TexC �ؽð� ���
            /*hash_combine(hash, std::hash<float>()(v.TexC.x));
            hash_combine(hash, std::hash<float>()(v.TexC.y));*/

            return hash;
        }

    private:
        // �ؽ� ���� �Լ�
        template <typename T>
        static void hash_combine(size_t& seed, const T& val)
        {
            seed ^= std::hash<T>()(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
    };

    bool operator==(const Vertex& other) const
    {
        return Pos.x == other.Pos.x &&
            Pos.y == other.Pos.y &&
            Pos.z == other.Pos.z &&
            Normal.x == other.Normal.x &&
            Normal.y == other.Normal.y &&
            Normal.z == other.Normal.z &&
            TexC.x == other.TexC.x &&
            TexC.y == other.TexC.y;
    }
};

struct SkinnedVertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT2 TexC;
    DirectX::XMFLOAT3 BoneWeights;
    BYTE BoneIndices[4];
};

// CPU�� �� �������� ��� ��ϵ��� �����ϴ� �� �ʿ��� �ڿ����� ��ǥ�ϴ� Ŭ����
// ���� ���α׷����� �ʿ��� �ڿ��� �ٸ� ���̹Ƿ�,
// �̷� Ŭ������ ��� ���� ���� ���� ���α׷����� �޶�� �� ���̴�.
struct FrameResource
{
public:
    
    FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT skinnedObjectCount, UINT materialCount);
    FrameResource(const FrameResource& rhs) = delete;
    FrameResource& operator=(const FrameResource& rhs) = delete;
    ~FrameResource();

    // ��� �Ҵ��ڴ� GPU�� ��ɵ��� �� ó���� �� �缳���ؾ� �Ѵ�.
    // ���� �����Ӹ��� �Ҵ��ڰ� �ʿ��ϴ�.
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

    // ��� ���۴� �װ��� �����ϴ� ��ɵ��� GPU�� �� ó���� �Ŀ� �����ؾ��Ѵ�.
    // ���� �����Ӹ��� �Ҵ��ڰ� �ʿ��ϴ�.
    std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
    std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;
    std::unique_ptr<UploadBuffer<SkinnedConstants>> SkinnedCB = nullptr;

    std::unique_ptr<UploadBuffer<MaterialData>> MaterialBuffer = nullptr;

    // Fence�� ���� ��Ÿ�� ���������� ��ɵ��� ǥ���ϴ� ���̴�.
    // �� ���� GPU�� ���� �� ������ �ڿ����� ����ϰ� �ִ��� �����ϴ� �뵵�� ���δ�.
    UINT64 Fence = 0;
};