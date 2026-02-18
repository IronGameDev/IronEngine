#pragma once
#include <Iron.Core/Core.h>

#define RHI_MAX_NAME 128

namespace Iron::RHI {
typedef u32 FGResource;

struct RHIBackend {
    enum Value : u32 {
        DirectX11 = 0,
        DirectX12,
    };
};

struct AdapterType {
    enum Type : u32 {
        Unknown = 0,
        Discrete,
        Integrated,
        Virtual,
    };
};

struct DeviceInitInfo {
    RHIBackend      Backend;
    bool            Debug;
};

struct FGResourceInitInfo {
};

class IRHIAdapter;
class IRHIDevice;

class IRHIFactory : public IObjectBase {
public:
    virtual ~IRHIFactory() = default;

    virtual u32 GetAdapterCount() const = 0;
    virtual void GetAdapters(
        IRHIAdapter** adapters,
        u32 count) const = 0;

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
    
    virtual void* const GetNative() const = 0;
};

class IRHIFrameGraph : public IObjectBase {
public:
    virtual ~IRHIFrameGraph() = default;
};

class RHIGraphBuilder {
public:
    FGResource CreateResource() {

    }

private:
    
};
}
