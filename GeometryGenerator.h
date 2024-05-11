//***************************************************************************************
// GeometryGenerator.h by Frank Luna (C) 2011 All Rights Reserved.
//   
// Defines a static class for procedurally generating the geometry of 
// common mathematical objects.
//
// All triangles are generated "outward" facing.  If you want "inward" 
// facing triangles (for example, if you want to place the camera inside
// a sphere to simulate a sky), you will need to:
//   1. Change the Direct3D cull mode or manually reverse the winding order.
//   2. Invert the normal.
//   3. Update the texture coordinates and tangent vectors.
//***************************************************************************************

#pragma once

#include <cstdint>
#include <DirectXMath.h>
#include <vector>

class GeometryGenerator
{
public:

    using uint16 = std::uint16_t;
    using uint32 = std::uint32_t;

	struct Vertex
	{
		Vertex() {}
        Vertex(
            const DirectX::XMFLOAT3& p, 
            const DirectX::XMFLOAT3& n, 
            const DirectX::XMFLOAT3& t, 
            const DirectX::XMFLOAT2& uv) :
            Position(p), 
            Normal(n), 
            TangentU(t), 
            TexC(uv) {}
		Vertex(
			float px, float py, float pz, 
			float nx, float ny, float nz,
			float tx, float ty, float tz,
			float u, float v) : 
            Position(px, py, pz), 
            Normal(nx, ny, nz),
			TangentU(tx, ty, tz), 
            TexC(u, v) {}

        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT3 Normal;
        DirectX::XMFLOAT3 TangentU;
        DirectX::XMFLOAT2 TexC;
	};

	struct MeshData
	{
		std::vector<Vertex> Vertices;
        std::vector<uint32> Indices32;

        std::vector<uint16>& GetIndices16()
        {
			if(mIndices16.empty())
			{
				mIndices16.resize(Indices32.size());
				for(size_t i = 0; i < Indices32.size(); ++i)
					mIndices16[i] = static_cast<uint16>(Indices32[i]);
			}

			return mIndices16;
        }

	private:
		std::vector<uint16> mIndices16;
	};

	///<summary>
	/// Creates a box centered at the origin with the given dimensions, where each
    /// face has m rows and n columns of vertices.
	///</summary>
    MeshData CreateBox(float width, float height, float depth, uint32 numSubdivisions);

	///<summary>
	/// Creates a sphere centered at the origin with the given radius.  The
	/// slices and stacks parameters control the degree of tessellation.
	///</summary>
    MeshData CreateSphere(float radius, uint32 sliceCount, uint32 stackCount);

	///<summary>
	/// 측지구 메시를 생성한다. numSubdivisions는 테셀레이션 정도를 결정한다.
	/// 정 20면체를 생성한 뒤 삼각형 면 하나를 4개의 삼각형으로 세분화한다.
	/// 새 정점을 반지름이 r인 구에 투영한다.
	/// 이작업을 numSubdivisions만큼 반복하여 측지구를 근사한다.
	///</summary>
    MeshData CreateGeosphere(float radius, uint32 numSubdivisions);

	///<summary>
	/// Y축을 축으로 하는 원기둥을 생성한다. 중심은 원점이다.
	/// sliceCount, stackCount는 테셀레이션 정도를 정한다.
	/// 층층이 싸인 더미들의 개수가 stackCount이며, 각 고리의 정점 개수는 sliceCount이다.
	///</summary>
    MeshData CreateCylinder(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount);

	///<summary>
	/// Creates an mxn grid in the xz-plane with m rows and n columns, centered
	/// at the origin with the specified width and depth.
	///</summary>
    MeshData CreateGrid(float width, float depth, uint32 m, uint32 n);

	///<summary>
	/// Creates a quad aligned with the screen.  This is useful for postprocessing and screen effects.
	///</summary>
    MeshData CreateQuad(float x, float y, float w, float h, float depth);

private:
	void Subdivide(MeshData& meshData);
    Vertex MidPoint(const Vertex& v0, const Vertex& v1);
    void BuildCylinderTopCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount, MeshData& meshData);
    void BuildCylinderBottomCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount, MeshData& meshData);
};

