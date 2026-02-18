#pragma once

#if !(defined(_WIN32) || defined(_WIN64))
#error "Non-Windows platforms are not supported!"
#endif

#ifdef CORE_EXPORT
#define CORE_API __declspec(dllexport)
#else
#define CORE_API __declspec(dllimport)
#endif

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

typedef float f32;

typedef u32 TypeId;

struct Version {
    u32 Major : 11;
    u32 Minor : 11;
    u32 Patch : 10;

    Version() = default;
    constexpr Version(u32 VMajor, u32 VMinor, u32 VPatch)
        : Major(VMajor), Minor(VMinor), Patch(VPatch) {
    }

    constexpr static inline u32 Pack(const Version& V) {
        return V.Major << 21 | V.Minor << 10 | V.Patch;
    }

    constexpr static inline Version Unpack(u32 V) {
        Version R{};
        R.Major = (V >> 21) & 0x7ff;
        R.Minor = (V >> 10) & 0x7ff;
        R.Patch = V & 0x3ff;
        return R;
    }
};

constexpr static u64 Fnv1A(const char* Str) {
    u64 Hash = 14695981039346656037ull;
    while (*Str) {
        Hash ^= static_cast<u8>(*Str++);
        Hash *= 1099511628211ull;
    }
    return Hash;
}

#ifdef _DEBUG
#define LOG_DEBUG(x, ...) ::Iron::Log(::Iron::LogLevel::Debug, __FILE__, __LINE__, x, __VA_ARGS__)
#else
#define LOG_DEBUG(x, ...)
#endif
#define LOG_INFO(x, ...) ::Iron::Log(::Iron::LogLevel::Info, __FILE__, __LINE__, x, __VA_ARGS__)
#define LOG_WARNING(x, ...) ::Iron::Log(::Iron::LogLevel::Warning, __FILE__, __LINE__, x, __VA_ARGS__)
#define LOG_ERROR(x, ...) ::Iron::Log(::Iron::LogLevel::Error, __FILE__, __LINE__, x, __VA_ARGS__)
#define LOG_FATAL(x, ...) ::Iron::Log(::Iron::LogLevel::Fatal, __FILE__, __LINE__, x, __VA_ARGS__)
#define LOG_RESULT(x) ::Iron::LogError(x, __FILE__, __LINE__)

#ifndef UNLIKELY
#define UNLIKELY [[unlikely]]
#endif

#ifndef MAKE_VERSION
#define MAKE_VERSION(Mod, Major, Minor, Patch) constexpr static inline Version Mod##_HeaderVersion(Major, Minor, Patch);
#endif

#define IRON_MAX_PATH 260


namespace Iron {
struct Result {
    enum Code : u32 {
        Ok = 0,
        ENomemory,
        ENullptr,
        EInvalidarg,
        EIncomplete,
        ECreatewindow,
        ELoadlibrary,
        EGetvtable,
        ESizemismatch,
        ELoadfile,
        EWritefile,
        ENointerface,
        ELoadIcon,

        Count,
    };

    constexpr static inline bool Success(Code C) {
        return C == Ok;
    }

    constexpr static inline bool Fail(Code C) {
        return !(C == Ok || C == EIncomplete);
    }
};

struct ModuleState {
    enum State : u32 {
        Complete = 0,
        Incomplete,
        Invalid
    };

    constexpr static inline bool IsValid(State S) {
        return S != Invalid;
    }
};

struct LogLevel {
    enum Level : u32 {
        Debug = 0,
        Info,
        Warning,
        Error,
        Fatal,
        Count,
    };
};

struct InputType {
    enum Type : u32 {
        Press = 0,
        Release,
        Hold
    };
};

struct Key {
    enum Code : u32 {
        KeyA, KeyB, KeyC, KeyD, KeyE, KeyF, KeyG,
        KeyH, KeyI, KeyJ, KeyK, KeyL, KeyM, KeyN,
        KeyO, KeyP, KeyQ, KeyR, KeyS, KeyT, KeyU,
        KeyV, KeyW, KeyX, KeyY, KeyZ,
        Key1, Key2, Key3, Key4, Key5,
        Key6, Key7, Key8, Key9, Key0,
        KeyF1, KeyF2, KeyF3, KeyF4, KeyF5,
        KeyF6, KeyF7, KeyF8, KeyF9, KeyF10,
        KeyF11, KeyF12,
        KeyShift, KeyAlt, KeyControl,
        KeyEnter, KeySpace,
        KeyNum1, KeyNum2, KeyNum3, KeyNum4,
        KeyNum5, KeyNum6, KeyNum7, KeyNum8,
        KeyNum9, KeyNum0,
        KeyCaps, KeyHome, KeyPageUp,
        KeyPageDown, KeyDelete, KeyEscape,
        Count
    };
};

struct ModifierKey {
    enum Code : u32 {
        Shift = Key::KeyShift,
        Alt = Key::KeyAlt,
        Control = Key::KeyControl,
    };
};

constexpr static inline size_t StrLen(const char* Str) {
    size_t Len{ 0 };
    while (*Str++ != '\0') ++Len;
    return Len;
}

template<typename T>
constexpr static inline T MinT(T X, T Y) {
    return X < Y ? X : Y;
}

template<typename T>
constexpr static inline T MaxT(T X, T Y) {
    return X > Y ? X : Y;
}

CORE_API void* MemAlloc(size_t Size);
CORE_API void MemFree(void* Block);
CORE_API void MemSet(void* Dst, u8 Value, size_t Size);
CORE_API void MemCopy(void* Dst, const void* Src, size_t Size);
CORE_API void MemCopyS(void* Dst, const void* Src, size_t DstSize, size_t SrcSize);

CORE_API void Log(LogLevel::Level Level, const char* File, int Line, const char* Msg, ...);
CORE_API void EnableLogLevel(LogLevel::Level Level, bool Enable);
CORE_API void EnableLogIncludePath(bool Enable);
CORE_API void LogError(Result::Code Code, const char* File, int Line);

class IObjectBase {
public:
    virtual ~IObjectBase() = default;

    virtual void Release() = 0;
};

template<typename T>
void SafeRelease(T*& ptr) {
    if (ptr) {
        ptr->Release();
        ptr = nullptr;
    }
}

//Important! No constructors/destructors will be called here!

template<typename T>
class Vector {
public:
    using ValueType = T;

    Vector() : m_Data(nullptr), m_Size(0), m_Capacity(0) {}

    Vector(const Vector& Other)
        : m_Data(nullptr), m_Size(0), m_Capacity(0) {
        Reserve(Other.m_Size);
        m_Size = Other.m_Size;
        MemCopy(m_Data, Other.m_Data, m_Size * sizeof(T));
    }

    Vector& operator=(const Vector& Other) {
        if (this == &Other) return *this;
        Clear();
        Reserve(Other.m_Size);
        m_Size = Other.m_Size;
        MemCopy(m_Data, Other.m_Data, m_Size * sizeof(T));
        return *this;
    }

    ~Vector() {
        if (m_Data) MemFree(m_Data);
    }

    void Reserve(u32 NewCapacity) {
        if (NewCapacity <= m_Capacity) return;
        T* NewData = (T*)MemAlloc(NewCapacity * sizeof(T));
        if (!NewData) return;
        if (m_Data) {
            MemCopy(NewData, m_Data, m_Size * sizeof(T));
            MemFree(m_Data);
        }
        m_Data = NewData;
        m_Capacity = NewCapacity;
    }

    void Resize(u32 NewSize) {
        if (NewSize > m_Capacity)
            Reserve(GrowCapacity(NewSize));
        m_Size = NewSize;
    }

    void Clear() { m_Size = 0; }

    void PushBack(const T& Value) {
        if (m_Size >= m_Capacity)
            Reserve(GrowCapacity(m_Size + 1));
        m_Data[m_Size++] = Value;
    }

    void PopBack() {
        if (m_Size > 0) --m_Size;
    }

    void Erase(u32 Index) {
        if (Index >= m_Size) return;
        for (u32 I = Index; I < m_Size - 1; ++I)
            m_Data[I] = m_Data[I + 1];
        --m_Size;
    }

    T& operator[](u32 Index) { return m_Data[Index]; }
    const T& operator[](u32 Index) const { return m_Data[Index]; }

    inline T* Data() { return m_Data; }
    inline const T* Data() const { return m_Data; }

    inline T* Begin() { return m_Data; }
    inline const T* Begin() const { return m_Data; }

    inline T* End() { return &m_Data[m_Size]; }
    inline const T* End() const { return &m_Data[m_Size]; }

    constexpr u32 Size() const { return m_Size; }
    constexpr u32 Capacity() const { return m_Capacity; }
    constexpr bool Empty() const { return m_Size == 0; }

private:
    T*      m_Data;
    u32     m_Size;
    u32     m_Capacity;

    static u32 GrowCapacity(u32 MinCapacity) {
        u32 Cap = MinCapacity > 1 ? MinCapacity : 1;
        Cap |= Cap >> 1;
        Cap |= Cap >> 2;
        Cap |= Cap >> 4;
        Cap |= Cap >> 8;
        Cap |= Cap >> 16;
        return Cap + 1;
    }
};

template<typename T>
class FreeList {
public:
    static_assert(sizeof(T) >= sizeof(size_t),
        "FreeList<T> requires sizeof(T) >= sizeof(size_t)");

    FreeList()
        : m_FreeHead(Npos) {
    }

    size_t Allocate() {
        if (m_FreeHead != Npos) {
            const size_t Index{ m_FreeHead };
            m_FreeHead = ReadNext(Index);
            return Index;
        }

        const size_t Index{ m_Data.Size() };
        m_Data.Resize((u32)(Index + 1));
        return Index;
    }

    void Free(size_t Index) {
        WriteNext(Index, m_FreeHead);
        m_FreeHead = Index;
    }

    T& operator[](size_t Index) {
        return m_Data[(u32)Index];
    }

    const T& operator[](size_t Index) const {
        return m_Data[(u32)Index];
    }

    T* Data() {
        return m_Data.Data();
    }

    const T* Data() const {
        return m_Data.Data();
    }

    constexpr size_t Size() const {
        return m_Data.Size();
    }

    void Clear() {
        m_Data.Clear();
        m_FreeHead = Npos;
    }

    bool Valid(size_t Index) const {
        return Index < m_Data.Size();
    }

    static constexpr size_t Npos = (size_t)-1;

private:
    Vector<T> m_Data;
    size_t    m_FreeHead;

    inline size_t ReadNext(size_t Index) const {
        return *(const size_t*)(&m_Data[(u32)Index]);
    }

    inline void WriteNext(size_t Index, size_t Next) {
        *(size_t*)(&m_Data[(u32)Index]) = Next;
    }
};

class ConfigFile {
public:
    ConfigFile() = default;
    CORE_API ~ConfigFile();

    CORE_API Result::Code Load(const char* File);
    CORE_API Result::Code Save(const char* Path) const;

    CORE_API void Set(
        const char* Section,
        const char* Keyword,
        const char* Value);

    CORE_API const char* Get(
        const char* Section,
        const char* Keyword,
        const char* DefaultValue = nullptr) const;

    CORE_API void Clear();

private:
    struct Entry {
        const char* Section;
        const char* Keyword;
        const char* Value;
    };

    Vector<Entry> m_Entries{};
};

namespace Math {
constexpr static inline f32 Inv255{ 1.f / 255.f };

CORE_API f32 SqrtF(f32 Value);

constexpr f32 ConstexprRsqrt(f32 X) noexcept {
    if (X <= 0.0f) return 0.0f;

    f32 Y{ 1.0f };
    for (u32 I{ 0 }; I < 6; ++I) {
        Y = Y * (1.5f - 0.5f * X * Y * Y);
    }
    return Y;
}

template <typename V, int N>
struct VecBase;

template <typename V>
struct VecBase<V, 2> {
    f32 X, Y;

    constexpr VecBase() noexcept : X(), Y() {}
    constexpr explicit VecBase(f32 Vv) noexcept : X(Vv), Y(Vv) {}
    constexpr VecBase(f32 X_, f32 Y_) noexcept : X(X_), Y(Y_) {}

    constexpr bool operator==(const V& O) const noexcept {
        return X == O.X && Y == O.Y;
    }

    constexpr bool operator!=(const V& O) const noexcept {
        return !(*this == O);
    }

    constexpr V& operator+=(const V& O) noexcept {
        X += O.X; Y += O.Y;
        return static_cast<V&>(*this);
    }

    constexpr V& operator-=(const V& O) noexcept {
        X -= O.X; Y -= O.Y;
        return static_cast<V&>(*this);
    }

    constexpr V& operator*=(f32 S) noexcept {
        X *= S; Y *= S;
        return static_cast<V&>(*this);
    }

    constexpr V& operator/=(f32 S) noexcept {
        X /= S; Y /= S;
        return static_cast<V&>(*this);
    }
};

template <typename V>
struct VecBase<V, 3> {
    f32 X, Y, Z;

    constexpr VecBase() noexcept : X(), Y(), Z() {}
    constexpr explicit VecBase(f32 Vv) noexcept : X(Vv), Y(Vv), Z(Vv) {}
    constexpr VecBase(f32 X_, f32 Y_, f32 Z_) noexcept : X(X_), Y(Y_), Z(Z_) {}

    constexpr bool operator==(const V& O) const noexcept {
        return X == O.X && Y == O.Y && Z == O.Z;
    }

    constexpr bool operator!=(const V& O) const noexcept {
        return !(*this == O);
    }

    constexpr V& operator+=(const V& O) noexcept {
        X += O.X; Y += O.Y; Z += O.Z;
        return static_cast<V&>(*this);
    }

    constexpr V& operator-=(const V& O) noexcept {
        X -= O.X; Y -= O.Y; Z -= O.Z;
        return static_cast<V&>(*this);
    }

    constexpr V& operator*=(f32 S) noexcept {
        X *= S; Y *= S; Z *= S;
        return static_cast<V&>(*this);
    }

    constexpr V& operator/=(f32 S) noexcept {
        X /= S; Y /= S; Z /= S;
        return static_cast<V&>(*this);
    }
};

struct V2 : VecBase<V2, 2> {
    using VecBase::VecBase;
};

struct V3 : VecBase<V3, 3> {
    using VecBase::VecBase;
};

template <typename V>
constexpr V operator+(V A, const V& B) noexcept {
    return A += B;
}

template <typename V>
constexpr V operator-(V A, const V& B) noexcept {
    return A -= B;
}

template <typename V>
constexpr V operator*(V Vv, f32 S) noexcept {
    return Vv *= S;
}

template <typename V>
constexpr V operator*(f32 S, V Vv) noexcept {
    return Vv *= S;
}

template <typename V>
constexpr V operator/(V Vv, f32 S) noexcept {
    return Vv /= S;
}

template <typename V>
constexpr V operator-(const V& Vv) noexcept {
    V R = Vv;
    R *= -1.0f;
    return R;
}

template <typename V>
constexpr V operator+(const V& Vv) noexcept {
    return Vv;
}

constexpr f32 Dot(const V2& A, const V2& B) noexcept {
    return A.X * B.X + A.Y * B.Y;
}

constexpr f32 Dot(const V3& A, const V3& B) noexcept {
    return A.X * B.X + A.Y * B.Y + A.Z * B.Z;
}

constexpr f32 Cross(const V2& A, const V2& B) noexcept {
    return A.X * B.Y - A.Y * B.X;
}

constexpr V3 Cross(const V3& A, const V3& B) noexcept {
    return {
        A.Y * B.Z - A.Z * B.Y,
        A.Z * B.X - A.X * B.Z,
        A.X * B.Y - A.Y * B.X
    };
}

constexpr f32 LengthSq(const V2& Vv) noexcept {
    return Dot(Vv, Vv);
}

constexpr f32 LengthSq(const V3& Vv) noexcept {
    return Dot(Vv, Vv);
}

inline f32 Length(const V2& Vv) noexcept {
    return SqrtF(LengthSq(Vv));
}

inline f32 Length(const V3& Vv) noexcept {
    return SqrtF(LengthSq(Vv));
}

inline V2 Normalize(const V2& Vv) noexcept {
    const f32 LenSq = LengthSq(Vv);
    if (LenSq == 0.0f) return {};
    return Vv / SqrtF(LenSq);
}

inline V3 Normalize(const V3& Vv) noexcept {
    const f32 LenSq = LengthSq(Vv);
    if (LenSq == 0.0f) return {};
    return Vv / SqrtF(LenSq);
}

constexpr V2 NormalizeConstexpr(const V2& Vv) noexcept {
    const f32 LenSq = LengthSq(Vv);
    if (LenSq == 0.0f) return {};
    return Vv * ConstexprRsqrt(LenSq);
}

constexpr V3 NormalizeConstexpr(const V3& Vv) noexcept {
    const f32 LenSq = LengthSq(Vv);
    if (LenSq == 0.0f) return {};
    return Vv * ConstexprRsqrt(LenSq);
}

constexpr V2 Xy(const V2& Vv) noexcept { return Vv; }
constexpr V2 Yx(const V2& Vv) noexcept { return { Vv.Y, Vv.X }; }

constexpr V2 Xy(const V3& Vv) noexcept { return { Vv.X, Vv.Y }; }
constexpr V2 Xz(const V3& Vv) noexcept { return { Vv.X, Vv.Z }; }
constexpr V2 Yz(const V3& Vv) noexcept { return { Vv.Y, Vv.Z }; }

constexpr V2 Yx(const V3& Vv) noexcept { return { Vv.Y, Vv.X }; }
constexpr V2 Zx(const V3& Vv) noexcept { return { Vv.Z, Vv.X }; }
constexpr V2 Zy(const V3& Vv) noexcept { return { Vv.Z, Vv.Y }; }

constexpr V3 Xyz(const V3& Vv) noexcept { return Vv; }
constexpr V3 Xzy(const V3& Vv) noexcept { return { Vv.X, Vv.Z, Vv.Y }; }
constexpr V3 Yxz(const V3& Vv) noexcept { return { Vv.Y, Vv.X, Vv.Z }; }
constexpr V3 Yzx(const V3& Vv) noexcept { return { Vv.Y, Vv.Z, Vv.X }; }
constexpr V3 Zxy(const V3& Vv) noexcept { return { Vv.Z, Vv.X, Vv.Y }; }
constexpr V3 Zyx(const V3& Vv) noexcept { return { Vv.Z, Vv.Y, Vv.X }; }
}//math namespace
}//iron namespace
