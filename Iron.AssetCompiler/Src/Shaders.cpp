#include <Iron.AssetCompiler/Src/Shaders.h>

namespace Iron::AssetCompiler {
std::wstring
ToWideString(std::string str) {
    return { str.begin(), str.end() };
}

Result::Code
CShader::SaveToFile(const char* filePath) {
    if (!filePath) {
        return Result::ENullptr;
    }

    const u64 total_size{ sizeof(u32) + m_Size };

    u8* buffer{ (u8*)MemAlloc(total_size) };
    if (!buffer) {
        return Result::ENomemory;
    }

    *(u32*)buffer = AssetType::Shader;
    MemCopy(buffer + sizeof(u32), m_Blob, m_Size);

    Result::Code res{ WriteFile(filePath, buffer, total_size) };

    return res;
}

CShaderCompilerD3D::CShaderCompilerD3D(
    Version shaderVersion)
    : m_ShaderVersion(shaderVersion) {
    m_AppendStr.append(std::to_string(m_ShaderVersion.Major));
    m_AppendStr.append("_");
    m_AppendStr.append(std::to_string(m_ShaderVersion.Minor));
}

Result::Code
CShaderCompilerD3D::CompileShaderFromFile(
    const char* fileName,
    const CompileShaderInfo& info,
    IShader** outHandle) {
    if (!(fileName && info.Entry && outHandle)) {
        return Result::ENullptr;
    }

    if (info.NumDefines && !info.Defines
        || info.Type >= RHI::ShaderType::Count) {
        return Result::EInvalidarg;
    }

    std::string profile{ s_Prefixes[info.Type] };
    profile.append(m_AppendStr);

    LOG_INFO("Compiling shader %s:%s", fileName, info.Entry);

    Vector<D3D_SHADER_MACRO> defines(info.NumDefines + 1);
    for (u32 i{ 0 }; i < info.NumDefines; ++i) {
        defines[i].Name = info.Defines[i].Name;
        defines[i].Definition = info.Defines[i].Definition;
    }

    defines.EmplaceBack();//Last of array must be empty struct

    u32 flags{ 0 };
    if (info.Flags & ShaderFlags::Debug) {
        flags |= D3DCOMPILE_DEBUG;
        flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
    }
    else {
        flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
    }
    if (info.Flags & ShaderFlags::WarningsAreErrors) {
        flags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
    }

    ComPtr<ID3DBlob> code_blob{};
    ComPtr<ID3DBlob> error_blob{};

    HRESULT hr{ S_OK };
    hr = D3DCompileFromFile(
        ToWideString(fileName).c_str(),
        defines.Data(),
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        info.Entry,
        profile.c_str(),
        flags,
        0,
        &code_blob,
        &error_blob);

    if (FAILED(hr)) {
        if (error_blob.Get() && error_blob->GetBufferPointer()) {
            LOG_ERROR("Shader compiler error: %s", (const char*)error_blob->GetBufferPointer());
        }

        return Result::EShaderError;
    }

    if (error_blob.Get() && error_blob->GetBufferPointer()) {
        LOG_WARNING("Shader compiler warning: %s", (const char*)error_blob->GetBufferPointer());
    }

    LOG_INFO("Successfully compiled shader %s:%s", fileName, info.Entry);

    CShader* temp{ new CShader(
        (u8*)code_blob->GetBufferPointer(),
        code_blob->GetBufferSize()) };
    if (!temp) {
        return Result::ENomemory;
    }

    *outHandle = temp;

    return Result::Ok;
}

CShaderCompilerDXC::CShaderCompilerDXC(
    Version shaderVersion)
    : m_ShaderVersion(shaderVersion),
    m_AppendStr(),
    m_Compiler(),
    m_Utils(),
    m_Include() {
    m_AppendStr.append(std::to_string(m_ShaderVersion.Major));
    m_AppendStr.append("_");
    m_AppendStr.append(std::to_string(m_ShaderVersion.Minor));

    HRESULT hr{ S_OK };
    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_Compiler));
    if (FAILED(hr)) return;
    hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_Utils));
    if (FAILED(hr)) return;
    hr = m_Utils->CreateDefaultIncludeHandler(&m_Include);
    if (FAILED(hr)) return;
}

Result::Code
CShaderCompilerDXC::CompileShaderFromFile(
    const char* fileName,
    const CompileShaderInfo& info,
    IShader** outHandle) {
    if (!(fileName && info.Entry && outHandle)) {
        return Result::ENullptr;
    }

    if (info.NumDefines && !info.Defines
        || info.Type >= RHI::ShaderType::Count) {
        return Result::EInvalidarg;
    }

    if (!(m_Compiler && m_Utils && m_Include)) {
        return Result::ENotInitialized;
    }

    std::string profile{ s_Prefixes[info.Type] };
    profile.append(m_AppendStr);

    std::filesystem::path dir{ fileName };

    LOG_INFO("Compiling shader %s:%s", fileName, info.Entry);

    HRESULT hr{ S_OK };
    ComPtr<IDxcBlobEncoding> source_blob{};
    hr = m_Utils->LoadFile(ToWideString(fileName).c_str(),
        nullptr, &source_blob);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to load shader file!");
        return Result::ELoadfile;
    }

    DxcBuffer buffer{};
    buffer.Encoding = DXC_CP_ACP;
    buffer.Ptr = source_blob->GetBufferPointer();
    buffer.Size = source_blob->GetBufferSize();

    Vector<std::wstring> extra_args{ info.NumDefines };

    extra_args.EmplaceBack(ToWideString(fileName));

    extra_args.EmplaceBack(L"-E");
    extra_args.EmplaceBack(ToWideString(info.Entry));

    extra_args.EmplaceBack(L"-T");
    extra_args.EmplaceBack(ToWideString(profile));

    extra_args.EmplaceBack(L"-I");
    extra_args.EmplaceBack(ToWideString(dir.parent_path().string()));

    extra_args.EmplaceBack(DXC_ARG_ALL_RESOURCES_BOUND);

    if (info.Flags & ShaderFlags::Debug) {
        extra_args.EmplaceBack(DXC_ARG_DEBUG);
        extra_args.EmplaceBack(DXC_ARG_SKIP_OPTIMIZATIONS);
    }
    else {
        extra_args.EmplaceBack(DXC_ARG_OPTIMIZATION_LEVEL3);
    }
    if (info.Flags & ShaderFlags::Allow16BitTypes) {
        extra_args.EmplaceBack(L"-enable-16bit-types");
    }
    if (info.Flags & ShaderFlags::WarningsAreErrors) {
        extra_args.EmplaceBack(DXC_ARG_WARNINGS_ARE_ERRORS);
    }

    for (u32 i{ 0 }; i < info.NumDefines; ++i) {
        extra_args.EmplaceBack(ToWideString(info.Defines[i].Name));
        extra_args.EmplaceBack(L"=");
        extra_args.EmplaceBack(ToWideString(info.Defines[i].Definition));
    }

    Vector<LPCWSTR> args{};
    for (auto& arg : extra_args) {
        args.PushBack(arg.c_str());
    }

    ComPtr<IDxcResult> results{ nullptr };
    hr = m_Compiler->Compile(
        &buffer,
        args.Data(),
        args.Size(),
        m_Include,
        IID_PPV_ARGS(&results));

    ComPtr<IDxcBlobUtf8> errors{ nullptr };
    results->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);

    if (FAILED(hr)) {
        if (errors.Get() && errors->GetBufferPointer()) {
            LOG_ERROR("Shader compiler error: %s", (const char*)errors->GetBufferPointer());
        }

        return Result::EShaderError;
    }

    if (errors.Get() && errors->GetBufferPointer()) {
        LOG_WARNING("Shader compiler warning: %s", (const char*)errors->GetBufferPointer());
    }

    ComPtr<IDxcBlob> shader{ nullptr };
    hr = results->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader), nullptr);
    if (FAILED(hr)) {
        return Result::EShaderError;
    }

    LOG_INFO("Successfully compiled shader %s:%s", fileName, info.Entry);

    CShader* temp{ new CShader(
    (u8*)shader->GetBufferPointer(),
    shader->GetBufferSize()) };
    if (!temp) {
        return Result::ENomemory;
    }

    *outHandle = temp;

    return Result::Ok;
}

void
CShaderCompilerDXC::Release() {
    SafeRelease(m_Include);
    SafeRelease(m_Utils);
    SafeRelease(m_Compiler);
}
}
