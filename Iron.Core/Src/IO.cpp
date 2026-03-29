#include <Iron.Core/Core.h>

#include <fstream>
#include <filesystem>

namespace Iron {
Result::Code
WriteFile(const char* file,
    const u8* const data,
    u64 length) {
    if (!(file && data && length)) {
        return Result::ENullptr;
    }

    const std::filesystem::path directory{ std::filesystem::path(file).parent_path() };
    if (!std::filesystem::exists(directory)) {
        std::filesystem::create_directories(directory);
    }

    std::ofstream ofile{ file, std::ios::out | std::ios::binary };
    ofile.write((char*)data, length);
    ofile.flush();
    ofile.close();

    if (!std::filesystem::exists(file)) {
        return Result::EWritefile;
    }

    return Result::Ok;
}

Result::Code
ReadFile(const char* file,
    u8*& data,
    u64& length) {
    if (!file) {
        return Result::ENullptr;
    }

    if (!std::filesystem::exists(file)) {
        return Result::ELoadfile;
    }

    const u64 size{ std::filesystem::file_size(file) };

    if (!size) {
        return Result::ELoadfile;
    }

    std::ifstream ifile{ file, std::ios::in | std::ios::binary };
    if (!ifile.is_open()) {
        return Result::ELoadfile;
    }

    data = (u8*)MemAlloc(size);
    if (!data) {
        return Result::ENomemory;
    }

    length = size;
    ifile.read((char*)data, length);

    return Result::Ok;
}
}
