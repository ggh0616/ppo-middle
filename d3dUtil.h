//***************************************************************************************
// d3dUtil.h by Frank Luna (C) 2015 All Rights Reserved.
//
// General helper code.
//***************************************************************************************

#pragma once

#include <windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <string>
#include <memory>
#include <algorithm>
#include <vector>
#include <array>
#include <unordered_map>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <cassert>
#include "d3dx12.h"
#include "DDSTextureLoader.h"
#include "MathHelper.h"
//#include "Mesh.h"

extern const int gNumFrameResources;

inline void d3dSetDebugName(IDXGIObject* obj, const char* name)
{
	if (obj)
	{
		obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
	}
}

inline void d3dSetDebugName(ID3D12Device* obj, const char* name)
{
	if (obj)
	{
		obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
	}
}

inline void d3dSetDebugName(ID3D12DeviceChild* obj, const char* name)
{
	if (obj)
	{
		obj->SetPrivateData(WKPDID_D3DDebugObjectName, lstrlenA(name), name);
	}
}

inline std::wstring AnsiToWString(const std::string& str)
{
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return std::wstring(buffer);
}

/*
#if defined(_DEBUG)
	#ifndef Assert
	#define Assert(x, description)                                  \
	{                                                               \
		static bool ignoreAssert = false;                           \
		if(!ignoreAssert && !(x))                                   \
		{                                                           \
			Debug::AssertResult result = Debug::ShowAssertDialog(   \
			(L#x), description, AnsiToWString(__FILE__), __LINE__); \
		if(result == Debug::AssertIgnore)                           \
		{                                                           \
			ignoreAssert = true;                                    \
		}                                                           \
					else if(result == Debug::AssertBreak)           \
		{                                                           \
			__debugbreak();                                         \
		}                                                           \
		}                                                           \
	}
	#endif
#else
	#ifndef Assert
	#define Assert(x, description)
	#endif
#endif
*/

class d3dUtil
{
public:

	static bool IsKeyDown(int vkeyCode);

	static std::string ToString(HRESULT hr);

	static UINT CalcConstantBufferByteSize(UINT byteSize)
	{
		// ����� ������ ũ��� 256�� ������� �Ѵ�.
		return (byteSize + 255) & ~255;
	}

	static Microsoft::WRL::ComPtr<ID3DBlob> LoadBinary(const std::wstring& filename);

	static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		const void* initData,
		UINT64 byteSize,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);

	static Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
		const std::wstring& filename,
		const D3D_SHADER_MACRO* defines,
		const std::string& entrypoint,
		const std::string& target);
};

// ���� ���α׷� ��𼱰� ���и� ���ϴ� HRESULT ���� ��ȯ�Ǹ� ThrowIfFailed�� ���ܸ� ������.
// ���ܴ� catch������ �ö�� MessageBox�� �̿��� ��µȴ�.
class DxException
{
public:
	DxException() = default;
	DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber);

	std::wstring ToString()const;

	HRESULT ErrorCode = S_OK;
	std::wstring FunctionName;
	std::wstring Filename;
	int LineNumber = -1;
};

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    std::wstring wfn = AnsiToWString(__FILE__);                       \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
}
#endif

// HLSL ����ü ä��� ��Ģ�� ����Ͽ� 4���� ���� ������ ä���ִ´�.
struct Light
{
	DirectX::XMFLOAT3 Strength = { 0.5f, 0.5f, 0.5f };	// ���� ����
	float FalloffStart = 1.0f;                          // point, spot light only
	DirectX::XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f };// directional, spot light only
	float FalloffEnd = 10.0f;                           // point, spot light only
	DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };  // point, spot light only
	float SpotPower = 64.0f;                            // spot light only
};

#define MaxLights 16

struct MaterialConstants
{
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 0.25f;

	// Used in texture mapping.
	DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();
};


// ������ ��Ÿ���� �� ���̴� ����ü
struct Material
{
	// ������ ���� �̸�
	std::string Name;

	// �� ������ �ش��ϴ� ��������� ����
	int MatCBIndex = -1;

	// SRV ������ �� ������ �ش��ϴ� diffuse texture�� ����
	int DiffuseSrvHeapIndex = -1;

	// SRV ������ �� ������ �ش��ϴ� normal texture�� ����
	int NormalSrvHeapIndex = -1;

	// ������ ���ؼ� �ش� ��� ���۸� �����ؾ� �ϴ����� ���θ� ��Ÿ���� ������ �÷���
	int NumFramesDirty = gNumFrameResources;

	// ���̵��� ���̴� ���� ��� ���� �ڷ�.
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 0.25f;	// ǥ�� ��ĥ��[0, 1] 0 �Ų�����, 1 ��ģ
	DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();
};

struct Texture
{
	// Unique material name for lookup.
	std::string Name;

	std::wstring Filename;

	Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap = nullptr;
};

#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){ x->Release(); x = 0; } }
#endif

