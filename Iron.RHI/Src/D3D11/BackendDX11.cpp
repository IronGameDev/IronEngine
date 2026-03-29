#include <Iron.RHI/Src/D3D11/BackendDX11.h>
#include <Iron.RHI/Src/Shared/Shared.h>

#include <wrl.h>
#include <unordered_map>
#include <algorithm>
#include <queue>

using Microsoft::WRL::ComPtr;

namespace Iron::RHI::D3D11 {
namespace {
static const GUID WKPDID_D3DDebugObjectName =
{ 0x429b8c22, 0x9188, 0x4b0c, { 0x87, 0x42, 0xac, 0xb0, 0xbf, 0x85, 0xc2, 0x00 } };

constexpr D3D11_USAGE
ConvertUsage(ResourceUsage::Usage usage) {
    switch (usage)
    {
    case ResourceUsage::Default:
        return D3D11_USAGE_DEFAULT;
    case ResourceUsage::Immutable:
        return D3D11_USAGE_IMMUTABLE;
    case ResourceUsage::Dynamic:
        return D3D11_USAGE_DYNAMIC;
    case ResourceUsage::Copy:
        return D3D11_USAGE_STAGING;
    default:
        return D3D11_USAGE_DEFAULT;
    }
}

constexpr u32
ConvertResourceBindFlags(ResourceFlags::Flags flags) {
    u32 value{ 0 };
    if (flags & ResourceFlags::AllowShaderResource) value |= D3D11_BIND_SHADER_RESOURCE;
    if (flags & ResourceFlags::BindVertexBuffer) value |= D3D11_BIND_VERTEX_BUFFER;
    if (flags & ResourceFlags::BindIndexBuffer) value |= D3D11_BIND_INDEX_BUFFER;

    return value;
}

constexpr u32
ConvertResourceMiscFlags(ResourceFlags::Flags flags) {
    u32 value{ 0 };
    if (flags & ResourceFlags::TextureCube) value |= D3D11_RESOURCE_MISC_TEXTURECUBE;
    if (flags & ResourceFlags::Structured) value |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

    return value;
}

constexpr D3D11_BLEND
ConvertBlend(Blend::Type blend) {
    switch (blend) {
    case Blend::Zero:
        return D3D11_BLEND_ZERO;
    case Blend::One:
        return D3D11_BLEND_ONE;
    case Blend::SrcColor:
        return D3D11_BLEND_SRC_COLOR;
    case Blend::InvSrcColor:
        return D3D11_BLEND_INV_SRC_COLOR;
    case Blend::SrcAlpha:
        return D3D11_BLEND_SRC_ALPHA;
    case Blend::InvSrcAlpha:
        return D3D11_BLEND_INV_SRC_ALPHA;
    case Blend::DstColor:
        return D3D11_BLEND_DEST_COLOR;
    case Blend::InvDstColor:
        return D3D11_BLEND_INV_DEST_COLOR;
    case Blend::DstAlpha:
        return D3D11_BLEND_DEST_ALPHA;
    case Blend::InvDstAlpha:
        return D3D11_BLEND_INV_DEST_ALPHA;
    case Blend::BlendFactor:
        return D3D11_BLEND_BLEND_FACTOR;
    default:
        return D3D11_BLEND_ZERO;
    }
}

constexpr D3D11_BLEND_OP
ConvertBlendOp(BlendOp::Op op) {
    switch (op) {
    case BlendOp::Add:
        return D3D11_BLEND_OP_ADD;
    case BlendOp::Sub:
        return D3D11_BLEND_OP_SUBTRACT;
    case BlendOp::RevSub:
        return D3D11_BLEND_OP_REV_SUBTRACT;
    case BlendOp::Min:
        return D3D11_BLEND_OP_MIN;
    case BlendOp::Max:
        return D3D11_BLEND_OP_MAX;
    default:
        return D3D11_BLEND_OP_ADD;
    }
}

constexpr D3D11_LOGIC_OP
ConvertLogicOp(LogicOp::Op op) {
    switch (op) {
    case LogicOp::Clear:
        return D3D11_LOGIC_OP_CLEAR;
    case LogicOp::Set:
        return D3D11_LOGIC_OP_SET;
    case LogicOp::Copy:
        return D3D11_LOGIC_OP_COPY;
    case LogicOp::CopyInv:
        return D3D11_LOGIC_OP_COPY_INVERTED;
    case LogicOp::Noop:
        return D3D11_LOGIC_OP_NOOP;
    case LogicOp::Inv:
        return D3D11_LOGIC_OP_INVERT;
    case LogicOp::And:
        return D3D11_LOGIC_OP_AND;
    case LogicOp::Nand:
        return D3D11_LOGIC_OP_NAND;
    case LogicOp::Or:
        return D3D11_LOGIC_OP_OR;
    case LogicOp::Nor:
        return D3D11_LOGIC_OP_NOR;
    case LogicOp::Xor:
        return D3D11_LOGIC_OP_XOR;
    case LogicOp::Equiv:
        return D3D11_LOGIC_OP_EQUIV;
    case LogicOp::AndRev:
        return D3D11_LOGIC_OP_AND_REVERSE;
    case LogicOp::AndInv:
        return D3D11_LOGIC_OP_AND_INVERTED;
    case LogicOp::OrRev:
        return D3D11_LOGIC_OP_OR_REVERSE;
    case LogicOp::OrInv:
        return D3D11_LOGIC_OP_OR_INVERTED;
    default:
        return D3D11_LOGIC_OP_CLEAR;
    }
}

constexpr D3D11_FILL_MODE
ConvertFillMode(FillMode::Mode mode) {
    switch (mode) {
    case FillMode::Wireframe:
        return D3D11_FILL_WIREFRAME;
    case FillMode::Solid:
        return D3D11_FILL_SOLID;
    default:
        return D3D11_FILL_SOLID;
    }
}

constexpr D3D11_CULL_MODE
ConvertCullMode(CullMode::Mode mode) {
    switch (mode)
    {
    case CullMode::None:
        return D3D11_CULL_NONE;
    case CullMode::Front:
        return D3D11_CULL_FRONT;
    case CullMode::Back:
        return D3D11_CULL_BACK;
    default:
        return D3D11_CULL_FRONT;
    }
}

constexpr D3D11_COMPARISON_FUNC
ConvertComparisonFunc(ComparisonFunc::Func func) {
    switch (func) {
    case ComparisonFunc::None:
        return D3D11_COMPARISON_NEVER;
    case ComparisonFunc::Never:
        return D3D11_COMPARISON_NEVER;
    case ComparisonFunc::Less:
        return D3D11_COMPARISON_LESS;
    case ComparisonFunc::Equal:
        return D3D11_COMPARISON_EQUAL;
    case ComparisonFunc::LessEqual:
        return D3D11_COMPARISON_LESS_EQUAL;
    case ComparisonFunc::Greater:
        return D3D11_COMPARISON_GREATER;
    case ComparisonFunc::NotEqual:
        return D3D11_COMPARISON_NOT_EQUAL;
    case ComparisonFunc::GreaterEqual:
        return D3D11_COMPARISON_GREATER_EQUAL;
    case ComparisonFunc::Always:
        return D3D11_COMPARISON_ALWAYS;
    default:
        return D3D11_COMPARISON_NEVER;
    }
}

constexpr D3D11_STENCIL_OP
ConvertStencilOp(StencilOp::Op op) {
    switch (op) {
    case StencilOp::Keep:
        return D3D11_STENCIL_OP_KEEP;
    case StencilOp::Zero:
        return D3D11_STENCIL_OP_ZERO;
    case StencilOp::Replace:
        return D3D11_STENCIL_OP_REPLACE;
    case StencilOp::IncrSat:
        return D3D11_STENCIL_OP_INCR_SAT;
    case StencilOp::DecrSat:
        return D3D11_STENCIL_OP_DECR_SAT;
    case StencilOp::Inv:
        return D3D11_STENCIL_OP_INVERT;
    case StencilOp::Incr:
        return D3D11_STENCIL_OP_INCR;
    case StencilOp::Decr:
        return D3D11_STENCIL_OP_DECR;
    default:
        return D3D11_STENCIL_OP_KEEP;
    }
}


inline void SetDebugName(ID3D11DeviceChild* object, const char* name) {
#if defined(_DEBUG)
    if (object) {
        object->SetPrivateData(
            WKPDID_D3DDebugObjectName,
            (UINT)strlen(name),
            name
        );
    }
#endif
}

namespace Cmd {
struct CommandListData {
    constexpr static u32 PCIncrSize{ RHI_MAX_PUSH_CONSTANTS_IN_32BIT * sizeof(u32) };

    ID3D11DeviceContext4*           Ctx{};
    CRHIDevice_DX11*                Device{};

    bool                            GraphicsLayout{};
    RHIPipelineLayout               CurrentLayout{ Id::InvalidId };
    struct {
        PipelineLayoutParam*        BaseParam{};
        u32                         NumParams{};
        u32                         PCSize{};
    } Layout;

    struct {
        ID3D11Buffer*               Buffer{};
        u32                         Offset{};
        D3D11_MAPPED_SUBRESOURCE    Map{};
    } PC;

    ID3D11ComputeShader*            CurrentCS{};
    ID3D11VertexShader*             CurrentVS{};
    ID3D11PixelShader*              CurrentPS{};
    ID3D11DomainShader*             CurrentDS{};
    ID3D11HullShader*               CurrentHS{};
    ID3D11GeometryShader*           CurrentGS{};
    ID3D11BlendState1*              CurrentBlend{};
    ID3D11RasterizerState2*         CurrentRaster{};
    ID3D11DepthStencilState*        CurrentDepth{};

    u32                             CounterDraw{};
    u32                             CounterDrawInstanced{};
    u32                             CounterDrawIndexed{};
    u32                             CounterDrawIndexedInstanced{};

    void Reset() {
        SafeRelease(PC.Buffer);

        GraphicsLayout = true;
        CurrentLayout = Id::InvalidId;

        Layout.BaseParam = nullptr;
        Layout.NumParams = 0;
        Layout.PCSize = 0;

        PC.Offset = 0;

        CurrentCS = nullptr;
        CurrentVS = nullptr;
        CurrentPS = nullptr;
        CurrentDS = nullptr;
        CurrentHS = nullptr;
        CurrentGS = nullptr;
        CurrentBlend = nullptr;
        CurrentRaster = nullptr;
        CurrentDepth = nullptr;

        CounterDraw = 0;
        CounterDrawInstanced = 0;
        CounterDrawIndexed = 0;
        CounterDrawIndexedInstanced = 0;
    }

    bool InitPCBuffer(u32 size) {
        if (!(Device && Ctx)) {
            return false;
        }

        D3D11_BUFFER_DESC desc{};
        desc.ByteWidth = size;
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        ID3D11Device5* const d3d11{ (ID3D11Device5* const)Device->GetNative() };
        HRESULT hr{ S_OK };
        hr = d3d11->CreateBuffer(&desc, nullptr, &PC.Buffer);

        return SUCCEEDED(hr);
    }

    bool PushBegin() {
        if (!PC.Buffer)
            return false;

        HRESULT hr{ S_OK };
        hr = Ctx->Map(PC.Buffer,
            0,
            D3D11_MAP_WRITE_DISCARD,
            0,
            &PC.Map);

        PC.Offset = 0;

        return SUCCEEDED(hr);
    }

    void PushEnd() {
        Ctx->Unmap(PC.Buffer, 0);

        PC.Offset = 0;
        PC.Map = {};
    }
};

typedef void(*CommandFunc)(CommandListData&, const void*);

void
CopyResource(CommandListData& cmdData, const void* data) {
    const RHICommandBuilder::CmdCopyResourceInfo& info{ *(const RHICommandBuilder::CmdCopyResourceInfo*)data };
    ID3D11Resource* const src{ cmdData.Device->ResolveResource(info.Src) };
    ID3D11Resource* const dst{ cmdData.Device->ResolveResource(info.Dst) };
    cmdData.Ctx->CopyResource(dst, src);
}

void
SetGraphicsLayout(CommandListData& cmdData, const void* data) {
    const RHICommandBuilder::CmdSetGraphicsLayoutInfo& info{ *(const RHICommandBuilder::CmdSetGraphicsLayoutInfo*)data };
    if (cmdData.CurrentLayout != info.Layout) {
        CRHIDevice_DX11::PipelineLayout layout{ cmdData.Device->ResolvePipelineLayout(info.Layout) };
        cmdData.GraphicsLayout = true;
        cmdData.CurrentLayout = info.Layout;
        cmdData.Layout.BaseParam = cmdData.Device->GetParams(layout.Start);
        cmdData.Layout.NumParams = layout.Count;
        cmdData.Layout.PCSize = layout.PCSize;
    }
}

void
SetComputeLayout(CommandListData& cmdData, const void* data) {
    const RHICommandBuilder::CmdSetGraphicsLayoutInfo& info{ *(const RHICommandBuilder::CmdSetGraphicsLayoutInfo*)data };
    if (cmdData.CurrentLayout != info.Layout) {
        CRHIDevice_DX11::PipelineLayout layout{ cmdData.Device->ResolvePipelineLayout(info.Layout) };
        cmdData.GraphicsLayout = false;
        cmdData.CurrentLayout = info.Layout;
        cmdData.Layout.BaseParam = cmdData.Device->GetParams(layout.Start);
        cmdData.Layout.NumParams = layout.Count;
        cmdData.Layout.PCSize = layout.PCSize;
    }
}

void
SetPipeline(CommandListData& cmdData, const void* data) {
    const RHICommandBuilder::CmdSetPipelineInfo& info{ *(const RHICommandBuilder::CmdSetPipelineInfo*)data };
    const CRHIDevice_DX11::PipelineData& pso_data{ cmdData.Device->ResolvePipelineData(info.Pso) };

    if (pso_data.Graphics) {
        const CRHIDevice_DX11::GraphicsPipeline& pso{ cmdData.Device->ResolveGraphicsPipeline(pso_data) };

        if (pso.VS.Ptr != cmdData.CurrentVS) {
            cmdData.Ctx->VSSetShader(pso.VS.Ptr, nullptr, 0);
            cmdData.CurrentVS = pso.VS.Ptr;
        }
        if (pso.PS.Ptr != cmdData.CurrentPS) {
            cmdData.Ctx->PSSetShader(pso.PS.Ptr, nullptr, 0);
            cmdData.CurrentPS = pso.PS.Ptr;
        }
        if (pso.DS.Ptr != cmdData.CurrentDS) {
            cmdData.Ctx->DSSetShader(pso.DS.Ptr, nullptr, 0);
            cmdData.CurrentDS = pso.DS.Ptr;
        }
        if (pso.HS.Ptr != cmdData.CurrentHS) {
            cmdData.Ctx->HSSetShader(pso.HS.Ptr, nullptr, 0);
            cmdData.CurrentHS = pso.HS.Ptr;
        }
        if (pso.GS.Ptr != cmdData.CurrentGS) {
            cmdData.Ctx->GSSetShader(pso.GS.Ptr, nullptr, 0);
            cmdData.CurrentGS = pso.GS.Ptr;
        }
        if (pso.Blend.Ptr != cmdData.CurrentBlend) {
            constexpr float b[4]{ 0.f, 0.f, 0.f, 0.f };///////TODODOOOOOOOOO
            cmdData.Ctx->OMSetBlendState(pso.Blend.Ptr, &b[0], pso.SampleMask);
            cmdData.CurrentBlend = pso.Blend.Ptr;
        }
        if (pso.Rasterizer.Ptr != cmdData.CurrentRaster) {
            cmdData.Ctx->RSSetState(pso.Rasterizer.Ptr);
            cmdData.CurrentRaster = pso.Rasterizer.Ptr;
        }
        if (pso.Depth.Ptr != cmdData.CurrentDepth) {
            cmdData.Ctx->OMSetDepthStencilState(pso.Depth.Ptr, 0);
            cmdData.CurrentDepth = pso.Depth.Ptr;
        }
    }
    else {
        const CRHIDevice_DX11::ComputePipeline& pso{ cmdData.Device->ResolveComputePipeline(pso_data) };

        if (pso.CS.Ptr != cmdData.CurrentCS) {
            cmdData.Ctx->CSSetShader(pso.CS.Ptr, nullptr, 0);
            cmdData.CurrentCS = pso.CS.Ptr;
        }
    }
}

void
SetPrimitiveTopology(CommandListData& cmdData, const void* data) {
    const RHICommandBuilder::CmdSetPrimitiveTopologyInfo& info{ *(const RHICommandBuilder::CmdSetPrimitiveTopologyInfo*)data };
    cmdData.Ctx->IASetPrimitiveTopology(Shared::ConvertTopology(info.Topology));
}

void
SetViewport(CommandListData& cmdData, const void* data) {
    const RHICommandBuilder::CmdSetViewportsInfo& info{ *(const RHICommandBuilder::CmdSetViewportsInfo*)data };
    static D3D11_VIEWPORT vps[RHI_MAX_TARGET_COUNT]{};
    for (u32 i{ 0 }; i < info.Count; ++i) {
        vps[i].TopLeftX = info.Views[i].TopLeftX;
        vps[i].TopLeftY = info.Views[i].TopLeftY;
        vps[i].Width = info.Views[i].Width;
        vps[i].Height = info.Views[i].Height;
        vps[i].MinDepth = info.Views[i].MinDepth;
        vps[i].MaxDepth = info.Views[i].MaxDepth;
    }

    cmdData.Ctx->RSSetViewports(info.Count, &vps[0]);
}

void
SetScissor(CommandListData& cmdData, const void* data) {
    const RHICommandBuilder::CmdSetScissorsInfo& info{ *(const RHICommandBuilder::CmdSetScissorsInfo*)data };
    static D3D11_RECT rcs[RHI_MAX_TARGET_COUNT]{};
    for (u32 i{ 0 }; i < info.Count; ++i) {
        rcs[i].left = info.Rects[i].Left;
        rcs[i].top = info.Rects[i].Top;
        rcs[i].right = info.Rects[i].Right;
        rcs[i].bottom = info.Rects[i].Bottom;
    }

    cmdData.Ctx->RSSetScissorRects(info.Count, &rcs[0]);
}

void
SetPushConstants(CommandListData& cmdData, const void* data) {
    const RHICommandBuilder::CmdSetPushConstants& info{ *(const RHICommandBuilder::CmdSetPushConstants*)data };
    const u32 rounded{ ((cmdData.Layout.PCSize + 15) / 16) * 16 };
    u32& offset{ cmdData.PC.Offset };

    if (cmdData.GraphicsLayout) {
        if (cmdData.CurrentVS) {
            cmdData.Ctx->VSSetConstantBuffers1(
                0,
                1,
                &cmdData.PC.Buffer,
                &offset,
                &rounded);
        }
        if (cmdData.CurrentPS) {
            cmdData.Ctx->PSSetConstantBuffers1(
                0,
                1,
                &cmdData.PC.Buffer,
                &offset,
                &rounded);
        }
        if (cmdData.CurrentDS) {
            cmdData.Ctx->DSSetConstantBuffers1(
                0,
                1,
                &cmdData.PC.Buffer,
                &offset,
                &rounded);
        }
        if (cmdData.CurrentHS) {
            cmdData.Ctx->HSSetConstantBuffers1(
                0,
                1,
                &cmdData.PC.Buffer,
                &offset,
                &rounded);
        }
        if (cmdData.CurrentGS) {
            cmdData.Ctx->GSSetConstantBuffers1(
                0,
                1,
                &cmdData.PC.Buffer,
                &offset,
                &rounded);
        }
    }
    else {
        cmdData.Ctx->CSSetConstantBuffers1(
            0,
            1,
            &cmdData.PC.Buffer,
            &offset,
            &rounded);
    }

    offset += CommandListData::PCIncrSize;
}

void
Draw(CommandListData& cmdData, const void* data) {
    const RHICommandBuilder::CmdDrawInfo& info{ *(const RHICommandBuilder::CmdDrawInfo*)data };
    cmdData.Ctx->Draw(info.VertexCount, info.BaseVertex);
    ++cmdData.CounterDraw;
}

void
DrawInstanced(CommandListData& cmdData, const void* data) {
    const RHICommandBuilder::CmdDrawInstancedInfo& info{ *(const RHICommandBuilder::CmdDrawInstancedInfo*)data };
    cmdData.Ctx->DrawInstanced(info.VertexCount, info.InstanceCount, info.BaseVertex, info.BaseInstance);
    ++cmdData.CounterDrawInstanced;
}

void
DrawIndexed(CommandListData& cmdData, const void* data) {
    const RHICommandBuilder::CmdDrawIndexedInfo& info{ *(const RHICommandBuilder::CmdDrawIndexedInfo*)data };
    cmdData.Ctx->DrawIndexed(info.IndexCount, info.BaseIndex, info.BaseVertex);
    ++cmdData.CounterDrawIndexed;
}

void
DrawIndexedInstanced(CommandListData& cmdData, const void* data) {
    const RHICommandBuilder::CmdDrawInstancedIndexedInfo& info{ *(const RHICommandBuilder::CmdDrawInstancedIndexedInfo*)data };
    cmdData.Ctx->DrawIndexedInstanced(info.IndexCount, info.InstanceCount, info.BaseIndex, info.BaseVertex, info.BaseInstance);
    ++cmdData.CounterDrawIndexedInstanced;
}

constexpr static CommandFunc DispatchTable[RHICommandBuilder::CommandId::Count]{
    CopyResource,
    SetGraphicsLayout,
    SetComputeLayout,
    SetPipeline,
    SetPrimitiveTopology,
    SetViewport,
    SetScissor,
    SetPushConstants,
    Draw,
    DrawInstanced,
    DrawIndexed,
    DrawIndexedInstanced,
};

void
ParseCommandStream(RHICommandBuilder& builder, CommandListData& cmdData) {
    if (!(builder.GetStream() && cmdData.Ctx)) {
        return;
    }

    const u32 total_size{ builder.GetOffset() };
    const u8* start{ builder.GetStream() };
    const u8* stream{ start };
    const u8* const end{ stream + total_size };

    if (cmdData.PushBegin()) {
        while (stream < end) {
            const RHICommandBuilder::CmdHeader header{ *(const RHICommandBuilder::CmdHeader*)stream };
            if (header.Id >= RHICommandBuilder::CommandId::Count) {
                LOG_ERROR("Invalid command id used!");
            }

            stream += sizeof(RHICommandBuilder::CmdHeader);

            if (header.Id == RHICommandBuilder::CommandId::SetPushConstants) {
                const RHICommandBuilder::CmdSetPushConstants& info{ *(const RHICommandBuilder::CmdSetPushConstants*)stream };

                u8* const map_ptr{ (u8*)cmdData.PC.Map.pData };

                MemCopy(map_ptr + cmdData.PC.Offset, &info.Constants[0], CommandListData::PCIncrSize);

                cmdData.PC.Offset += CommandListData::PCIncrSize;
            }

            stream += header.PayloadSize;
        }

        cmdData.PushEnd();
        cmdData.PC.Offset = 0;
    }

    stream = start;

    while (stream < end) {
        const RHICommandBuilder::CmdHeader header{ *(const RHICommandBuilder::CmdHeader*)stream };
        stream += sizeof(RHICommandBuilder::CmdHeader);
        DispatchTable[header.Id](cmdData, stream);
        stream += header.PayloadSize;
    }
}
}
}//anonymous namespace

CRHIDevice_DX11::CRHIDevice_DX11(
    IRHIFactory* const factory,
    IRHIAdapter* const adapter,
    const DeviceInitInfo& info)
    :
    m_Factory(factory),
    m_Debug(false),
    m_D3D11Dll(),
    m_D3D11CreateDevice(),
    m_FeatureLevel(),
    m_Device(),
    m_Context(),
    m_Features() {
    if (!(adapter && m_Factory)) {
        LOG_ERROR("Invalid adapter/factory given to device!");
        return;
    }

    m_D3D11Dll = LoadLibraryExA("d3d11.dll", 0, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!m_D3D11Dll) {
        LOG_FATAL("Failed to load d3d11.dll!");
        return;
    }

    m_D3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(m_D3D11Dll, "D3D11CreateDevice");
    if (!m_D3D11CreateDevice) {
        LOG_FATAL("Failed to get D3D11CreateDevice() from d3d11.dll!");
        return;
    }

    UINT flags{ 0 };
    if (info.Debug) {
        flags |= D3D11_CREATE_DEVICE_DEBUG;
        LOG_DEBUG("Enabled debug for d3d11 device!");
        m_Debug = true;
    }
    if (info.DisableGPUTimeout) {
        flags |= D3D11_CREATE_DEVICE_DISABLE_GPU_TIMEOUT;
    }

    const D3D_FEATURE_LEVEL levels[]{
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

    ComPtr<ID3D11Device> device{};
    ComPtr<ID3D11DeviceContext> context{};

    HRESULT hr{ S_OK };
    hr = m_D3D11CreateDevice(
        (IDXGIAdapter*)adapter->GetNative(),
        D3D_DRIVER_TYPE_UNKNOWN,
        0,
        flags,
        &levels[0],
        _countof(levels),
        D3D11_SDK_VERSION,
        &device,
        &m_FeatureLevel,
        &context);

    if (FAILED(hr)) {
        LOG_ERROR("Failed to create d3d11 device!");
        return;
    }

    device->QueryInterface(IID_PPV_ARGS(&m_Device));
    context->QueryInterface(IID_PPV_ARGS(&m_Context));

    if (m_Debug) {
        ComPtr<ID3D11InfoQueue> queue{};
        if (SUCCEEDED(m_Device->QueryInterface(IID_PPV_ARGS(&queue)))) {
            queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, true);
            queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
            queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
        }
    }

    D3D11_FEATURE_DATA_D3D11_OPTIONS d3d11_options{ QueryFeatureSupport<D3D11_FEATURE_DATA_D3D11_OPTIONS>() };

    m_Features.Bindless = false;
    m_Features.PushConstants = d3d11_options.ConstantBufferPartialUpdate & d3d11_options.ConstantBufferOffsetting;
    m_Features.FeatureLevel.Major = ((m_FeatureLevel >> 8) & 0xf0) >> 4;
    m_Features.FeatureLevel.Minor = (m_FeatureLevel >> 8) & 0xf;

    switch (m_FeatureLevel)
    {
    case D3D_FEATURE_LEVEL_9_1:
    case D3D_FEATURE_LEVEL_9_2:
        m_Features.ShaderModel.Major = 2;
        m_Features.ShaderModel.Minor = 0xf;
        break;
    case D3D_FEATURE_LEVEL_9_3:
        m_Features.ShaderModel.Major = 3;
        m_Features.ShaderModel.Minor = 0;
        break;
    case D3D_FEATURE_LEVEL_10_0:
        m_Features.ShaderModel.Major = 4;
        m_Features.ShaderModel.Minor = 0;
        break;;
    case D3D_FEATURE_LEVEL_10_1:
        m_Features.ShaderModel.Major = 4;
        m_Features.ShaderModel.Minor = 1;
        break;
    case D3D_FEATURE_LEVEL_11_0:
    case D3D_FEATURE_LEVEL_11_1:
    case D3D_FEATURE_LEVEL_12_0:
    case D3D_FEATURE_LEVEL_12_1:
    case D3D_FEATURE_LEVEL_12_2:
        m_Features.ShaderModel.Major = 5;
        m_Features.ShaderModel.Minor = 0;
        break;
    default:
        break;
    }

}

void
CRHIDevice_DX11::Release() {
    SafeRelease(m_Context);

    if (m_Debug) {
        ComPtr<ID3D11InfoQueue> queue{};
        if (SUCCEEDED(m_Device->QueryInterface(IID_PPV_ARGS(&queue)))) {
            queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, false);
            queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, false);
            queue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, false);
        }

        ComPtr<ID3D11Debug> debug{};
        if (SUCCEEDED(m_Device->QueryInterface(IID_PPV_ARGS(&debug)))) {
            debug->ReportLiveDeviceObjects(D3D11_RLDO_SUMMARY | D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
        }
    }

    SafeRelease(m_Device);

    if (m_D3D11Dll) {
        FreeLibrary(m_D3D11Dll);
    }

    delete this;
}

Result::Code
CRHIDevice_DX11::CreateResource(const ResourceInitInfo& info,
    RHIResource* resource) {
    u32 cpu_flags{ 0 };
    if (info.CPURead) cpu_flags |= D3D11_CPU_ACCESS_READ;
    if (info.CPUWrite) cpu_flags |= D3D11_CPU_ACCESS_WRITE;

    const DXGI_FORMAT format{ Shared::ConvertFormat(info.Format) };
    const D3D11_USAGE usage{ ConvertUsage(info.Usage) };
    const u32 bind_flags{ ConvertResourceBindFlags((ResourceFlags::Flags)info.Flags) };
    const u32 misc_flags{ ConvertResourceMiscFlags((ResourceFlags::Flags)info.Flags) };

    DenseResource dense{};

    switch (info.Dimension)
    {
    case ResourceDimension::Buffer: {
        D3D11_BUFFER_DESC desc{};
        desc.ByteWidth = info.Width;
        desc.Usage = usage;
        desc.BindFlags = bind_flags;
        desc.CPUAccessFlags = cpu_flags;
        desc.MiscFlags = misc_flags;
        desc.StructureByteStride = info.StructuredStride;

        dense.Type = DenseResource::EBuffer;

        HRESULT hr{ S_OK };
        hr = m_Device->CreateBuffer(&desc, nullptr, (ID3D11Buffer**)&dense.Resource);
        if (FAILED(hr)) {
            return Result::ECreateResource;
        }
    } break;
    case ResourceDimension::Texture1D: {
        D3D11_TEXTURE1D_DESC desc{};
        desc.Width = info.Width;
        desc.MipLevels = info.MipLevels;
        desc.ArraySize = info.DepthOrArray;
        desc.Format = format;
        desc.Usage = usage;
        desc.BindFlags = bind_flags;
        desc.CPUAccessFlags = cpu_flags;
        desc.MiscFlags = misc_flags;

        dense.Type = DenseResource::ETexture1D;

        HRESULT hr{ S_OK };
        hr = m_Device->CreateTexture1D(&desc, nullptr, (ID3D11Texture1D**)&dense.Resource);
        if (FAILED(hr)) {
            return Result::ECreateResource;
        }
    } break;
    case ResourceDimension::Texture2D: {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = info.Width;
        desc.Height = info.Height;
        desc.MipLevels = info.MipLevels;
        desc.ArraySize = info.DepthOrArray;
        desc.Format = format;
        desc.SampleDesc = { 1, 0 };
        desc.Usage = usage;
        desc.BindFlags = bind_flags;
        desc.CPUAccessFlags = cpu_flags;
        desc.MiscFlags = misc_flags;

        dense.Type = DenseResource::ETexture2D;

        HRESULT hr{ S_OK };
        hr = m_Device->CreateTexture2D(&desc, nullptr, (ID3D11Texture2D**)&dense.Resource);
        if (FAILED(hr)) {
            return Result::ECreateResource;
        }
    } break;
    case ResourceDimension::Texture3D: {
        D3D11_TEXTURE3D_DESC desc{};
        desc.Width = info.Width;
        desc.Height = info.Height;
        desc.Depth = info.DepthOrArray;
        desc.MipLevels = info.MipLevels;
        desc.Format = format;
        desc.Usage = usage;
        desc.BindFlags = bind_flags;
        desc.CPUAccessFlags = cpu_flags;
        desc.MiscFlags = misc_flags;

        dense.Type = DenseResource::ETexture3D;

        HRESULT hr{ S_OK };
        hr = m_Device->CreateTexture3D(&desc, nullptr, (ID3D11Texture3D**)&dense.Resource);
        if (FAILED(hr)) {
            return Result::ECreateResource;
        }
    } break;
    default:
        LOG_WARNING("Invalid resource dimension!");
        break;
    }

    if (!dense.Resource) {
        return Result::ECreateResource;
    }

    u32 sparse_index{};
    if (!m_FreeSparseResources.Empty()) {
        sparse_index = m_FreeSparseResources.Back();
        m_FreeSparseResources.PopBack();
    }
    else {
        sparse_index = m_SparseResources.Size();
        m_SparseResources.PushBack({});
    }

    dense.SparseIndex = sparse_index;

    const u32 dense_index{ m_DenseResources.Size() };
    m_DenseResources.EmplaceBack(dense);

    auto& slot{ m_SparseResources[sparse_index] };
    slot.DenseIndex = dense_index;

    *resource = Id::MakeHandle(sparse_index, slot.Generation);

    return Result::Ok;
}

Result::Code
CRHIDevice_DX11::CreatePipelineLayout(const PipelineLayoutInitInfo& info,
    RHIPipelineLayout* outHandle) {
    if (!outHandle) {
        return Result::ENullptr;
    }

    *outHandle = Id::InvalidId;

    if (info.PushConstantSize && !HasPC()) {
        LOG_ERROR("D3D11 Push constants not supported!");
        return Result::EInvalidarg;
    }

    u32 index{};
    if (m_FreeLayouts.Size()) {
        for (u32 i{ 0 }; i < m_FreeLayouts.Size(); ++i) {
            const u32 free{ m_FreeLayouts[i] };
            if (m_PipelineLayouts[free].Count == info.NumParams) {
                index = free;
                m_FreeLayouts.Erase(i);
                break;
            }
        }
    }
    else {
        index = m_PipelineLayouts.Size();
        m_PipelineLayouts.EmplaceBack();
    }

    PipelineLayout& layout{ m_PipelineLayouts[index] };
    layout.Count = info.NumParams;
    layout.PCSize = info.PushConstantSize * sizeof(u32);
    if (layout.PCSize && !HasPC()) {
        LOG_ERROR("Push constants not supported!");
        return Result::ENoInterface;
    }

    if (!layout.Start) {
        layout.Start = m_PipelineParams.Size();
        for (u32 i{ 0 }; i < info.NumParams; ++i)
            m_PipelineParams.EmplaceBack();

    }

    for (u32 i{ 0 }; i < layout.Count; ++i) {
        m_PipelineParams[layout.Start + i] = info.Params[i];
    }

    *outHandle = Id::MakeHandle(index, layout.Generation);

    return Result::Ok;
}

Result::Code
CRHIDevice_DX11::CreateComputePipeline(const ComputePipelineInitInfo& info,
    RHIPipeline* outHandle) {
    const u64 cs_hash{ Shared::HashShaderBytecode(info.CS, 1469598103934665603ull) };

    ComputePipeline pipeline{};
    pipeline.Layout = info.Layout;
    pipeline.CS = m_ComputeShaders.Retrieve(cs_hash);

    if (!pipeline.CS.IsValid()) {
        ID3D11ComputeShader* shader{ nullptr };
        HRESULT hr{ S_OK };
        hr = m_Device->CreateComputeShader(
            info.CS.Blob,
            info.CS.Size,
            nullptr,
            &shader);
        if (FAILED(hr)) {
            LOG_HR(hr);
            return Result::ECreateRHIObject;
        }

        pipeline.CS = m_ComputeShaders.AddNewItem(cs_hash, shader);
    }

    if (!pipeline.CS.IsValid()) {
        return Result::ECreateRHIObject;
    }

    u32 index{};
    if (m_FreePipelines.Size()) {
        index = m_FreePipelines.Back();
        m_FreePipelines.PopBack();
    }
    else {
        index = m_Pipelines.Size();
        m_Pipelines.EmplaceBack();
    }

    m_Pipelines[index].Graphics = false;
    m_Pipelines[index].Index = m_ComputePipelines.Size();
    m_ComputePipelines.EmplaceBack(pipeline);

    *outHandle = Id::MakeHandle(index, m_Pipelines[index].Generation);

    return Result::Ok;
}

Result::Code
CRHIDevice_DX11::CreateGraphicsPipeline(const GraphicsPipelineInitInfo& info,
    RHIPipeline* outHandle) {
    if (!outHandle) {
        return Result::ENullptr;
    }

    const u64 vs_hash{ Shared::HashShaderBytecode(info.VS, 1469598103934665603ull) };
    const u64 ps_hash{ Shared::HashShaderBytecode(info.PS, 1469598103934665603ull) };
    const u64 ds_hash{ Shared::HashShaderBytecode(info.DS, 1469598103934665603ull) };
    const u64 hs_hash{ Shared::HashShaderBytecode(info.HS, 1469598103934665603ull) };
    const u64 gs_hash{ Shared::HashShaderBytecode(info.GS, 1469598103934665603ull) };

    GraphicsPipeline pipeline{};
    pipeline.Layout = info.Layout;
    pipeline.VS = m_VertexShaders.Retrieve(vs_hash);
    pipeline.PS = m_PixelShaders.Retrieve(ps_hash);
    pipeline.DS = m_DomainShaders.Retrieve(ds_hash);
    pipeline.HS = m_HullShaders.Retrieve(hs_hash);
    pipeline.GS = m_GeometryShaders.Retrieve(gs_hash);

    if (!pipeline.VS.IsValid() && info.VS.Blob) {
        ID3D11VertexShader* shader{ nullptr };
        HRESULT hr{ S_OK };
        hr = m_Device->CreateVertexShader(
            info.VS.Blob,
            info.VS.Size,
            nullptr,
            &shader);
        if (FAILED(hr)) {
            LOG_HR(hr);
            return Result::ECreateRHIObject;
        }

        pipeline.VS = m_VertexShaders.AddNewItem(vs_hash, shader);
    }
    if (!pipeline.PS.IsValid() && info.PS.Blob) {
        ID3D11PixelShader* shader{ nullptr };
        HRESULT hr{ S_OK };
        hr = m_Device->CreatePixelShader(
            info.PS.Blob,
            info.PS.Size,
            nullptr,
            &shader);
        if (FAILED(hr)) {
            LOG_HR(hr);
            return Result::ECreateRHIObject;
        }

        pipeline.PS = m_PixelShaders.AddNewItem(ps_hash, shader);
    }
    if (!pipeline.DS.IsValid() && info.DS.Blob) {
        ID3D11DomainShader* shader{ nullptr };
        HRESULT hr{ S_OK };
        hr = m_Device->CreateDomainShader(
            info.DS.Blob,
            info.DS.Size,
            nullptr,
            &shader);
        if (FAILED(hr)) {
            LOG_HR(hr);
            return Result::ECreateRHIObject;
        }

        pipeline.DS = m_DomainShaders.AddNewItem(ds_hash, shader);
    }
    if (!pipeline.HS.IsValid() && info.HS.Blob) {
        ID3D11HullShader* shader{ nullptr };
        HRESULT hr{ S_OK };
        hr = m_Device->CreateHullShader(
            info.HS.Blob,
            info.HS.Size,
            nullptr,
            &shader);
        if (FAILED(hr)) {
            LOG_HR(hr);
            return Result::ECreateRHIObject;
        }

        pipeline.HS = m_HullShaders.AddNewItem(hs_hash, shader);
    }
    if (!pipeline.GS.IsValid() && info.GS.Blob) {
        ID3D11GeometryShader* shader{ nullptr };
        HRESULT hr{ S_OK };
        hr = m_Device->CreateGeometryShader(
            info.GS.Blob,
            info.GS.Size,
            nullptr,
            &shader);
        if (FAILED(hr)) {
            LOG_HR(hr);
            return Result::ECreateRHIObject;
        }

        pipeline.GS = m_GeometryShaders.AddNewItem(gs_hash, shader);
    }

    pipeline.SampleMask = info.SampleMask;

    D3D11_BLEND_DESC1 blend{};
    blend.AlphaToCoverageEnable = info.Blend.AlphaToCoverageEnable;
    blend.IndependentBlendEnable = info.Blend.IndependentBlendEnable;

    for (u32 i{ 0 }; i < RHI_MAX_TARGET_COUNT; ++i) {
        const TargetBlendInitInfo& tgt{ info.Blend.Targets[i] };

        blend.RenderTarget[i].BlendEnable = tgt.BlendEnable;
        blend.RenderTarget[i].LogicOpEnable = tgt.LogicOp;
        blend.RenderTarget[i].SrcBlend = ConvertBlend(tgt.SrcBlend);
        blend.RenderTarget[i].DestBlend = ConvertBlend(tgt.DstBlend);
        blend.RenderTarget[i].BlendOp = ConvertBlendOp(tgt.BlendOp);
        blend.RenderTarget[i].SrcBlendAlpha = ConvertBlend(tgt.SrcBlendAlpha);
        blend.RenderTarget[i].DestBlendAlpha = ConvertBlend(tgt.DstBlendAlpha);
        blend.RenderTarget[i].BlendOpAlpha = ConvertBlendOp(tgt.BlendOpAlpha);
        blend.RenderTarget[i].LogicOp = ConvertLogicOp(tgt.LogicOp);
        blend.RenderTarget[i].RenderTargetWriteMask = tgt.TargetMask;
    }

    D3D11_RASTERIZER_DESC2 rasterizer{};
    rasterizer.FillMode = ConvertFillMode(info.Rasterizer.Fill);
    rasterizer.CullMode = ConvertCullMode(info.Rasterizer.Cull);
    rasterizer.FrontCounterClockwise = info.Rasterizer.FrontCounterClockwise;
    rasterizer.DepthBias = info.Rasterizer.DepthBias;
    rasterizer.DepthBiasClamp = info.Rasterizer.DepthBiasClamp;
    rasterizer.SlopeScaledDepthBias = info.Rasterizer.SlopeScaledDepthBias;
    rasterizer.DepthClipEnable = info.Rasterizer.DepthClipEnable;
    rasterizer.MultisampleEnable = info.Rasterizer.MultisampleEnable;
    rasterizer.AntialiasedLineEnable = info.Rasterizer.AntialiasedLineEnable;
    rasterizer.ForcedSampleCount = info.Rasterizer.ForcedSampleCount;
    rasterizer.ConservativeRaster =
        info.Rasterizer.ConservativeRaster
        ? D3D11_CONSERVATIVE_RASTERIZATION_MODE_ON
        : D3D11_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    D3D11_DEPTH_STENCIL_DESC depth{};
    depth.DepthEnable = info.DepthStencil.DepthEnable;
    depth.DepthWriteMask = info.DepthStencil.DepthWriteAll ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    depth.DepthFunc = ConvertComparisonFunc(info.DepthStencil.DepthFunc);
    depth.StencilEnable = info.DepthStencil.StencilEnable;
    depth.StencilReadMask = info.DepthStencil.StencilReadMask;
    depth.StencilWriteMask = info.DepthStencil.StencilWriteMask;
    depth.FrontFace.StencilFailOp = ConvertStencilOp(info.DepthStencil.FrontFace.StencilFailOp);
    depth.FrontFace.StencilDepthFailOp = ConvertStencilOp(info.DepthStencil.FrontFace.StencilDepthFailOp);
    depth.FrontFace.StencilPassOp = ConvertStencilOp(info.DepthStencil.FrontFace.StencilPassOp);
    depth.FrontFace.StencilFunc = ConvertComparisonFunc(info.DepthStencil.FrontFace.StencilFunc);
    depth.BackFace.StencilFailOp = ConvertStencilOp(info.DepthStencil.BackFace.StencilFailOp);
    depth.BackFace.StencilDepthFailOp = ConvertStencilOp(info.DepthStencil.BackFace.StencilDepthFailOp);
    depth.BackFace.StencilPassOp = ConvertStencilOp(info.DepthStencil.BackFace.StencilPassOp);
    depth.BackFace.StencilFunc = ConvertComparisonFunc(info.DepthStencil.BackFace.StencilFunc);

    const u64 blend_hash{ Shared::HashStruct(blend, 1469598103934665603ull) };
    const u64 raster_hash{ Shared::HashStruct(rasterizer, 1469598103934665603ull) };
    const u64 depth_hash{ Shared::HashStruct(depth, 1469598103934665603ull) };

    pipeline.Blend = m_BlendStates.Retrieve(blend_hash);
    pipeline.Rasterizer = m_RasterStates.Retrieve(raster_hash);
    pipeline.Depth = m_DepthStates.Retrieve(depth_hash);

    if (!pipeline.Blend.IsValid()) {
        ID3D11BlendState1* state{ nullptr };
        HRESULT hr{ S_OK };
        hr = m_Device->CreateBlendState1(&blend,
            &state);

        if (FAILED(hr)) {
            LOG_HR(hr);
            return Result::ECreateRHIObject;
        }

        pipeline.Blend = m_BlendStates.AddNewItem(blend_hash, state);
    }
    if (!pipeline.Rasterizer.IsValid()) {
        ID3D11RasterizerState2* state{ nullptr };
        HRESULT hr{ S_OK };
        hr = m_Device->CreateRasterizerState2(&rasterizer,
            &state);

        if (FAILED(hr)) {
            LOG_HR(hr);
            return Result::ECreateRHIObject;
        }

        pipeline.Rasterizer = m_RasterStates.AddNewItem(raster_hash, state);
    }
    if (!pipeline.Depth.IsValid()) {
        ID3D11DepthStencilState* state{ nullptr };
        HRESULT hr{ S_OK };
        hr = m_Device->CreateDepthStencilState(&depth,
            &state);

        if (FAILED(hr)) {
            LOG_HR(hr);
            return Result::ECreateRHIObject;
        }

        pipeline.Depth = m_DepthStates.AddNewItem(depth_hash, state);
    }

    u32 index{};
    if (m_FreePipelines.Size()) {
        index = m_FreePipelines.Back();
        m_FreePipelines.PopBack();
    }
    else {
        index = m_Pipelines.Size();
        m_Pipelines.EmplaceBack();
    }

    m_Pipelines[index].Graphics = true;
    m_Pipelines[index].Index = m_GraphicsPipelines.Size();
    m_GraphicsPipelines.EmplaceBack(pipeline);

    *outHandle = Id::MakeHandle(index, m_Pipelines[index].Generation);

    return Result::Ok;
}

Result::Code
CRHIDevice_DX11::CreateSurface(const SurfaceInitInfo& info,
    IRHISurface** surface)
{
    CRHISurface_DX11* temp{ new CRHISurface_DX11(this, m_Factory, info) };
    if (!temp) {
        return Result::ENomemory;
    }

    if (!temp->GetNative()) {
        delete temp;
        return Result::ECreateRHIObject;
    }

    *surface = temp;

    return Result::Ok;
}

Result::Code
CRHIDevice_DX11::CreateFrameGraph(const RHIGraphBuilder& builder,
    u32 flags,
    IRHIFrameGraph** outHandle) {
    CRHIFrameGraph_DX11* temp{ new CRHIFrameGraph_DX11() };
    if (!temp) {
        return Result::ENomemory;
    }

    Result::Code res{ Result::Ok };
    res = temp->Initialize(this, flags, builder);

    if (Result::Fail(res)) {
        delete temp;
        return res;
    }

    *outHandle = temp;

    return Result::Ok;
}

void
CRHIDevice_DX11::DestroyResource(RHIResource resource) {
    if (!Id::IsValid(resource)) {
        return;
    }

    u32 sparse_index{ Id::Index(resource) };
    auto& slot{ m_SparseResources[sparse_index] };

    if (slot.Generation != Id::Generation(resource)) {
        return;
    }

    const u32 dense_index{ slot.DenseIndex };
    const u32 last_dense{ m_DenseResources.Size() - 1 };
    const u32 moved_sparse{ m_DenseResources[last_dense].SparseIndex };
    DenseResource& removed{ m_DenseResources[dense_index] };

    SafeRelease(removed.Resource);

    std::swap(m_DenseResources[dense_index], m_DenseResources[last_dense]);

    m_SparseResources[moved_sparse].DenseIndex = dense_index;

    m_DenseResources.PopBack();

    ++slot.Generation;
    m_FreeSparseResources.PushBack(sparse_index);
}

void
CRHIDevice_DX11::DestroyPipelineLayout(RHIPipelineLayout layout) {
    if (!Id::IsValid(layout)) {
        return;
    }

    const u32 index{ Id::Index(layout) };
    const u32 gen{ Id::Generation(layout) };

    if (index >= m_PipelineLayouts.Size()) {
        return;
    }

    PipelineLayout& ref{ m_PipelineLayouts[index] };
    if (ref.Generation != gen) {
        return;
    }

    ++ref.Generation;
    m_FreeLayouts.PushBack(index);
}

void
CRHIDevice_DX11::DestroyPipeline(RHIPipeline pso) {
    if (!Id::IsValid(pso)) {
        return;
    }

    const u32 index{ Id::Index(pso) };
    const u32 gen{ Id::Generation(pso) };

    if (index >= m_Pipelines.Size()) {
        return;
    }

    PipelineData& data{ m_Pipelines[index] };
    if (data.Generation != gen) {
        return;
    }

    if (data.Graphics) {
        GraphicsPipeline& ref{ m_GraphicsPipelines[data.Index] };
        m_VertexShaders.Release(ref.VS);
        m_PixelShaders.Release(ref.PS);
        m_DomainShaders.Release(ref.DS);
        m_HullShaders.Release(ref.HS);
        m_GeometryShaders.Release(ref.GS);
        m_BlendStates.Release(ref.Blend);
        m_RasterStates.Release(ref.Rasterizer);
        m_DepthStates.Release(ref.Depth);
    }
    else {
        ComputePipeline& ref{ m_ComputePipelines[data.Index] };
        m_ComputeShaders.Release(ref.CS);
    }

    ++data.Generation;
    m_FreePipelines.PushBack(index);
}

void
CRHIDevice_DX11::GetFeatures(
    DeviceFeatures* features) {
    if (!features)
        return;

    *features = m_Features;
}

void* const
CRHIDevice_DX11::GetNative() const {
    return m_Device;
}

ID3D11Resource* const
CRHIDevice_DX11::ResolveResource(RHIResource handle) const {
    u32 index{ Id::Index(handle) };
    u32 gen{ Id::Index(handle) };

    const auto& slot{ m_SparseResources[index] };
    if (slot.Generation != gen)
        return nullptr;

    return m_DenseResources[slot.DenseIndex].Resource;
}

const
CRHIDevice_DX11::PipelineData
CRHIDevice_DX11::ResolvePipelineData(RHIPipeline pso) const {
    if (!Id::IsValid(pso)) {
        return {};
    }

    u32 index{ Id::Index(pso) };
    u32 gen{ Id::Index(pso) };

    const auto& slot{ m_Pipelines[index] };
    if (slot.Generation != gen)
        return {};

    return slot;
}

const CRHIDevice_DX11::PipelineLayout
CRHIDevice_DX11::ResolvePipelineLayout(RHIPipelineLayout layout) const {
    if (!Id::IsValid(layout)) {
        return {};
    }

    u32 index{ Id::Index(layout) };
    u32 gen{ Id::Index(layout) };

    const auto& slot{ m_PipelineLayouts[index] };
    if (slot.Generation != gen)
        return {};

    return slot;
}

const
CRHIDevice_DX11::GraphicsPipeline
CRHIDevice_DX11::ResolveGraphicsPipeline(const PipelineData& data) const {
    return m_GraphicsPipelines[data.Index];
}

const
CRHIDevice_DX11::ComputePipeline
CRHIDevice_DX11::ResolveComputePipeline(const PipelineData& data) const {
    return m_ComputePipelines[data.Index];
}

CRHISurface_DX11::CRHISurface_DX11(
    CRHIDevice_DX11* const device,
    IRHIFactory* const factory,
    const SurfaceInitInfo& info)
    : m_Swapchain(),
    m_Buffer(),
    m_Rtv(),
    m_BufferCount(info.TripleBuffering ? 3 : 2),
    m_SupportTearing() {
    if (!(device && factory &&
        info.Native && device->GetContext()
        && factory->GetNative())) {
        return;
    }

    const HWND hwnd{ (const HWND)info.Native };
    IDXGIFactory7* const dxgi{ (IDXGIFactory7* const)factory->GetNative() };
    ID3D11Device5* const d3d11{ (ID3D11Device5* const)device->GetNative() };
    m_SupportTearing = factory->SupportsTearing() && info.AllowTearing;

    DXGI_SWAP_CHAIN_DESC1 swap_desc{};
    swap_desc.Width = info.Width;
    swap_desc.Height = info.Height;
    swap_desc.Format = Shared::ConvertFormat(info.Format);
    swap_desc.Stereo = FALSE;
    swap_desc.SampleDesc = { 1, 0 };
    swap_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_desc.BufferCount = m_BufferCount;
    swap_desc.Scaling = DXGI_SCALING_STRETCH;
    swap_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swap_desc.Flags = m_SupportTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    ComPtr<IDXGISwapChain1> temp{};

    HRESULT hr{ S_OK };
    hr = dxgi->CreateSwapChainForHwnd(
        d3d11,
        hwnd,
        &swap_desc,
        nullptr,
        nullptr,
        &temp);

    if (FAILED(hr)) {
        LOG_ERROR("Failed to create swapchain!");
        return;
    }

    temp->QueryInterface(IID_PPV_ARGS(&m_Swapchain));

    m_Swapchain->GetBuffer(0, IID_PPV_ARGS(&m_Buffer));
    d3d11->CreateRenderTargetView1(m_Buffer, nullptr, &m_Rtv);
}

void
CRHISurface_DX11::Release() {
    SafeRelease(m_Rtv);
    SafeRelease(m_Buffer);
    SafeRelease(m_Swapchain);

    delete this;
}

void
CRHISurface_DX11::Present(bool vsync) {
    m_Swapchain->Present(vsync ? 1 : 0, vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING);
}

u32
CRHISurface_DX11::GetBufferCount() const {
    return m_BufferCount;
}

void* const
CRHISurface_DX11::GetNative() const {
    return m_Swapchain;
}

void* const
CRHISurface_DX11::GetNativeBuffer(u32 index) const {
    UNREFERENCED_PARAMETER(index);
    return m_Buffer;
}

Result::Code
CRHIFrameGraph_DX11::Initialize(
    CRHIDevice_DX11* const device,
    u32 flags,
    const RHIGraphBuilder& builder) {
    if (!device) {
        return Result::ENullptr;
    }

    m_Device = device;

    const Vector<Vector<u32>> dependencies{ GetDependencies(builder) };
    if ((u32)(flags & FGCompileFlags::LogInfo)) {
        PrintDependencies(dependencies);
    }

    m_Ctx = device->GetContext();

    ID3D11Device5* const d3d11{ (ID3D11Device5* const)device->GetNative() };
    const Vector<u32> sorted{ TopologicalSort(dependencies) };
    const Vector<RHIGraphBuilder::FGPassDesc>& passes{ builder.GetPasses() };
    const Vector<FGResourceInitInfo>& resources{ builder.GetResources() };
    const bool debug_names{ (const bool)(flags & FGCompileFlags::DebugNames) };

    m_Passes.Resize(sorted.Size());
    for (u32 i{ 0 }; i < m_Passes.Size(); ++i) {
        m_Passes[i].Func = passes[sorted[i]].Func;
    }

    {//Resource creation
        m_DescResources.Resize(resources.Size());

        u32 num_texture2ds{};
        u32 num_buffers{};

        for (u32 i{ 0 }; i < resources.Size(); ++i) {
            const FGResourceInitInfo& info{ resources[i] };
            if (info.Type == FGResourceType::Texture) {
                Resource res{};
                res.Start = (u16)num_texture2ds;
                res.Count = (u16)info.TemporalCount;
                res.Type = CompiledType::Texture;

                m_DescResources[i] = res;

                num_texture2ds += info.TemporalCount;
            }
            else if (info.Type == FGResourceType::Buffer) {
                Resource res{};
                res.Start = (u16)num_buffers;
                res.Count = (u16)info.TemporalCount;
                res.Type = CompiledType::Buffer;

                m_DescResources[i] = res;

                num_buffers += info.TemporalCount;
            }
            else if (info.Type == FGResourceType::Swapchain) {
                Resource res{};
                res.Start = 0;
                res.Count = (u16)info.TemporalCount;
                res.Type = CompiledType::Swapchain;

                m_DescResources[i] = res;
            }
        }

        m_Texture2DHeap.Resize(num_texture2ds);

        for (u32 i{ 0 }; i < resources.Size(); ++i) {
            const FGResourceInitInfo& info{ resources[i] };
            const u8 info_flags{ info.GetFlags() };
            u32 bind_flags{ 0 };
            if (info_flags & RHIGraphBuilder::ResourceFlag_SRV) {
                bind_flags |= D3D11_BIND_SHADER_RESOURCE;
            }
            if (info_flags & RHIGraphBuilder::ResourceFlag_RTV) {
                bind_flags |= D3D11_BIND_RENDER_TARGET;
            }
            if (info_flags & RHIGraphBuilder::ResourceFlag_DSV) {
                bind_flags |= D3D11_BIND_DEPTH_STENCIL;
            }
            if (info_flags & RHIGraphBuilder::ResourceFlag_UAV) {
                bind_flags |= D3D11_BIND_UNORDERED_ACCESS;
            }

            if (info.Type == FGResourceType::Texture) {
                D3D11_TEXTURE2D_DESC tex_desc{};
                tex_desc.Width = info.Width;
                tex_desc.Height = info.Height;
                tex_desc.MipLevels = info.MipLevels;
                tex_desc.ArraySize = info.DepthOrArray;
                tex_desc.Format = Shared::ConvertFormat(info.StorageFormat);
                tex_desc.SampleDesc = { 1, 0 };
                tex_desc.Usage = D3D11_USAGE_DEFAULT;
                tex_desc.BindFlags = bind_flags;
                tex_desc.CPUAccessFlags = 0;
                tex_desc.MiscFlags = 0;
                const Resource& logical{ m_DescResources[i] };

                for (u32 j{ 0 }; j < info.TemporalCount; ++j) {
                    const u32 index{ logical.Start + j };
                    HRESULT hr{ S_OK };
                    hr = d3d11->CreateTexture2D(&tex_desc, nullptr, &m_Texture2DHeap[index]);
                    if (FAILED(hr)) {
                        LOG_ERROR("Failed to create graph resource!");
                        return Result::ECreateResource;
                    }

                    if (debug_names) {
                        SetDebugName(m_Texture2DHeap[index], info.Name);
                    }
                }

                MemFree((void*)info.Name);
            }
        }
    }
    using namespace Shared;
    std::unordered_map<ViewCacheEntry, u32, ViewCacheHasher> view_cache;

    for (u32 i{ 0 }; i < (u32)m_Passes.Size(); ++i) {
        const RHIGraphBuilder::FGPassDesc& pass_desc{ passes[i] };
        Pass& compiled{ m_Passes[i] };

        for (u32 j{ 0 }; j < (u32)pass_desc.Reads.Size(); ++j) {
            const auto& read{ pass_desc.Reads[j] };
            const u32 slot{ read.Slot };
            const Resource& resource{ m_DescResources[read.Resource] };

            if (resource.Type == CompiledType::Swapchain) {
                continue;
            }

            ViewCacheEntry entry{};
            entry.Resource = read.Resource;
            entry.Format = read.ViewFormat;
            entry.BaseMip = read.Range.BaseMip;
            entry.MipCount = read.Range.MipCount;
            entry.BaseLayer = read.Range.BaseLayer;
            entry.LayerCount = read.Range.LayerCount;
            entry.Plane = read.Range.Plane;

            auto it = view_cache.find(entry);
            if (it == view_cache.end()) {
                if (read.State & ResourceState::PixelResource
                    || read.State & ResourceState::NonPixelResource) {
                    View view{};
                    view.Resource = (u16)entry.Resource;
                    view.Type = ViewType::ShaderResource;
                    view.BaseIndex = (u16)m_SrvHeap.Size();

                    D3D11_SHADER_RESOURCE_VIEW_DESC1 srv_desc{};
                    srv_desc.Format = ConvertFormat(entry.Format);
                    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                    srv_desc.Texture2D.MipLevels = entry.MipCount;
                    srv_desc.Texture2D.MostDetailedMip = entry.BaseMip;
                    srv_desc.Texture2D.PlaneSlice = entry.Plane;

                    const u32 temporal{ resource.Count };
                    HRESULT hr{ S_OK };

                    for (u32 idx{ 0 }; idx < temporal; ++idx) {
                        ID3D11Texture2D* const texture{ m_Texture2DHeap[resource.Start + idx] };

                        ID3D11ShaderResourceView1* viewhandle{ nullptr };
                        hr = d3d11->CreateShaderResourceView1(texture, &srv_desc, &viewhandle);
                        if (FAILED(hr)) {
                            return Result::ECreateView;
                        }

                        m_SrvHeap.PushBack(viewhandle);
                    }

                    const u32 view_id{ m_DescViews.Size() };
                    view_cache[entry] = view_id;
                    m_DescViews.EmplaceBack(view);

                    BoundView binding{};
                    binding.View = (u16)view_id;
                    binding.Slot = (u16)slot;
                    compiled.Srvs.PushBack(binding);
                }
                else if (read.State & ResourceState::DepthRead) {
                    View view{};
                    view.Resource = (u16)entry.Resource;
                    view.Type = ViewType::DepthStencil;
                    view.BaseIndex = (u16)m_DsvHeap.Size();

                    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
                    dsv_desc.Format = ConvertFormat(entry.Format);
                    dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                    dsv_desc.Texture2D.MipSlice = entry.BaseMip;

                    const u32 temporal{ resource.Count };
                    HRESULT hr{ S_OK };

                    for (u32 idx{ 0 }; idx < temporal; ++idx) {
                        ID3D11Texture2D* const texture{ m_Texture2DHeap[resource.Start + idx] };

                        ID3D11DepthStencilView* viewhandle{ nullptr };
                        hr = d3d11->CreateDepthStencilView(texture, &dsv_desc, &viewhandle);
                        if (FAILED(hr)) {
                            return Result::ECreateView;
                        }

                        m_DsvHeap.PushBack(viewhandle);
                    }

                    const u32 view_id{ m_DescViews.Size() };
                    view_cache[entry] = view_id;
                    m_DescViews.EmplaceBack(view);

                    compiled.HasDsv = 1;
                    compiled.Dsv.View = (u16)view_id;
                    compiled.Dsv.Slot = 0;
                }
            }
            else {
                if (read.State & ResourceState::PixelResource
                    || read.State & ResourceState::NonPixelResource) {
                    BoundView binding{};
                    binding.View = (u16)it->second;
                    binding.Slot = (u16)slot;
                    compiled.Srvs.PushBack(binding);
                }
                else if (read.State & ResourceState::DepthRead) {
                    compiled.HasDsv = 1;
                    compiled.Dsv.View = (u16)it->second;
                    compiled.Dsv.Slot = (u16)slot;
                }
            }
        }

        for (u32 j{ 0 }; j < (u32)pass_desc.Writes.Size(); ++j) {
            const auto& write{ pass_desc.Writes[j] };
            const u32 slot{ write.Slot };
            const Resource& resource{ m_DescResources[write.Resource] };

            if (write.ClearOp == FGClearOp::RenderTarget) {
                compiled.RtvClearValues[slot].X = write.ClearValue.Color[0];
                compiled.RtvClearValues[slot].Y = write.ClearValue.Color[1];
                compiled.RtvClearValues[slot].Z = write.ClearValue.Color[2];
                compiled.RtvClearValues[slot].W = write.ClearValue.Color[3];
                compiled.EnableClearTarget((u16)slot);
            }
            else if (write.ClearOp == FGClearOp::DepthStencil) {
                compiled.DepthClearValue.Depth = write.ClearValue.Depth.Depth;
                compiled.DepthClearValue.Stencil = write.ClearValue.Depth.Stencil;
                compiled.EnableClearDepth();
            }

            if (resource.Type == CompiledType::Swapchain) {

                compiled.NumRtvs++;
                compiled.Rtvs[slot].Slot = (u16)slot;
                compiled.Rtvs[slot].View = (u16)~0;
                continue;
            }

            ViewCacheEntry entry{};
            entry.Resource = write.Resource;
            entry.Format = write.ViewFormat;
            entry.BaseMip = write.Range.BaseMip;
            entry.MipCount = write.Range.MipCount;
            entry.BaseLayer = write.Range.BaseLayer;
            entry.LayerCount = write.Range.LayerCount;
            entry.Plane = write.Range.Plane;
            auto it = view_cache.find(entry);
            if (it == view_cache.end()) {
                if (write.State & ResourceState::RenderTarget) {
                    View view{};
                    view.Resource = (u16)entry.Resource;
                    view.Type = ViewType::RenderTarget;
                    view.BaseIndex = (u16)m_RtvHeap.Size();

                    D3D11_RENDER_TARGET_VIEW_DESC1 rtv_desc{};
                    rtv_desc.Format = ConvertFormat(entry.Format);
                    rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                    rtv_desc.Texture2D.MipSlice = entry.BaseMip;
                    rtv_desc.Texture2D.PlaneSlice = entry.Plane;

                    const u32 temporal{ resource.Count };
                    HRESULT hr{ S_OK };

                    for (u32 idx{ 0 }; idx < temporal; ++idx) {
                        ID3D11Texture2D* const texture{ m_Texture2DHeap[resource.Start + idx] };

                        ID3D11RenderTargetView1* viewhandle{ nullptr };
                        hr = d3d11->CreateRenderTargetView1(texture, &rtv_desc, &viewhandle);
                        if (FAILED(hr)) {
                            return Result::ECreateView;
                        }

                        m_RtvHeap.PushBack(viewhandle);
                    }

                    const u32 view_id{ m_DescViews.Size() };

                    view_cache[entry] = view_id;
                    m_DescViews.EmplaceBack(view);

                    compiled.NumRtvs++;
                    compiled.Rtvs[slot].Slot = (u16)slot;
                    compiled.Rtvs[slot].View = (u16)view_id;
                }
                else if (write.State & ResourceState::DepthWrite) {
                    View view{};
                    view.Resource = (u16)entry.Resource;
                    view.Type = ViewType::DepthStencil;
                    view.BaseIndex = (u16)m_DsvHeap.Size();

                    D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
                    dsv_desc.Format = ConvertFormat(entry.Format);
                    dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                    dsv_desc.Texture2D.MipSlice = entry.BaseMip;

                    const u32 temporal{ resource.Count };
                    HRESULT hr{ S_OK };

                    for (u32 idx{ 0 }; idx < temporal; ++idx) {
                        ID3D11Texture2D* const texture{ m_Texture2DHeap[resource.Start + idx] };

                        ID3D11DepthStencilView* viewhandle{ nullptr };
                        hr = d3d11->CreateDepthStencilView(texture, &dsv_desc, &viewhandle);
                        if (FAILED(hr)) {
                            return Result::ECreateView;
                        }

                        m_DsvHeap.PushBack(viewhandle);
                    }

                    const u32 view_id{ m_DescViews.Size() };
                    view_cache[entry] = view_id;
                    m_DescViews.EmplaceBack(view);

                    compiled.HasDsv = 1;
                    compiled.Dsv.View = (u16)view_id;
                    compiled.Dsv.Slot = 0;
                }
            }
            else {
                if (write.State & ResourceState::RenderTarget) {
                    compiled.NumRtvs++;
                    compiled.Rtvs[slot].Slot = (u16)slot;
                    compiled.Rtvs[slot].View = (u16)it->second;
                }
                else if (write.State & ResourceState::DepthRead) {
                    compiled.HasDsv = 1;
                    compiled.Dsv.View = (u16)it->second;
                    compiled.Dsv.Slot = (u16)write.Slot;
                }
            }
        }
    }

    return Result::Ok;
}

void
CRHIFrameGraph_DX11::Release() {
    for (auto& res : m_Texture2DHeap) SafeRelease(res);
    for (auto& res : m_SrvHeap) SafeRelease(res);
    for (auto& res : m_RtvHeap) SafeRelease(res);
    for (auto& res : m_DsvHeap) SafeRelease(res);
}

void
CRHIFrameGraph_DX11::Execute(
    IRHISurface* const surface,
    u64 frameNumber) {
    CRHISurface_DX11* const dx_surface{ (CRHISurface_DX11* const)surface };

    Cmd::CommandListData cmd_data{};
    cmd_data.Ctx = m_Ctx;
    cmd_data.Device = m_Device;
    if (m_Device->HasPC()) {
        cmd_data.InitPCBuffer(1024);
    }

    RHICommandBuilder builder{ 1024 * 1024 };
    builder.Reset();

    for (auto& pass : m_Passes) {
        const u32 num_rtvs{ pass.NumRtvs };
        const bool has_dsv{ (bool)pass.HasDsv };

        ID3D11RenderTargetView* targets[RHI_MAX_TARGET_COUNT]{};
        ID3D11DepthStencilView* depth{ nullptr };

        for (u16 i{ 0 }; i < num_rtvs; ++i) {
            const u16 view_handle{ pass.Rtvs[i].View };
            if (view_handle == (u16)~0) {
                targets[i] = dx_surface->GetRtv();
            }
            else {
                const View& view_desc{ m_DescViews[view_handle] };
                const Resource& resource{ m_DescResources[view_desc.Resource] };

                const u32 actual_index{ CalculateTemporal(frameNumber, view_desc.BaseIndex, resource.Count) };

                targets[i] = m_RtvHeap[actual_index];
            }

            if (pass.HasTargetClear(i)) {
                cmd_data.Ctx->ClearRenderTargetView(targets[i],
                    &pass.RtvClearValues[i].X);
            }
        }

        if (has_dsv) {
            const u16 view_handle{ pass.Dsv.View };
            const View& view_desc{ m_DescViews[view_handle] };
            const Resource& resource{ m_DescResources[view_desc.Resource] };

            const u32 actual_index{ CalculateTemporal(frameNumber, view_desc.BaseIndex, resource.Count) };

            depth = m_DsvHeap[actual_index];
            if (pass.HasDepthClear()) {

                //TODO: Only depth or stencil???
                cmd_data.Ctx->ClearDepthStencilView(depth,
                    D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
                    pass.DepthClearValue.Depth,
                    pass.DepthClearValue.Stencil);
            }
        }

        cmd_data.Ctx->OMSetRenderTargets(num_rtvs, &targets[0], depth);
        pass.Func(builder);
        Cmd::ParseCommandStream(builder, cmd_data);
        builder.Reset();
    }

    dx_surface->Present();

    cmd_data.Reset();
}

Vector<Vector<u32>>
CRHIFrameGraph_DX11::GetDependencies(
    const RHIGraphBuilder& builder) const {
    const Vector<RHIGraphBuilder::FGPassDesc>& passes{ builder.GetPasses() };
    const u32 num_passes{ passes.Size() };

    Vector<Vector<u32>> dependencies(num_passes);

    std::unordered_map<FGResource, Vector<u32>> writers;
    writers.reserve(num_passes * 2);

    for (u32 passIndex{ 0 }; passIndex < num_passes; ++passIndex) {
        for (const RHIGraphBuilder::FGResourceUsage& write : passes[passIndex].Writes) {
            writers[write.Resource].PushBack(passIndex);
        }
    }

    for (u32 passIndex{ 0 }; passIndex < num_passes; ++passIndex) {
        auto& deps = dependencies[passIndex];

        for (const RHIGraphBuilder::FGResourceUsage& read : passes[passIndex].Reads) {
            auto it = writers.find(read.Resource);
            if (it == writers.end())
                continue;

            for (u32 writerPass : it->second) {
                if (writerPass != passIndex) {
                    deps.PushBack(writerPass);
                }
            }
        }

        // Optional: remove duplicates
        std::sort(deps.begin(), deps.end());
        deps.Erase(std::unique(deps.begin(), deps.end()), deps.end());
    }

    return dependencies;
}

Vector<u32>
CRHIFrameGraph_DX11::TopologicalSort(
    const Vector<Vector<u32>>& dependencies) const {
    const u32 numPasses = static_cast<u32>(dependencies.Size());

    Vector<u32> inDegree(numPasses, 0);
    Vector<std::vector<u32>> dependents(numPasses);

    for (u32 pass = 0; pass < numPasses; ++pass) {
        for (u32 dep : dependencies[pass]) {
            ++inDegree[pass];
            dependents[dep].push_back(pass);
        }
    }

    std::queue<u32> ready;
    for (u32 i = 0; i < numPasses; ++i) {
        if (inDegree[i] == 0) {
            ready.push(i);
        }
    }

    Vector<u32> sorted;
    sorted.Reserve(numPasses);

    while (!ready.empty()) {
        u32 pass = ready.front();
        ready.pop();

        sorted.PushBack(pass);

        for (u32 dependent : dependents[pass]) {
            if (--inDegree[dependent] == 0) {
                ready.push(dependent);
            }
        }
    }

    if (sorted.Size() != numPasses) {
        // Cycle detected: graph is invalid
        LOG_ERROR("D3D12RHI: Cycle detected in Task Graph!");
    }

    return sorted;
}

void
CRHIFrameGraph_DX11::PrintDependencies(
    const Vector<Vector<u32>>& dependencies) const {
    LOG_DEBUG("Task Graph Dependencies:");

    for (u32 pass = 0; pass < dependencies.Size(); ++pass) {
        LOG_DEBUG("Pass %u depends on: ");

        if (dependencies[pass].Empty()) {
            LOG_DEBUG("(none)");
        }
        else {
            for (u32 dep : dependencies[pass]) {
                LOG_DEBUG("    %u", dep);
            }
        }
    }
}
}
