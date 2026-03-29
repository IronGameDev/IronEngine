#include <Iron.RHI/RHI.h>

namespace Iron {
Result::Code BuildPipelineFromJson(
    const char* path,
    RHI::IRHIDevice* const device,
    RHI::RHIPipeline* const pipeline);
}
