#pragma once
#include <Iron.RHI/Renderer.h>

#include <dxgi1_6.h>

namespace Iron::RHI::Shared {
constexpr static DXGI_FORMAT
ConvertFormat(RHIFormat::Fmt fmt) {
	return (DXGI_FORMAT)fmt;
}
}
