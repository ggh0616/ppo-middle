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

    // std::hash 템플릿 특수화
    struct Hash
    {
        size_t operator()(const Vertex& v) const
        {
            size_t hash = 0;

            // Pos 해시값 계산
            hash_combine(hash, std::hash<float>()(v.Pos.x));
            hash_combine(hash, std::hash<float>()(v.Pos.y));
            hash_combine(hash, std::hash<float>()(v.Pos.z));

            // Normal 해시값 계산
            /*hash_combine(hash, std::hash<float>()(v.Normal.x));
            hash_combine(hash, std::hash<float>()(v.Normal.y));
            hash_combine(hash, std::hash<float>()(v.Normal.z));*/

            // TexC 해시값 계산
            /*hash_combine(hash, std::hash<float>()(v.TexC.x));
            hash_combine(hash, std::hash<float>()(v.TexC.y));*/

            return hash;
        }

    private:
        // 해시 결합 함수
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

// CPU가 한 프레임의 명령 목록들을 구축하는 데 필요한 자원들을 대표하는 클래스
// 응용 프로그램마다 필요한 자원이 다를 것이므로,
// 이런 클래스의 멤버 구성 역시 응용 프로그램마다 달라야 할 것이다.
struct FrameResource
{
public:
    
    FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT skinnedObjectCount, UINT materialCount);
    FrameResource(const FrameResource& rhs) = delete;
    FrameResource& operator=(const FrameResource& rhs) = delete;
    ~FrameResource();

    // 명령 할당자는 GPU가 명령들을 다 처리한 후 재설정해야 한다.
    // 따라서 프레임마다 할당자가 필요하다.
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

    // 상수 버퍼는 그것을 참조하는 명령들을 GPU가 다 처리한 후에 갱신해야한다.
    // 따라서 프레임마다 할당자가 필요하다.
    std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
    std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;
    std::unique_ptr<UploadBuffer<SkinnedConstants>> SkinnedCB = nullptr;

    std::unique_ptr<UploadBuffer<MaterialData>> MaterialBuffer = nullptr;

    // Fence는 현재 울타리 지점까지의 명령들을 표시하는 값이다.
    // 이 값은 GPU가 아직 이 프레임 자원들을 사용하고 있는지 판정하는 용도로 쓰인다.
    UINT64 Fence = 0;
};