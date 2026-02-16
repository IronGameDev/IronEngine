#include <Iron.Core/Core.h>

#include <algorithm>
#include <cstdlib>
#include <ctype.h>

namespace Iron {
namespace {
static char* StrDup(const char* S) {
    if (!S) return nullptr;
    size_t Len{ StrLen(S) + 1 };
    char* Out = (char*)MemAlloc(Len);
    if (!Out) return nullptr;
    memcpy(Out, S, Len);
    return Out;
}

static char* Trim(char* Str) {
    if (!Str) return nullptr;

    while (isspace(*Str)) ++Str;

    char* End{ Str + strlen(Str) };
    while (End > Str && isspace(*(End - 1))) --End;
    *End = '\0';

    return Str;
}

static void ParseLine(ConfigFile& Cfg, char* Line, char*& CurrentSection) {
    Line = Trim(Line);
    if (!Line || *Line == '\0')
        return;

    if (*Line == ';' || *Line == '#')
        return;

    if (*Line == '[') {
        char* End{ strchr(Line, ']') };
        if (!End) return;

        *End = '\0';
        CurrentSection = Trim(Line + 1);
        return;
    }

    char* Equal{ strchr(Line, '=') };
    if (!Equal) return;

    *Equal = '\0';
    char* Key = Trim(Line);
    char* Value = Trim(Equal + 1);

    if (!CurrentSection)
        CurrentSection = (char*)"default";

    Cfg.Set(CurrentSection, Key, Value);
}

static void ParseBuffer(ConfigFile& Cfg, const char* Buffer) {
    if (!Buffer) return;

    const size_t Len{ StrLen(Buffer) + 1 };
    char* Copy{ (char*)MemAlloc(Len) };
    if (!Copy) return;
    strcpy_s(Copy, Len, Buffer);

    char* CurrentSection{ nullptr };
    char* Line{ Copy };

    while (*Line) {
        char* Next{ strchr(Line, '\n') };
        if (Next) {
            *Next = '\0';
            ++Next;
        }
        ParseLine(Cfg, Line, CurrentSection);
        Line = Next;
    }

    MemFree(Copy);
}

static bool ParseFile(ConfigFile& Cfg, const char* Path) {
    if (!Path) return false;

    FILE* F{ nullptr };
    fopen_s(&F, Path, "r");

    if (!F) return false;

    fseek(F, 0, SEEK_END);
    long Size{ std::ftell(F) };
    fseek(F, 0, SEEK_SET);

    if (Size <= 0) {
        fclose(F);
        return false;
    }

    char* Buffer{ (char*)MemAlloc(Size + 1) };
    if (!Buffer) {
        fclose(F);
        return false;
    }

    size_t Read{ fread(Buffer, 1, Size, F) };
    Buffer[Read] = '\0';
    fclose(F);

    ParseBuffer(Cfg, Buffer);
    MemFree(Buffer);

    return true;
}
} // anonymous namespace


ConfigFile::~ConfigFile()
{
    Clear();
}

Result::Code
ConfigFile::Load(const char* File) {
    if (!ParseFile(*this, File)) {
        return Result::ELoadfile;
    }

    LOG_INFO("Loaded config file %s", File);

    return Result::Ok;
}

Result::Code
ConfigFile::Save(const char* Path) const {
    if (!Path) return Result::EInvalidarg;

    FILE* F{ nullptr };
    fopen_s(&F, Path, "w");
    if (!F) return Result::EWritefile;

    const char* CurrentSection{ nullptr };

    for (u32 I{ 0 }; I < m_Entries.Size(); ++I) {
        const Entry& E{ m_Entries[I] };

        if (!CurrentSection || std::strcmp(CurrentSection, E.Section) != 0) {
            CurrentSection = E.Section;
            fprintf(F, "[%s]\n", CurrentSection);
        }

        fprintf(F, "%s=%s\n", E.Keyword, E.Value);
    }

    fclose(F);

    LOG_INFO("Saved config file to %s", Path);

    return Result::Ok;
}

void
ConfigFile::Set(
    const char* Section,
    const char* Keyword,
    const char* Value) {
    if (!Section || !Keyword)
        return;

    for (u32 I{ 0 }; I < m_Entries.Size(); ++I) {
        Entry& E{ m_Entries[I] };
        if (std::strcmp(E.Section, Section) == 0 &&
            std::strcmp(E.Keyword, Keyword) == 0)
        {
            MemFree((void*)E.Value);
            E.Value = StrDup(Value ? Value : "");
            return;
        }
    }

    Entry E{};
    E.Section = StrDup(Section ? Section : "");
    E.Keyword = StrDup(Keyword ? Keyword : "");
    E.Value = StrDup(Value ? Value : "");
    m_Entries.PushBack(E);
}

const char*
ConfigFile::Get(
    const char* Section,
    const char* Keyword,
    const char* DefaultValue) const {
    if (!Section || !Keyword)
        return DefaultValue;

    for (u32 I{ 0 }; I < m_Entries.Size(); ++I) {
        const Entry& E{ m_Entries[I] };
        if (std::strcmp(E.Section, Section) == 0 &&
            std::strcmp(E.Keyword, Keyword) == 0)
        {
            return E.Value;
        }
    }

    return DefaultValue;
}

void
ConfigFile::Clear() {
    for (u32 I{ 0 }; I < m_Entries.Size(); ++I) {
        MemFree((void*)m_Entries[I].Section);
        MemFree((void*)m_Entries[I].Keyword);
        MemFree((void*)m_Entries[I].Value);
    }
    m_Entries.Clear();
}
}
