#include <Editor/Assets/AssetRegistry.h>

#include <External/Nlohmann/json.hpp>

using json = nlohmann::json;

namespace Iron::Editor::Assets {
AssetRegistry   g_AssetRegistry{};

Result::Code
AssetRegistry::Load(const char* path) {
    u8* blob{};
    u64 length{};

    Result::Code res{ Result::Ok };
    res = ReadFile(path, blob, length);

    if (Result::Fail(res)) {
        return res;
    }

    try {
        json j{ json::parse(blob, blob + length) };
        if (!j.contains("lastpacktime")) {
            LOG_ERROR("Invalid registry json!");
            return Result::EInvalidData;
        }

        if (!j.contains("assets")) {
            LOG_WARNING("No assets in registry!");
            return Result::Ok;
        }
        json asset_infos{ j.at("assets") };

        for (auto& info : asset_infos) {
            
        }
    }
    catch (json::parse_error& e) {
        LOG_ERROR("Json parse error: %s", e.what());
    }
    catch (json::out_of_range& e) {
        LOG_ERROR("Json missing key: %s", e.what());
    }

    MemFree(blob);

    return Result::Ok;
}
}
