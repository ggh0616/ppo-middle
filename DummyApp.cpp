
#include "DummyApp.h"

const int gNumFrameResources = 3;

DummyApp::DummyApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

DummyApp::~DummyApp()
{
	if (md3dDevice != nullptr)
		FlushCommandQueue();
}

bool DummyApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	// �ʱ�ȭ ����� ���� ��ɸ���� �缳���ϴ�.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// �� �� ������ �ִ� �������� ���� ũ�⸦ �����ɴϴ�. �̴� �ϵ��� ���� �ٸ���.
	mCbvSrvDescriptorSize = md3dDevice->
		GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	LoadTextures();
	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildShapeGeometry();
	LoadSkinnedModel();
	LoadTerrain();
	BuildMaterials();
	BuildGameObjects();
	BuildFrameResources();
	BuildPSOs();

	// �ʱ�ȭ ��� ����
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// �ʱ�ȭ ��ɵ��� ��� ó���Ǳ� ��ٸ���.
	FlushCommandQueue();

	return true;
}

void DummyApp::OnResize()
{
	D3DApp::OnResize();

	// â�� ũ�Ⱑ ����Ǿ��� ������ ��Ⱦ�� �����ϰ�
	// ��������� �ٽ� ����Ѵ�.
	if (!mCamera) {
		mCamera = new Camera;
	}
		
	mCamera->SetLens(0.25 * MathHelper::Pi, AspectRatio());
}

void DummyApp::Update(const GameTimer& gt)
{
	OnKeyboardInput(gt);

	mPlayer->Update(gt);

	// ��ȯ������ �ڿ� ������ �迭�� ���� ���ҿ� �����Ѵ�.
	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	// GPU�� ���� ������ �ڿ��� ��ɵ��� �� ó���ߴ��� Ȯ��
	// ���� �� ó������ �ʾ����� GPU�� �� ��Ÿ�� ���������� ��ɵ��� ó���� ������ ��ٸ���.
	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	// mCurrFrameResource�� �ڿ� ����
	AnimateMaterials(gt);
	UpdateObjectCBs(gt);
	UpdateSkinnedCBs(gt);
	UpdateMaterialCBs(gt);
	UpdateMainPassCB(gt);
}

void DummyApp::Draw(const GameTimer& gt)
{
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	// ��� ��Ͽ� ���õ� �޸��� ��Ȱ���� ���� ��� �Ҵ��� �缳��
	// �缳���� GPU�� ��� ����� ��� ó���� �Ŀ� �Ͼ��.
	ThrowIfFailed(cmdListAlloc->Reset());

	// ��� ����� ExecuteCommandList�� ���ؼ� ��� ��⿭��
	// �߰��ߴٸ� ��� ����� �缳���� �� �ִ�. 
	// ��� ����� �缺���ϸ� �޸𸮰� ��Ȱ��ȴ�.
	if (mIsWireframe)
	{
		ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque_wireframe"].Get()));
	}
	else if (mIsToonShading)
	{
		ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque_toonShading"].Get()));
	}
	else
	{
		ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));
	}

	// ����Ʈ�� ���� ���簢���� �����Ѵ�.
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// �ڿ� �뵵�� ���õ� ���� ���̸� D3D�� �����Ѵ�.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// �ĸ� ����, ���� ���۸� �����.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// ������ ����� ��ϵ� ���� ��� ���۵��� �����Ѵ�.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());



	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

	// �� ��鿡 ���̴� ��� ������ ���´�.
	// ������ ���۴� ���� �����ϰ� �׳� �ϳ��� ��Ʈ �����ڷ� ���� �� �ִ�.
	auto matBuffer = mCurrFrameResource->MaterialBuffer->Resource();
	mCommandList->SetGraphicsRootShaderResourceView(3, matBuffer->GetGPUVirtualAddress());

	CD3DX12_GPU_DESCRIPTOR_HANDLE skyTexDescriptor(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	skyTexDescriptor.Offset(mSkyTexHeapIndex, mCbvSrvDescriptorSize);
	mCommandList->SetGraphicsRootDescriptorTable(4, skyTexDescriptor);


	// �� ��鿡 ���̴� ��� �ؽ�ó�� ���´�. 
	// ���̺��� ù �����ڸ� �����ϸ� �ȴ�.
	// ���̺� �� ���� �����ڰ� �ִ����� ��Ʈ ���� �����Ǿ� �ִ�.
	mCommandList->SetGraphicsRootDescriptorTable(5, mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	DrawGameObjects(mCommandList.Get(), mGameObjectLayer[(int)RenderLayer::Opaque]);

	mCommandList->SetPipelineState(mPSOs["skinnedOpaque"].Get());
	DrawGameObjects(mCommandList.Get(), mGameObjectLayer[(int)RenderLayer::SkinnedOpaque]);

	mCommandList->SetPipelineState(mPSOs["sky"].Get());
	DrawGameObjects(mCommandList.Get(), mGameObjectLayer[(int)RenderLayer::Sky]);


	// �ڿ� �뵵�� ���õ� ���� ���̸� D3D�� �����Ѵ�.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// ��� ����� ��ģ��.
	ThrowIfFailed(mCommandList->Close());

	// ��� ������ ���� ��� ����� ��� ��⿭�� �߰��Ѵ�.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// �ĸ���ۿ� ������۸� �ٲ۴�.
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// ���� ��Ÿ�� ���������� ��ɵ��� ǥ���ϵ��� ��Ÿ�� ���� ������Ų��.
	mCurrFrameResource->Fence = ++mCurrentFence;

	// �� ��Ÿ�� ������ �����ϴ� ����� ��ɴ�⿭�� �߰��Ѵ�.
	// ���� �츮�� GPU �ð��� �� �����Ƿ�, ����Ÿ�� ������ GPU�� �� Signal() 
	// ��ɱ����� ��� ����� ó���ϱ� �������� �������� �ʴ´�.
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void DummyApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void DummyApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void DummyApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		mPlayer->MouseInput(dx, dy);

		//mCamera->Pitch(dy);
		//mCamera->RotateY(dx);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

bool DummyApp::OnKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	mPlayer->OnKeyboardMessage(nMessageID, wParam);

	switch (nMessageID)
	{
	case WM_KEYDOWN:
		switch (wParam)
		{
		case '1':
			mIsWireframe = false;
			mIsToonShading = false;
			return(false);
		case '2':
			mIsWireframe = true;
			mIsToonShading = false;
			return(false);
		case '3':
			mIsWireframe = false;
			mIsToonShading = true;
			return(false);
		case 'F':
			int a = 5;
			XMFLOAT3 position;
			XMStoreFloat3(&position, XMLoadFloat3(&mPlayer->GetPosition()) + XMLoadFloat3(&mPlayer->GetLook()) * 100.f);
			auto gameObject = std::make_unique<GameObject>("box", XMMatrixScaling(50.f, 50.f, 50.f) * XMMatrixTranslation(position.x, position.y + 130.f, position.z), XMMatrixIdentity());
			gameObject->SetCBIndex(a);
			gameObject->SetMesh(mMeshes["shapeGeo"].get());
			gameObject->SetMaterial(mMaterials["tile0"].get());
			gameObject->AddSubmesh(gameObject->GetMesh()->GetSubmesh("box"));

			gameObject->SetFrameDirty();

			mGameObjectLayer[(int)RenderLayer::Opaque].push_back(gameObject.get());

			mAllGameObjects.push_back(std::move(gameObject));


			vector<vector<Vertex>> vertices;
			vector<vector<UINT>> indices;
			int numMeshes = MeshSlice::MeshCompleteSlice(mMeshes["shapeGeo"].get(), mMeshes["shapeGeo"].get()->mSubmeshes[0], XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f), vertices, indices);

			// �ʱ�ȭ ����� ���� ��ɸ���� �缳���ϴ�.
			ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
			for (int i = 0; i < numMeshes; i++)
			{
				const UINT vbByteSize = (UINT)vertices[i].size() * sizeof(Vertex);
				const UINT ibByteSize = (UINT)indices[i].size() * sizeof(UINT);

				auto geo = std::make_unique<Mesh>();
				geo->mName = "slicingMesh" + to_string(i);

				geo->CreateBlob
				(vertices[i], indices[i]);
				geo->UploadBuffer(md3dDevice.Get(), mCommandList.Get(), vertices[i], indices[i]);

				Submesh submesh;
				submesh.name = "box";
				submesh.baseVertex = 0;
				submesh.baseIndex = 0;
				submesh.numIndices = indices[i].size();
				geo->mSubmeshes.push_back(submesh);

				mMeshes[geo->mName] = std::move(geo);
			}
			// �ʱ�ȭ ��� ����
			ThrowIfFailed(mCommandList->Close());
			ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
			mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

			// �ʱ�ȭ ��ɵ��� ��� ó���Ǳ� ��ٸ���.
			FlushCommandQueue();

			for (int i = 0; i < 2; i++)
			{
				auto gameObject = std::make_unique<GameObject>("box", XMMatrixScaling(50.f, 50.f, 50.f) * XMMatrixTranslation(position.x + (i + 1) * 150.f, position.y + 130.f, position.z), XMMatrixIdentity());
				gameObject->SetCBIndex(a);
				string meshName = "slicingMesh" + to_string(i);
				gameObject->SetMesh(mMeshes[meshName].get());
				gameObject->SetMaterial(mMaterials["tile0"].get());
				gameObject->AddSubmesh(gameObject->GetMesh()->GetSubmesh("box"));

				gameObject->SetFrameDirty();

				mGameObjectLayer[(int)RenderLayer::Opaque].push_back(gameObject.get());
				mAllGameObjects.push_back(std::move(gameObject));
			}
			break;
		}
		return(false);
	}

	return(false);
}

void DummyApp::OnKeyboardInput(const GameTimer& gt)
{
	const float dt = gt.DeltaTime();

	mPlayer->KeyboardInput(dt);

	//mCamera->UpdateViewMatrix();
}

void DummyApp::AnimateMaterials(const GameTimer& gt)
{
}

void DummyApp::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();

	for (auto& e : mAllGameObjects)
	{
		// ������� �ٲ���� ������ cbuffer �ڷḦ �����Ѵ�.
		// �̷��� ������ ������ �ڿ����� �����ؾ��Ѵ�.
		if (e->GetFramesDirty() > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&e->GetWorld());
			XMMATRIX texTransform = XMLoadFloat4x4(&e->GetTexTransform());

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

			for (UINT i = 0; i < e->GetNumSubmeshes(); i++)
			{
				objConstants.MaterialIndex = e->GetMeterial(i)->MatCBIndex; 
				currObjectCB->CopyData(e->GetObjCBIndex(i), objConstants);
			}

			// ���� ������ �ڿ����� �Ѿ��.
			e->DecreaseFrameDirty();
		}
	}
}

void DummyApp::UpdateSkinnedCBs(const GameTimer& gt)
{
	auto currSkinnedCB = mCurrFrameResource->SkinnedCB.get();

	std::vector<XMFLOAT4X4> boneTransforms;
	mSkinnedMesh.GetBoneTransforms(mPlayer->GetAnimationTime(), boneTransforms, mPlayer->GetAnimationIndex());
	SkinnedConstants skinnedConstants;

	int numBones = boneTransforms.size();
	for (int i = 0; i < numBones; i++)
	{
		skinnedConstants.BoneTransforms[i] = boneTransforms[i];
	}
	// �ּ��� 4���� ����� �ʱ�ȭ��
	if (numBones < 4) {
		for (int i = numBones; i < 4; i++)
			skinnedConstants.BoneTransforms[i] = Matrix4x4::Identity();
	}

	currSkinnedCB->CopyData(0, skinnedConstants);
}

void DummyApp::UpdateMaterialCBs(const GameTimer& gt)
{
	auto currMaterialCB = mCurrFrameResource->MaterialBuffer.get();
	for (auto& e : mMaterials)
	{
		// ������� �ٲ���� ������ cbuffer �ڷḦ �����Ѵ�.
		// �̷��� ������ ������ �ڿ����� �����ؾ��Ѵ�.
		Material* mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialData matData;
			matData.DiffuseAlbedo = mat->DiffuseAlbedo;
			matData.FresnelR0 = mat->FresnelR0;
			matData.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matData.MatTransform, XMMatrixTranspose(matTransform));
			matData.DiffuseMapIndex = mat->DiffuseSrvHeapIndex;

			currMaterialCB->CopyData(mat->MatCBIndex, matData);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}

void DummyApp::UpdateMainPassCB(const GameTimer& gt)
{
	XMMATRIX view = mCamera->GetView();
	XMMATRIX proj = mCamera->GetProj();

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	mMainPassCB.EyePosW = mCamera->GetPosition3f();
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = mCamera->GetNearZ();
	mMainPassCB.FarZ = mCamera->GetFarZ();
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();

	mMainPassCB.AmbientLight = { 0.2f, 0.2f, 0.2f, 1.0f };

	mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[0].Strength = { 0.8f, 0.8f, 0.8f };
	mMainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[1].Strength = { 0.4f, 0.4f, 0.4f };
	mMainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	mMainPassCB.Lights[2].Strength = { 0.2f, 0.2f, 0.2f };
	
	/*
	for (int i = 0; i < 5; i++)
	{
		float fallOffStart = 2.0f;
		float fallOffEnd = 10.0f;
		XMFLOAT3 strength = { 0.7f, 0.7f, 0.7f };

		mMainPassCB.Lights[2 * i].Strength = strength;
		mMainPassCB.Lights[2 * i].FalloffStart = fallOffStart;
		mMainPassCB.Lights[2 * i].FalloffEnd = fallOffEnd;
		mMainPassCB.Lights[2 * i].Position = { -5.0f, 3.5f, -10.0f + 5.0f * i };

		mMainPassCB.Lights[2 * i + 1].Strength = strength;
		mMainPassCB.Lights[2 * i + 1].FalloffStart = fallOffStart;
		mMainPassCB.Lights[2 * i + 1].FalloffEnd = fallOffEnd;
		mMainPassCB.Lights[2 * i + 1].Position = { +5.0f, 3.5f, -10.0f + 5.0f * i };
	}
	*/

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void DummyApp::LoadTextures()
{
	std::vector<std::string> texNames =
	{
		"missing",
		"bricksDiffuseMap",
		"stoneDiffuseMap",
		"tileDiffuseMap",
		"terrainDiffuseMap",
		"skyCubeMap"
	};
	
	std::vector<std::wstring> texFilenames =
	{
		L"Textures/Character Texture.dds",
		L"Textures/bricks.dds",
		L"Textures/stone.dds",
		L"Textures/tile.dds",
		L"Textures/terrainColorMap.dds",
		L"Textures/grasscube1024.dds"
	};

	for (int i = 0; i < (int)texNames.size(); ++i)
	{
		// ���� �̸��� �ؽ�ó�� ������ �ʵ����Ѵ�.
		if (mTextures.find(texNames[i]) == std::end(mTextures))
		{
			auto texMap = std::make_unique<Texture>();
			texMap->Name = texNames[i];
			texMap->Filename = texFilenames[i];
			ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
				mCommandList.Get(), texMap->Filename.c_str(),
				texMap->Resource, texMap->UploadHeap));

			mTextures[texMap->Name] = std::move(texMap);
		}
	}
}

void DummyApp::BuildRootSignature()
{
	// �Ϲ������� ���̴� ���α׷��� Ư�� �ڿ���(��� ����, �ؽ�ó, ǥ������� ��)�� �Էµȴٰ� ����Ѵ�.
	// ��Ʈ ������ ���̴� ���α׷��� ����ϴ� �ڿ����� �����Ѵ�.
	// ���̴� ���α׷��� ���������� �ϳ��� �Լ��̰� ���̴��� �ԷµǴ� �ڿ����� 
	// �Լ��� �Ű������鿡 �ش��ϹǷ�, ��Ʈ ������ �� �Լ� ������ �����ϴ� �����̶� �� �� �ִ�.

	CD3DX12_DESCRIPTOR_RANGE texTable0;
	texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

	CD3DX12_DESCRIPTOR_RANGE texTable1;
	texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 1, 0);

	// ��Ʈ �Ű������� ������ ���̺��̰ų� ��Ʈ ������ �Ǵ� ��Ʈ ����̴�.
	CD3DX12_ROOT_PARAMETER slotRootParameter[6];

	// ��Ʈ CBV �����Ѵ�.
	// ���� ��: ���󵵰� �����Ϳ��� ������ ������� �迭�Ѵ�.
	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsConstantBufferView(1);
	slotRootParameter[2].InitAsConstantBufferView(2);
	slotRootParameter[3].InitAsShaderResourceView(0, 1);
	slotRootParameter[4].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[5].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL);

	auto staticSamplers = GetStaticSamplers();

	// ��Ʈ ������ ��Ʈ �Ű��������� �迭�̴�.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(6, slotRootParameter, 
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// ��� ���� �ϳ��� ������ ������ ������ ����Ű��
	// ���� �ϳ��� �̷���� ��Ʈ ������ �����Ѵ�.
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void DummyApp::BuildDescriptorHeaps()
{
	// CBV, SRV, UAV�� �����Ҽ��ְ�, ���̴����� ������ �� �ִ� ���� ����
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 6;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	// �ؽ�ó �ڿ��� �̹� �ε�Ǿ� ������

	// ���� ������ ����Ű�� �����͸� ��´�.
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	auto missingTex = mTextures["missing"]->Resource;
	auto bricksTex = mTextures["bricksDiffuseMap"]->Resource;
	auto stoneTex = mTextures["stoneDiffuseMap"]->Resource;
	auto tileTex = mTextures["tileDiffuseMap"]->Resource;
	auto terrainTex = mTextures["terrainDiffuseMap"]->Resource;
	auto skyTex = mTextures["skyCubeMap"]->Resource;

	// �ؽ�ó�� ���� ���� �����ڵ��� �տ��� ������ ���� �����Ѵ�.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = missingTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = missingTex->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	md3dDevice->CreateShaderResourceView(missingTex.Get(), &srvDesc, hDescriptor);

	// ���� �����ڷ� �Ѿ��.
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = bricksTex->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = bricksTex->GetDesc().MipLevels;
	md3dDevice->CreateShaderResourceView(bricksTex.Get(), &srvDesc, hDescriptor);

	// ���� �����ڷ� �Ѿ��.
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = stoneTex->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = stoneTex->GetDesc().MipLevels;
	md3dDevice->CreateShaderResourceView(stoneTex.Get(), &srvDesc, hDescriptor);

	// ���� �����ڷ� �Ѿ��.
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = tileTex->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = tileTex->GetDesc().MipLevels;
	md3dDevice->CreateShaderResourceView(tileTex.Get(), &srvDesc, hDescriptor);

	// ���� �����ڷ� �Ѿ��.
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = terrainTex->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = terrainTex->GetDesc().MipLevels;
	md3dDevice->CreateShaderResourceView(terrainTex.Get(), &srvDesc, hDescriptor);

	// ���� �����ڷ� �Ѿ��. skyCubeMap
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = skyTex->GetDesc().MipLevels;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	srvDesc.Format = skyTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(skyTex.Get(), &srvDesc, hDescriptor);

	mSkyTexHeapIndex = 5;
}

void DummyApp::BuildShadersAndInputLayout()
{
	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO skinnedDefines[] =
	{
		"SKINNED", "1",
		NULL, NULL
	};

	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "PS", "ps_5_1");
	mShaders["toonLightingOpaquePS"] = d3dUtil::CompileShader(L"Shaders\\ToonLighting.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["skinnedVS"] = d3dUtil::CompileShader(L"Shaders\\Default.hlsl", skinnedDefines, "VS", "vs_5_1");


	mShaders["skyVS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["skyPS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "PS", "ps_5_1");

	mInputLayout = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	mSkinnedInputLayout = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		/*
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "WEIGHTS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONEINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, 56, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }*/
		{ "WEIGHTS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONEINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void DummyApp::BuildShapeGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(1.0f, 1.0f, 1.0f, 0);
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(20.0f, 30.0f, 60, 40);
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
	GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);

	//
	// We are concatenating all the geometry into one big vertex/index buffer.  So
	// define the regions in the buffer each submesh covers.
	//

	// Cache the vertex offsets to each object in the concatenated vertex buffer.
	UINT boxVertexOffset = 0;
	UINT gridVertexOffset = (UINT)box.Vertices.size();
	UINT sphereVertexOffset = gridVertexOffset + (UINT)grid.Vertices.size();
	UINT cylinderVertexOffset = sphereVertexOffset + (UINT)sphere.Vertices.size();

	// Cache the starting index for each object in the concatenated index buffer.
	UINT boxIndexOffset = 0;
	UINT gridIndexOffset = (UINT)box.Indices32.size();
	UINT sphereIndexOffset = gridIndexOffset + (UINT)grid.Indices32.size();
	UINT cylinderIndexOffset = sphereIndexOffset + (UINT)sphere.Indices32.size();

	Submesh boxSubmesh;
	boxSubmesh.name = string("box");
	boxSubmesh.numIndices = (UINT)box.Indices32.size();
	boxSubmesh.baseIndex = boxIndexOffset;
	boxSubmesh.baseVertex = boxVertexOffset;

	Submesh gridSubmesh;
	gridSubmesh.name = string("grid");
	gridSubmesh.numIndices = (UINT)grid.Indices32.size();
	gridSubmesh.baseIndex = gridIndexOffset;
	gridSubmesh.baseVertex = gridVertexOffset;

	Submesh sphereSubmesh;
	sphereSubmesh.name = string("sphere");
	sphereSubmesh.numIndices = (UINT)sphere.Indices32.size();
	sphereSubmesh.baseIndex = sphereIndexOffset;
	sphereSubmesh.baseVertex = sphereVertexOffset;

	Submesh cylinderSubmesh;
	cylinderSubmesh.name = string("cylinder");
	cylinderSubmesh.numIndices = (UINT)cylinder.Indices32.size();
	cylinderSubmesh.baseIndex = cylinderIndexOffset;
	cylinderSubmesh.baseVertex = cylinderVertexOffset;

	//
	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	//

	auto totalVertexCount =
		box.Vertices.size() +
		grid.Vertices.size() +
		sphere.Vertices.size() +
		cylinder.Vertices.size();

	std::vector<Vertex> vertices(totalVertexCount);

	UINT k = 0;
	for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = box.Vertices[i].Position;
		vertices[k].Normal = box.Vertices[i].Normal;
		vertices[k].TexC = box.Vertices[i].TexC;
	}

	for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = grid.Vertices[i].Position;
		vertices[k].Normal = grid.Vertices[i].Normal;
		vertices[k].TexC = grid.Vertices[i].TexC;
	}

	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = sphere.Vertices[i].Position;
		vertices[k].Normal = sphere.Vertices[i].Normal;
		vertices[k].TexC = sphere.Vertices[i].TexC;
	}

	for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cylinder.Vertices[i].Position;
		vertices[k].Normal = cylinder.Vertices[i].Normal;
		vertices[k].TexC = cylinder.Vertices[i].TexC;
	}

	std::vector<UINT> indices;
	indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
	indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
	indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
	indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(UINT);

	auto geo = std::make_unique<Mesh>();
	geo->mName = "shapeGeo";
	
	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->mVertexBufferCPU));
	CopyMemory(geo->mVertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->mIndexBufferCPU));
	CopyMemory(geo->mIndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->mVertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->mVertexBufferUploader);

	geo->mIndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->mIndexBufferUploader);

	geo->mVertexByteStride = sizeof(Vertex);
	geo->mVertexBufferByteSize = vbByteSize;
	geo->mIndexFormat = DXGI_FORMAT_R32_UINT;
	geo->mIndexBufferByteSize = ibByteSize;

	geo->mSubmeshes.resize(4);
	geo->mSubmeshes[0] = boxSubmesh;
	geo->mSubmeshes[1] = gridSubmesh;
	geo->mSubmeshes[2] = sphereSubmesh;
	geo->mSubmeshes[3] = cylinderSubmesh;

	mMeshes[geo->mName] = std::move(geo);
}

void DummyApp::LoadSkinnedModel()
{
	//mSkinnedMesh.LoadMesh("Models/model.dae");
	
	mSkinnedMesh.SetOffsetMatrix(XMFLOAT3(0.0f, 1.0f, 0.0f), 180.f, XMFLOAT3(1.0f, 0.0f, 0.0f), -90.f);
	mSkinnedMesh.LoadMesh("Models/SKM_Quinn_Simple.FBX");
	mSkinnedMesh.LoadAnimations("Models/MF_Idle.FBX");
	mSkinnedMesh.LoadAnimations("Models/MF_Walk.FBX");
	mSkinnedMesh.LoadAnimations("Models/MF_Run.FBX");
	mSkinnedMesh.LoadAnimations("Models/MM_Jump.FBX");
	mSkinnedMesh.LoadAnimations("Models/MM_Fall.FBX");
	mSkinnedMesh.LoadAnimations("Models/MM_Land.FBX");

	UINT vcount = 0;
	UINT tcount = 0;
	std::vector<SkinnedVertex> vertices;
	std::vector<UINT> indices;
	UINT index;
	UINT dindex = 0;

	UINT numVertices = mSkinnedMesh.mPositions.size();
	for (int j = 0; j < numVertices; j++)
	{
		SkinnedVertex vertex;
		vertex.Pos.x = mSkinnedMesh.mPositions[j].x;
		vertex.Pos.y = mSkinnedMesh.mPositions[j].y;
		vertex.Pos.z = mSkinnedMesh.mPositions[j].z;

		vertex.Normal.x = mSkinnedMesh.mNormals[j].x;
		vertex.Normal.y = mSkinnedMesh.mNormals[j].y;
		vertex.Normal.z = mSkinnedMesh.mNormals[j].z;

		vertex.TexC.x = mSkinnedMesh.mTexCoords[j].x;
		vertex.TexC.y = mSkinnedMesh.mTexCoords[j].y;

		vertices.push_back(vertex);
	}

	for (int i = 0; i < numVertices; i++)
	{
		vertices[i].BoneIndices[0] = (BYTE)mSkinnedMesh.mBones[i].BoneIDs[0];
		vertices[i].BoneIndices[1] = (BYTE)mSkinnedMesh.mBones[i].BoneIDs[1];
		vertices[i].BoneIndices[2] = (BYTE)mSkinnedMesh.mBones[i].BoneIDs[2];
		vertices[i].BoneIndices[3] = (BYTE)mSkinnedMesh.mBones[i].BoneIDs[3];

		float weights = mSkinnedMesh.mBones[i].Weights[0] + mSkinnedMesh.mBones[i].Weights[1] + mSkinnedMesh.mBones[i].Weights[2] + mSkinnedMesh.mBones[i].Weights[3];

		vertices[i].BoneWeights.x = mSkinnedMesh.mBones[i].Weights[0] / weights;
		vertices[i].BoneWeights.y = mSkinnedMesh.mBones[i].Weights[1] / weights;
		vertices[i].BoneWeights.z = mSkinnedMesh.mBones[i].Weights[2] / weights;
	}

	UINT numIndices = mSkinnedMesh.mIndices.size();
	for (UINT i = 0; i < numIndices; i++)
	{
		indices.push_back(mSkinnedMesh.mIndices[i]);
	}

	//
	// Pack the indices of all the meshes into one index buffer.

	auto geo = std::make_unique<SkinnedMesh>();
	geo->mName = "skullGeo";

	geo->CreateBlob(vertices, indices);
	geo->UploadBuffer(md3dDevice.Get(), mCommandList.Get(), vertices, indices);

	Submesh submesh1 = mSkinnedMesh.mSubmeshes[0];
	Submesh submesh2 = mSkinnedMesh.mSubmeshes[1];

	geo->mSubmeshes.push_back(submesh1);
	geo->mSubmeshes.push_back(submesh2);

	mMeshes[geo->mName] = std::move(geo);
}

void DummyApp::LoadTerrain()
{
	mTerrain.LoadHeightMap(L"HeightMap/heightmap.r16", 1025, 1025, 0.02f);
	
	UINT vcount = 1025 * 1025;
	UINT tcount = 1024 * 1024 * 2 * 3;
	
	//
	// Pack the indices of all the meshes into one index buffer.
	//
	
	std::vector<Vertex> vertices(vcount);
	std::vector<uint32_t> indices(tcount);

	mTerrain.CreateTerrain(4000.0f, 4000.f, vertices, indices);

	auto geo = std::make_unique<Mesh>();
	geo->mName = "terrain";

	geo->CreateBlob(vertices, indices);
	geo->UploadBuffer(md3dDevice.Get(), mCommandList.Get(), vertices, indices);

	geo->AddSubmesh("terrain", indices.size());
	
	mMeshes[geo->mName] = std::move(geo);
}

void DummyApp::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	//
	// PSO for opaque objects.
	//
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, 
		IID_PPV_ARGS(&mPSOs["opaque"])));

	//
	// PSO for opaque wireframe objects.
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePsoDesc = opaquePsoDesc;
	opaqueWireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaqueWireframePsoDesc, 
		IID_PPV_ARGS(&mPSOs["opaque_wireframe"])));
	
	//
	// PSO for skinned pass.
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC skinnedOpaquePsoDesc = opaquePsoDesc;
	skinnedOpaquePsoDesc.InputLayout = { mSkinnedInputLayout.data(), (UINT)mSkinnedInputLayout.size() };
	skinnedOpaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["skinnedVS"]->GetBufferPointer()),
		mShaders["skinnedVS"]->GetBufferSize()
	};
	skinnedOpaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&skinnedOpaquePsoDesc, IID_PPV_ARGS(&mPSOs["skinnedOpaque"])));

	//
	// PSO for toon shading.
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC toonShadingPsoDesc = opaquePsoDesc;
	toonShadingPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["toonLightingOpaquePS"]->GetBufferPointer()),
		mShaders["toonLightingOpaquePS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&toonShadingPsoDesc,
		IID_PPV_ARGS(&mPSOs["opaque_toonShading"])));

	//
	// PSO for sky.
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC skyPsoDesc = opaquePsoDesc;

	// ī�޶� ��ī�� �ڽ� �ȿ� �ֱ⶧���� �ø��� ��Ȱ��ȭ�Ѵ�.
	skyPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	// ���� ���� ����� ���� ���̺񱳸� LESS_EQUAL�� �����Ѵ�.
	// LESS�� �ƴ� ����: ���� ���� ���۸� 1�� ����� ��� z = 1���� ����ȭ�� ���̰��� ���� ������ �����Ѵ�.
	skyPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	skyPsoDesc.pRootSignature = mRootSignature.Get();
	skyPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["skyVS"]->GetBufferPointer()),
		mShaders["skyVS"]->GetBufferSize()
	};
	skyPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["skyPS"]->GetBufferPointer()),
		mShaders["skyPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&skyPsoDesc, IID_PPV_ARGS(&mPSOs["sky"])));
}

void DummyApp::BuildFrameResources()
{
	UINT numObjCBs = 0;
	for (auto& gameObj : mAllGameObjects)
		numObjCBs += gameObj.get()->GetNumSubmeshes();

	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
			1, 
			numObjCBs + 10,
			(UINT)mGameObjectLayer[(int)RenderLayer::SkinnedOpaque].size(), // skinned obj
			(UINT)mMaterials.size()));
	}
}

void DummyApp::BuildMaterials()
{
	int matCBIndex = 0;

	auto missing = std::make_unique<Material>();
	missing->Name = "missing";
	missing->MatCBIndex = matCBIndex++;
	missing->DiffuseSrvHeapIndex = 0;
	missing->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	missing->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	missing->Roughness = 1.0f;

	auto bricks0 = std::make_unique<Material>();
	bricks0->Name = "bricks0";
	bricks0->MatCBIndex = matCBIndex++;
	bricks0->DiffuseSrvHeapIndex = 1;
	bricks0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	bricks0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	bricks0->Roughness = 0.1f;

	auto stone0 = std::make_unique<Material>();
	stone0->Name = "stone0";
	stone0->MatCBIndex = matCBIndex++;
	stone0->DiffuseSrvHeapIndex = 2;
	stone0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	stone0->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	stone0->Roughness = 0.3f;

	auto tile0 = std::make_unique<Material>();
	tile0->Name = "tile0";
	tile0->MatCBIndex = matCBIndex++;
	tile0->DiffuseSrvHeapIndex = 3;
	tile0->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	tile0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	tile0->Roughness = 0.3f;

	auto skullMat = std::make_unique<Material>();
	skullMat->Name = "skullMat";
	skullMat->MatCBIndex = matCBIndex++;
	skullMat->DiffuseSrvHeapIndex = 4;
	skullMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	skullMat->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	skullMat->Roughness = 0.3f;

	auto terrainMat = std::make_unique<Material>();
	terrainMat->Name = "terrainMat";
	terrainMat->MatCBIndex = matCBIndex++;
	terrainMat->DiffuseSrvHeapIndex = 4;
	terrainMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	terrainMat->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	terrainMat->Roughness = 0.05f;

	auto sky = std::make_unique<Material>();
	sky->Name = "sky";
	sky->MatCBIndex = matCBIndex++;
	sky->DiffuseSrvHeapIndex = mSkyTexHeapIndex;
	sky->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	sky->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	sky->Roughness = 1.0f;

	mMaterials["missing"] = std::move(missing);
	mMaterials["bricks0"] = std::move(bricks0);
	mMaterials["stone0"] = std::move(stone0);
	mMaterials["tile0"] = std::move(tile0);
	mMaterials["skullMat"] = std::move(skullMat);
	mMaterials["terrainMat"] = std::move(terrainMat);
	mMaterials["sky"] = std::move(sky);
}

void DummyApp::BuildGameObjects()
{
	int objCBIndex = 0, skinnedCBIndex = 0;
	Submesh submesh;

	// ------------------------------------------
	// sky sphere
	// ------------------------------------------
	auto skyGameObject = std::make_unique<GameObject>("sky", XMMatrixIdentity(), XMMatrixIdentity());
	skyGameObject->SetCBIndex(objCBIndex);
	skyGameObject->SetMesh(mMeshes["shapeGeo"].get());
	skyGameObject->SetMaterial(mMaterials["sky"].get());
	skyGameObject->AddSubmesh(skyGameObject->GetMesh()->GetSubmesh("sphere"));

	mGameObjectLayer[(int)RenderLayer::Sky].push_back(skyGameObject.get());
	mAllGameObjects.push_back(std::move(skyGameObject));

	// ------------------------------------------
	// terrain
	// ------------------------------------------
	auto terrainGameObject = std::make_unique<GameObject>("terrain", XMMatrixTranslation(0.0f, -600.0f, 0.0f), XMMatrixIdentity());
	terrainGameObject->SetCBIndex(objCBIndex);
	terrainGameObject->SetMesh(mMeshes["terrain"].get());
	terrainGameObject->SetMaterial(mMaterials["terrainMat"].get());
	terrainGameObject->AddSubmesh(terrainGameObject->GetMesh()->GetSubmesh("terrain"));

	mGameObjectLayer[(int)RenderLayer::Opaque].push_back(terrainGameObject.get());
	mAllGameObjects.push_back(std::move(terrainGameObject));

	// ------------------------------------------
	// Opaque objects - player
	// ------------------------------------------
	/*auto player = std::make_unique<Player>("player", XMMatrixScaling(10.0f, 10.0f, 10.0f) * XMMatrixTranslation(0.0f, 0.0f, -100.0f), XMMatrixIdentity());
	player->SetCBIndex(objCBIndex);
	player->SetMesh(mMeshes["shapeGeo"].get());
	player->SetMaterial(mMaterials["bricks0"].get());
	player->AddSubmesh(player->GetMesh()->GetSubmesh("box"));

	mPlayer = player.get();
	mCamera = mPlayer->GetCamera();
	mCamera->SetLens(0.25f * MathHelper::Pi, AspectRatio());

	mGameObjectLayer[(int)RenderLayer::Opaque].push_back(player.get());
	mAllGameObjects.push_back(std::move(player));*/

	// ------------------------------------------
	// Opaque objects
	// ------------------------------------------
	auto gridGameObject = std::make_unique<GameObject>("grid", XMMatrixScaling(10.0f, 1.0f, 10.0f) * XMMatrixTranslation(100.0f, 0.0f, 0.0f), XMMatrixScaling(3.0f, 4.0f, 1.0f));
	gridGameObject->SetCBIndex(objCBIndex);
	gridGameObject->SetMesh(mMeshes["shapeGeo"].get());
	gridGameObject->SetMaterial(mMaterials["tile0"].get());
	gridGameObject->AddSubmesh(gridGameObject->GetMesh()->GetSubmesh("grid"));

	mGameObjectLayer[(int)RenderLayer::Opaque].push_back(gridGameObject.get());
	mAllGameObjects.push_back(std::move(gridGameObject));

	// ------------------------------------------
	// Skinned objects - player
	// ------------------------------------------
	auto SkinnedGameObject = std::make_unique<Player>("skinned1", XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranslation(0.0f, 0.0f, 0.0f), XMMatrixIdentity());
	SkinnedGameObject->SetCBIndex(2, objCBIndex, skinnedCBIndex);
	SkinnedGameObject->SetMesh(mMeshes["skullGeo"].get());
	SkinnedGameObject->SetMaterials(2, { mMaterials["bricks0"].get(),  mMaterials["tile0"].get() });
	SkinnedGameObject->AddSubmesh(SkinnedGameObject->GetMesh()->mSubmeshes[0]);
	SkinnedGameObject->AddSubmesh(SkinnedGameObject->GetMesh()->mSubmeshes[1]);

	mPlayer = SkinnedGameObject.get();
	mCamera = mPlayer->GetCamera();
	mCamera->SetLens(0.25 * MathHelper::Pi, AspectRatio());

	mGameObjectLayer[(int)RenderLayer::SkinnedOpaque].push_back(SkinnedGameObject.get());
	mAllGameObjects.push_back(std::move(SkinnedGameObject));
}

void DummyApp::DrawGameObjects(ID3D12GraphicsCommandList* cmdList, const std::vector<GameObject*>& gameObjects)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT skinnedCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(SkinnedConstants));

	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	auto skinnedCB = mCurrFrameResource->SkinnedCB->Resource();

	// �� �����׸� ����:
	for (UINT i = 0; i < gameObjects.size(); ++i)
	{
		auto gameObj = gameObjects[i];

		cmdList->IASetVertexBuffers(0, 1, &gameObj->GetMesh()->VertexBufferView());
		cmdList->IASetIndexBuffer(&gameObj->GetMesh()->IndexBufferView());
		cmdList->IASetPrimitiveTopology(gameObj->GetPrimitiveType());

		if (gameObj->GetSkinnedCBIndex() != -1) {
			D3D12_GPU_VIRTUAL_ADDRESS skinnedCBAddress = skinnedCB->GetGPUVirtualAddress() + 0/*gameObj->GetSkinnedCBIndex()*/ * skinnedCBByteSize;
			cmdList->SetGraphicsRootConstantBufferView(1, skinnedCBAddress);
		}
		else {
			cmdList->SetGraphicsRootConstantBufferView(1, 0);
		}

		for (UINT j = 0; j < gameObj->GetNumSubmeshes(); j++)
		{
			// ���� ������ �ڿ��� ���� �� ��ü�� ���� CBV�� �������� ���Ѵ�.
			D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + gameObj->GetObjCBIndex(j) * objCBByteSize;
			cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);

			cmdList->DrawIndexedInstanced(gameObj->GetNumIndices(j), 1, gameObj->GetBaseIndex(j), gameObj->GetBaseVertex(j), 0);
		}
	}
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> DummyApp::GetStaticSamplers()
{
	// �׷��� ���� ���α׷��� ����ϴ� ǥ��������� ���� �׸� ���� �����Ƿ�,
	// �̸� ���� ��Ʈ ���� ���Խ��� �д�.

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW
	
	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp };
}


