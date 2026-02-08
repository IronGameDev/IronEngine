#include <Iron.Core/Core.h>

#include <algorithm>
#include <cstdlib>
#include <ctype.h>

namespace iron {
namespace {
static char* str_dup(const char* s) {
    if (!s) return nullptr;
    size_t len{ str_len(s) + 1 };
    char* out = (char*)mem_alloc(len);
    if (!out) return nullptr;
    memcpy(out, s, len);
    return out;
}

static char* trim(char* str) {
    if (!str) return nullptr;

    while (isspace(*str)) ++str;

    char* end{ str + strlen(str) };
    while (end > str && isspace(*(end - 1))) --end;
    *end = '\0';

    return str;
}

static void parse_line(config_file& cfg, char* line, char*& current_section) {
    line = trim(line);
    if (!line || *line == '\0')
        return;

    if (*line == ';' || *line == '#')
        return;

    if (*line == '[') {
        char* end{ strchr(line, ']') };
        if (!end) return;

        *end = '\0';
        current_section = trim(line + 1);
        return;
    }

    char* equal{ strchr(line, '=') };
    if (!equal) return;

    *equal = '\0';
    char* key = trim(line);
    char* value = trim(equal + 1);

    if (!current_section)
        current_section = (char*)"default";

    cfg.set(current_section, key, value);
}

static void parse_buffer(config_file& cfg, const char* buffer) {
    if (!buffer) return;

    const size_t len{ str_len(buffer) + 1};
    char* copy{ (char*)mem_alloc(len) };
    if (!copy) return;
    strcpy_s(copy, len, buffer);

    char* current_section{ nullptr };
    char* line{ copy };

    while (*line) {
        char* next{ strchr(line, '\n') };
        if (next) {
            *next = '\0';
            ++next;
        }
        parse_line(cfg, line, current_section);
        line = next;
    }

    mem_free(copy);
}

static bool parse_file(config_file& cfg, const char* path) {
    if (!path) return false;

    FILE* f{ nullptr };
    fopen_s(&f, path, "r");

    if (!f) return false;

    fseek(f, 0, SEEK_END);
    long size{ std::ftell(f) };
    fseek(f, 0, SEEK_SET);

    if (size <= 0) {
        fclose(f);
        return false;
    }

    char* buffer{ (char*)mem_alloc(size + 1) };
    if (!buffer) {
        fclose(f);
        return false;
    }

    size_t read{ fread(buffer, 1, size, f) };
    buffer[read] = '\0';
    fclose(f);

    parse_buffer(cfg, buffer);
    mem_free(buffer);

    return true;
}
}//anonymous namespace

config_file::~config_file()
{
    clear();
}

result::code
config_file::load(const char* file) {
    if (!parse_file(*this, file)) {
        return result::e_loadfile;
    }

    LOG_INFO("Loaded config file %s", file);

    return result::ok;
}

result::code
config_file::save(const char* path) const {
    if (!path) return result::e_invalidarg;

    FILE* f{ nullptr };
    fopen_s(&f, path, "w");
    if (!f) return result::e_writefile;

    const char* current_section{ nullptr };

    for (u32 i{ 0 }; i < m_entries.size(); ++i) {
        const entry& e{ m_entries[i] };

        if (!current_section || std::strcmp(current_section, e.section) != 0) {
            current_section = e.section;
            fprintf(f, "[%s]\n", current_section);
        }

        fprintf(f, "%s=%s\n", e.keyword, e.value);
    }

    fclose(f);

    LOG_INFO("Saved config file to %s", path);

    return result::ok;
}

void
config_file::set(
    const char* section,
    const char* keyword,
    const char* value) {
    if (!section || !keyword)
        return;

    for (u32 i{ 0 }; i < m_entries.size(); ++i) {
        entry& e{ m_entries[i] };
        if (std::strcmp(e.section, section) == 0 &&
            std::strcmp(e.keyword, keyword) == 0)
        {
            mem_free((void*)e.value);
            e.value = str_dup(value ? value : "");
            return;
        }
    }

    entry e{};
    e.section = str_dup(section ? section : "");
    e.keyword = str_dup(keyword ? keyword : "");
    e.value = str_dup(value ? value : "");
    m_entries.push_back(e);
}

const char*
config_file::get(
    const char* section,
    const char* keyword,
    const char* defaultValue) const {
    if (!section || !keyword)
        return defaultValue;

    for (u32 i{ 0 }; i < m_entries.size(); ++i) {
        const entry& e{ m_entries[i] };
        if (std::strcmp(e.section, section) == 0 &&
            std::strcmp(e.keyword, keyword) == 0)
        {
            return e.value;
        }
    }

    return defaultValue;
}

void
config_file::clear() {
    for (u32 i{ 0 }; i < m_entries.size(); ++i) {
        mem_free((void*)m_entries[i].section);
        mem_free((void*)m_entries[i].keyword);
        mem_free((void*)m_entries[i].value);
    }
    m_entries.clear();
}
}
