/*
RHI.h
This header contains all interfaces for Iron.RHI.Dll

All struct/enum/non-exported functions should be defined in RHIDefines.h
*/

#pragma once
#include <Iron.Core/Core.h>
#include <Iron.RHI/RHIDefines.h>

namespace Iron::RHI {
class IRHIFactory : public IObjectBase {
public:
    virtual ~IRHIFactory() = default;

    virtual u32 GetAdapterCount() const = 0;
    virtual void GetAdapters(
        IRHIAdapter** adapters,
        u32 count) const = 0;

    virtual Result::Code CreateDevice(
        IRHIAdapter* const adapter,
        const DeviceInitInfo& info,
        IRHIDevice** outHandle) = 0;

    virtual bool SupportsTearing() const = 0;
    virtual bool IsDebug() const = 0;

    virtual void* const GetNative() const = 0;
};

class IRHIAdapter : public IObjectBase {
public:
    virtual ~IRHIAdapter() = default;

    virtual const char* GetName() const = 0;
    virtual AdapterType::Type GetType() const = 0;
    virtual u32 GetVendorID() const = 0;
    virtual u32 GetDeviceID() const = 0;
    virtual void* const GetNative() const = 0;
};

class IRHIDevice : public IObjectBase {
public:
    virtual ~IRHIDevice() = default;

    virtual Result::Code CreateResource(
        const ResourceInitInfo& info,
        RHIResource* resource) = 0;

    virtual Result::Code CreatePipelineLayout(
        const PipelineLayoutInitInfo& info,
        RHIPipelineLayout* outHandle) = 0;

    virtual Result::Code CreateComputePipeline(
        const ComputePipelineInitInfo& info,
        RHIPipeline* outHandle) = 0;

    virtual Result::Code CreateGraphicsPipeline(
        const GraphicsPipelineInitInfo& info,
        RHIPipeline* outHandle) = 0;

    virtual Result::Code CreateSurface(
        const SurfaceInitInfo& info,
        IRHISurface** surface) = 0;

    virtual Result::Code CreateFrameGraph(
        const RHIGraphBuilder& builder,
        u32 flags,
        IRHIFrameGraph** outHandle) = 0;

    virtual void DestroyResource(
        RHIResource resource) = 0;

    virtual void DestroyPipelineLayout(
        RHIPipelineLayout layout) = 0;

    virtual void DestroyPipeline(
        RHIPipeline pso) = 0;

    virtual void GetFeatures(
        DeviceFeatures* features) = 0;

    virtual void* const GetNative() const = 0;
};

class IRHISurface : public IObjectBase {
public:
    virtual ~IRHISurface() = default;

    virtual u32 GetBufferCount() const = 0;
    virtual void* const GetNative() const = 0;
    virtual void* const GetNativeBuffer(u32 index) const = 0;
};

class IRHIFrameGraph : public IObjectBase {
public:
    virtual ~IRHIFrameGraph() = default;

    virtual void Execute(
        IRHISurface* const surface,
        u64 frameNumber) = 0;

    virtual void WaitIdle() = 0;
};

//TODO: Rework for better clarity
class RHIGraphBuilder {
    static constexpr u32 InvalidPass = ~0u;
public:
    constexpr static u8 ResourceFlag_SRV{ 0x01 };
    constexpr static u8 ResourceFlag_RTV{ 0x02 };
    constexpr static u8 ResourceFlag_DSV{ 0x04 };
    constexpr static u8 ResourceFlag_UAV{ 0x08 };

    struct FGResourceUsage
    {
        FGResource                      Resource;
        ResourceState::State            State;
        RHIFormat::Fmt                  ViewFormat;
        SubresourceRange                Range;
        u32                             Slot;
        FGClearOp::Op                   ClearOp{ FGClearOp::None };
        ClearValue                      ClearValue;
    };

    struct FGPassDesc
    {
        const char*                     Name{};
        FGPassFunc                      Func{};
        Vector<FGResourceUsage>         Reads{};
        Vector<FGResourceUsage>         Writes{};
    };

public:
    FGResource RegisterOutput(
        const char* name,
        u32 width,
        u32 height,
        RHIFormat::Fmt format,
        bool tripleBuffering) {
        FGResourceInitInfo desc(
            name,
            FGResourceType::Swapchain,
            width,
            height,
            format,
            1,
            1,
            tripleBuffering ? 3 : 2
        );

        FGResource handle{ (FGResource)m_Resources.Size() };
        m_Resources.PushBack(desc);
        return handle;
    }

    FGResource CreateResource(
        const FGResourceInitInfo& desc) {
        if (!FmtIsTypeless(desc.StorageFormat)) {
            LOG_ERROR("RHI: Task Graph base resource MUST be typeless!");
            return (FGResource)~0;
        }

        FGResource handle{ (FGResource)m_Resources.Size() };

        m_Resources.PushBack(desc);
        return handle;
    }

    u32 BeginPass(
        const char* name,
        FGPassFunc func) {
        if (m_BuildingPass) {
            LOG_ERROR("RHI: Nested passes are not allowed");
        }
        if (!func) {
            LOG_ERROR("RHI: Nullptr func passed to BeginPass()!");
        }

        u32 handle{ m_Passes.Size() };

        FGPassDesc pass{};
        pass.Name = StrDup(name);
        pass.Func = func;

        m_Passes.PushBack(pass);
        m_CurrentPass = handle;
        m_BuildingPass = true;

        return handle;
    }

    void EndPass() {
        ValidatePass();
        m_BuildingPass = false;
        m_CurrentPass = InvalidPass;
    }

    void AddDepth(FGResource    resource,
        RHIFormat::Fmt          viewFormat,
        u32                     state,
        SubresourceRange        range = {}) {
        ValidatePass();
        AddFlags(resource, (ResourceState::State)state);
        const FGResourceUsage usage{ resource, (ResourceState::State)state, viewFormat, range, 0 };
        if (state & ResourceState::DepthRead) {
            m_Passes[m_CurrentPass].Reads.PushBack(usage);
        }
        else {
            m_Passes[m_CurrentPass].Writes.PushBack(usage);
        }
    }

    void Read(
        FGResource          resource,
        RHIFormat::Fmt      viewFormat,
        u32                 state,
        u32                 slot,
        SubresourceRange    range = {}) {
        ValidatePass();
        AddFlags(resource, (ResourceState::State)state);
        const FGResourceUsage usage{ resource, (ResourceState::State)state, viewFormat, range, slot };
        m_Passes[m_CurrentPass].Reads.PushBack(usage);
    }

    void Write(
        FGResource          resource,
        RHIFormat::Fmt      viewFormat,
        u32                 state,
        u32                 slot,
        SubresourceRange    range = {}) {
        ValidatePass();
        AddFlags(resource, (ResourceState::State)state);
        const FGResourceUsage usage{ resource, (ResourceState::State)state, viewFormat, range, slot };
        m_Passes[m_CurrentPass].Writes.PushBack(usage);
    }

    void WriteClear(
        FGResource          resource,
        RHIFormat::Fmt      viewFormat,
        u32                 state,
        u32                 slot,
        ClearValue          clear,
        SubresourceRange    range = {}) {
        ValidatePass();
        AddFlags(resource, (ResourceState::State)state);
        const FGResourceUsage usage{ resource, (ResourceState::State)state, viewFormat, range, slot,
        FmtIsDepth(viewFormat)
                        ? FGClearOp::DepthStencil
                        : FGClearOp::RenderTarget,
            clear };
        m_Passes[m_CurrentPass].Writes.PushBack(usage);
    }

    void ReadWrite(
        FGResource          resource,
        RHIFormat::Fmt      viewFormat,
        u32                 state,
        u32                 slot,
        SubresourceRange    range = {}) {
        ValidatePass();
        AddFlags(resource, (ResourceState::State)state);
        m_Passes[m_CurrentPass].Writes.PushBack(
            { resource, (ResourceState::State)state, viewFormat, range, slot });
    }

    const Vector<FGPassDesc>& GetPasses() const {
        return m_Passes;
    }

    const Vector<FGResourceInitInfo>& GetResources() const {
        return m_Resources;
    }

    const u32 GetSRViewCounter() const { return m_NumSrvs; }
    const u32 GetRTViewCounter() const { return m_NumRtvs; }
    const u32 GetDSViewCounter() const { return m_NumDsvs; }
    const u32 GetUAViewCounter() const { return m_NumUavs; }

private:
    void AddFlags(FGResource resource, ResourceState::State state) {
        FGResourceInitInfo& info{ m_Resources[resource] };

        info.Last_State = state;

        if (state & ResourceState::RenderTarget) {
            info.Flag_Field |= ResourceFlag_RTV;
            m_NumRtvs += info.TemporalCount;
        }
        if (state & ResourceState::DepthRead
            || state & ResourceState::DepthWrite) {
            info.Flag_Field |= ResourceFlag_DSV;
            m_NumDsvs += info.TemporalCount;
        }
        if (state & ResourceState::PixelResource
            || state & ResourceState::NonPixelResource) {
            info.Flag_Field |= ResourceFlag_SRV;
            m_NumSrvs += info.TemporalCount;
        }
        if (state & ResourceState::UnorderedAccess) {
            info.Flag_Field |= ResourceFlag_UAV;
            m_NumUavs += info.TemporalCount;
        }
    }

    void ValidatePass() const {
        if (!m_BuildingPass) {
            LOG_ERROR("RHI: No active pass!");
        }
        if (m_CurrentPass == InvalidPass) {
            LOG_ERROR("RHI: Invalid pass!");
        }
    }

    void Reset() {
        m_Passes.Clear();
        m_Resources.Clear();
        m_CurrentPass = InvalidPass;
        m_BuildingPass = false;
    }


    Vector<FGPassDesc>          m_Passes{};
    Vector<FGResourceInitInfo>  m_Resources{};

    u32                         m_NumSrvs{};
    u32                         m_NumRtvs{};
    u32                         m_NumDsvs{};
    u32                         m_NumUavs{};
    u32                         m_CurrentPass{ InvalidPass };
    bool                        m_BuildingPass{ false };
};
}
