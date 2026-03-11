#pragma once
#include <Iron.AssetCompiler/AssetCompiler.h>

#include <filesystem>

#include <d3dcompiler.h>
#include <dxcapi.h>
#include <wrl.h>
#include <string>

using Microsoft::WRL::ComPtr;

namespace Iron::AssetCompiler {
class CShader : public IShader {
public:
    CShader(u8* blob, u64 size)
        : m_Blob(nullptr), m_Size(size) {
        m_Blob = (u8*)MemAlloc(m_Size);
        MemCopy(m_Blob, blob, m_Size);
    }

    void Release() override {
        MemFree(m_Blob);
        m_Blob = nullptr;
        m_Size = 0;
    }

    Result::Code SaveToFile(
        const char* filePath) override;

    u8* const GetData() override {
        return m_Blob;
    }

    u64 GetSize() override {
        return m_Size;
    }

private:
    u8*         m_Blob;
    u64         m_Size;
};

class CShaderCompilerD3D : public IShaderCompiler {
public:
    CShaderCompilerD3D(Version shaderVersion);

    void Release() override {}

    Result::Code CompileShaderFromFile(
        const char* fileName,
        const CompileShaderInfo& info,
        IShader** outHandle) override;

private:
    Version                         m_ShaderVersion;
    std::string                     m_AppendStr;

    constexpr static const char*    s_Prefixes[RHI::ShaderType::Count]{
        "err",
        "vs_",
        "ps_",
        "cs_",
        "err",
        "err",
    };
};

class CShaderCompilerDXC : public IShaderCompiler {
public:
    CShaderCompilerDXC(Version shaderVersion);

    Result::Code CompileShaderFromFile(
        const char* fileName,
        const CompileShaderInfo& info,
        IShader** outHandle) override;

    void Release() override;

private:
    Version                         m_ShaderVersion;
    std::string                     m_AppendStr;
    IDxcCompiler3*                  m_Compiler;
    IDxcUtils*                      m_Utils;
    IDxcIncludeHandler*             m_Include;

    constexpr static const char*    s_Prefixes[RHI::ShaderType::Count]{
        "err",
        "vs_",
        "ps_",
        "cs_",
        "ms_",
        "as_",
    };
};
}
