
#include "d3dUtil.h"
#include <comdef.h>
#include <fstream>

using Microsoft::WRL::ComPtr;

DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber) :
    ErrorCode(hr),
    FunctionName(functionName),
    Filename(filename),
    LineNumber(lineNumber)
{
}

bool d3dUtil::IsKeyDown(int vkeyCode)
{
    return (GetAsyncKeyState(vkeyCode) & 0x8000) != 0;
}

// 컴파일된 셰이더 바이트코드를 C++ 표준 파일 입출력 라이브러리를 이용해 적재하는 함수
// 사용예시)
// ComPtr<ID3DBlob> mvsByteCode = d3dUtil::LoadBinary(L"Shaders\\color_vs.hlsl");
ComPtr<ID3DBlob> d3dUtil::LoadBinary(const std::wstring& filename)
{
    std::ifstream fin(filename, std::ios::binary);

    fin.seekg(0, std::ios_base::end);
    std::ifstream::pos_type size = (int)fin.tellg();
    fin.seekg(0, std::ios_base::beg);

    ComPtr<ID3DBlob> blob;
    ThrowIfFailed(D3DCreateBlob(size, blob.GetAddressOf()));

    fin.read((char*)blob->GetBufferPointer(), size);
    fin.close();

    return blob;
}

// 정적 기하구조(프레임마다 변하지 않는 기하구조)를 그릴 때는 최적의 성능을 위해 정점 버퍼들은 Default Heap에 넣는다.
// 정적 기하구조의 경우 초기화후에는 GPU만 정보를 읽으므로 Default Heap에 넣는 것이 합당하다.
// CPU는 기본힙의 정점버퍼를 수정하지 못한다.
// 따라서, D3D12_HEAP_TYPE_UPLOAD형식의 임시 업로드 버퍼를 생성하여 시스템 메모리에 있는 정점자료를 업로드 버퍼에 복사하고, 
// 업로드 버퍼의 자료를 실제 정점버퍼로 복사한다.
Microsoft::WRL::ComPtr<ID3D12Resource> d3dUtil::CreateDefaultBuffer(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const void* initData,
    UINT64 byteSize,
    Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
{
    ComPtr<ID3D12Resource> defaultBuffer;

    // 실제 기본 버퍼 자원을 생성한다.
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

    // CPU는 메모리의 자료를 기본 버퍼에 복사하기 위해
    // 임시 업로드 힙을 생성한다.
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(uploadBuffer.GetAddressOf())));


    // 기본 버퍼에 복사할 자료를 서술한다.
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;
    subResourceData.RowPitch = byteSize;
    subResourceData.SlicePitch = byteSize;

    // 업로드 버퍼에서 기본 버퍼 자원으로 자료 복사를 요청한다.
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON, 
        D3D12_RESOURCE_STATE_COPY_DEST));
    UpdateSubresources<1>(cmdList, 
        defaultBuffer.Get(), uploadBuffer.Get(), 
        0, 0, 1, &subResourceData);
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, 
        D3D12_RESOURCE_STATE_GENERIC_READ));

    // 위의 함수 호출 이후에도 uploadBuffer를 계속 유지해야 한다.
    // 실제로 복사를 수행하는 명령목록이 아직 실행되지 않았기 때문이다.
    // 복사가 완료되었음이 확실해진 후에 uploadBuffer를 해제하면 된다.

    return defaultBuffer;
}

// 
ComPtr<ID3DBlob> d3dUtil::CompileShader(
    const std::wstring& filename,
    const D3D_SHADER_MACRO* defines,
    const std::string& entrypoint,
    const std::string& target)
{
    // 디버그 모드에서 디버깅 관련 플래그들을 사용한다.
    UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    HRESULT hr = S_OK;

    ComPtr<ID3DBlob> byteCode = nullptr;
    ComPtr<ID3DBlob> errors;
    hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

    // 오류 메세지를 디버그 창에 출력
    if (errors != nullptr)
        OutputDebugStringA((char*)errors->GetBufferPointer());

    ThrowIfFailed(hr);

    return byteCode;
}

std::wstring DxException::ToString()const
{
    // 오류 코드의 문자열 설명을 가져옵니다
    _com_error err(ErrorCode);
    std::wstring msg = err.ErrorMessage();

    return FunctionName + L" failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
}
