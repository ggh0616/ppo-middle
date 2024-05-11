#pragma once

//#include "stdafx.h"
#include "d3dApp.h"
#include "MathHelper.h"
#include "UploadBuffer.h"
#include "FrameResource.h"
#include "GeometryGenerator.h"
#include "Terrain.h"
#include "Camera.h"
#include "GameObject.h"
#include "SkinnedMesh.h"
#include "Player.h"
#include "MeshSlice.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

extern const int gNumFrameResources;

enum class RenderLayer : int
{
	Opaque = 0,
	SkinnedOpaque,
	Debug,
	Sky,
	Count
};

class DummyApp : public D3DApp
{
public:

	DummyApp(HINSTANCE hInstance);
	DummyApp(const DummyApp& rhs) = delete;
	DummyApp& operator=(const DummyApp& rhs) = delete;
	~DummyApp();

	virtual bool Initialize()override;

private:

	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	virtual void Draw(const GameTimer& gt)override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;
	virtual bool OnKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

	void OnKeyboardInput(const GameTimer& gt);
	void AnimateMaterials(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateSkinnedCBs(const GameTimer& gt);
	void UpdateMaterialCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);
	
	void LoadTextures();
	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayout();
	void BuildShapeGeometry();
	void LoadSkinnedModel();
	void LoadTerrain();
	void BuildPSOs();
	void BuildFrameResources();
	void BuildMaterials();
	void BuildGameObjects();
	void DrawGameObjects(ID3D12GraphicsCommandList* cmdList, const std::vector<GameObject*>& ritems);

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

private:

	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;

	UINT mCbvSrvDescriptorSize = 0;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

	Terrain mTerrain;
	std::unordered_map<std::string, std::unique_ptr<Mesh>> mMeshes;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mSkinnedInputLayout;

	// List of all the render items.
	//std::vector<std::unique_ptr<RenderItem>> mAllRitems;
	std::vector<std::unique_ptr<GameObject>> mAllGameObjects;

	//std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];
	std::vector<GameObject*> mGameObjectLayer[(int)RenderLayer::Count];

	PassConstants mMainPassCB;
	
	bool mIsWireframe = false;
	bool mIsToonShading = false;

	SkinnedMesh mSkinnedMesh;

	Player* mPlayer = nullptr;

	Camera* mCamera = nullptr;

	POINT mLastMousePos;

	UINT mSkyTexHeapIndex = 0;
};


