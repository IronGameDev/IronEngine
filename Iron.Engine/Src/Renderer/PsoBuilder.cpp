#include <Iron.Engine/Src/Renderer/PsoBuilder.h>

#include <filesystem>
#include <External/Nlohmann/json.hpp>

using json = nlohmann::json;

namespace Iron {
Result::Code
BuildPipelineFromJson(
    const char* path,
    RHI::IRHIDevice* const device,
    RHI::RHIPipeline* const pipeline) {
    Result::Code res{ Result::Ok };
    u8* blob{};
    u64 length{};
    res = ReadFile(path, blob, length);
    if (Result::Fail(res)) {
        return res;
    }

    try {
        json data = json::parse((char*)blob);
        

    }
    catch (json::parse_error& e) {
        LOG_ERROR("Json parse error: %s", e.what());
    }
    catch (json::out_of_range& e) {
        LOG_ERROR("Json missing key: %s", e.what());
    }

    MemFree(blob);
}
}
