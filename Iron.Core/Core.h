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

struct option {
    enum value : u8 {
        disable = 0,
        enable = 1,
    };

    constexpr static inline bool get(value v) {
        return v == enable;
    }
};

struct version {
    u32         major : 11;
    u32         minor : 11;
    u32         patch : 10;

    version() = default;
    constexpr version(u32 vmajor, u32 vminor, u32 vpatch)
        : major(vmajor), minor(vminor), patch(vpatch) {
    }

    constexpr static inline u32 pack(const version& v) {
        return v.major << 21 | v.minor << 10 | v.patch;
    }

    constexpr static inline version unpack(u32 v) {
        version r{};
        r.major = (v >> 21) & 0x7ff;
        r.minor = (v >> 10) & 0x7ff;
        r.patch = v & 0x3ff;
        return r;
    }
};

#ifdef _DEBUG
#define LOG_DEBUG(x, ...) ::iron::log(::iron::log_level::debug, __FILE__, __LINE__, x, __VA_ARGS__)
#else
#define LOG_DEBUG(x, ...)
#endif
#define LOG_INFO(x, ...) ::iron::log(::iron::log_level::info, __FILE__, __LINE__, x, __VA_ARGS__)
#define LOG_WARNING(x, ...) ::iron::log(::iron::log_level::warning, __FILE__, __LINE__, x, __VA_ARGS__)
#define LOG_ERROR(x, ...) ::iron::log(::iron::log_level::error, __FILE__, __LINE__, x, __VA_ARGS__)
#define LOG_FATAL(x, ...) ::iron::log(::iron::log_level::fatal, __FILE__, __LINE__, x, __VA_ARGS__)
#define LOG_RESULT(x) ::iron::log_error(x, __FILE__, __LINE__)

#ifndef INVALID_TABLE
#define INVALID_TABLE(func) LOG_ERROR("Vtable function %s() was called, but not implemented!", #func);
#define CHECK_TABLE(func) if(!(m_table && m_table->func)) {                                                                   \
LOG_WARNING("Failed to get vtable function %s(), this is fine as long as the user does not call this function",   \
#func); }
#endif

#ifndef UNLIKELY
#define UNLIKELY [[unlikely]]
#endif

#ifndef MAKE_VERSION
#define MAKE_VERSION(mod, major, minor, patch) constexpr static inline version mod##_header_version(major, minor, patch);
#endif

#define IRON_MAX_PATH 260

namespace iron {
struct result {
    enum code : u32 {
        ok = 0,

        e_nomemory,
        e_nullptr,
        e_invalidarg,
        e_incomplete,
        e_createwindow,
        e_loadlibrary,
        e_getvtable,
        
        count,
    };

    constexpr static inline bool success(code c) {
        return c == ok;
    }

    constexpr static inline bool fail(code c) {
        return c != ok;
    }
};

struct log_level {
    enum level : u32 {
        debug = 0,
        info,
        warning,
        error,
        fatal,
        count,
    };
};

constexpr static inline size_t
str_len(const char* str) {
    size_t len{ 0 };
    while (*str++ != '\0') {
        ++len;
    }

    return len;
}

template<typename T>
constexpr static inline T
min_t(T x, T y) {
    return x < y ? x : y;
}

template<typename T>
constexpr static inline T
max_t(T x, T y) {
    return x > y ? x : y;
}

CORE_API void* mem_alloc(size_t size);
CORE_API void mem_free(void* block);
CORE_API void mem_set(void* dst, u8 value, size_t size);
CORE_API void mem_copy(void* dst, const void* src, size_t size);
CORE_API void mem_copy_s(void* dst, const void* src, size_t dst_size, size_t src_size);

CORE_API void log(log_level::level level, const char* file, int line, const char* msg, ...);
CORE_API void enable_log_level(log_level::level level, option::value enable);
CORE_API void enable_log_include_path(option::value enable);
CORE_API void log_error(result::code code, const char* file, int line);

template<typename T>
class vector {
public:
    using value_type = T;

    vector()
        : m_data(nullptr), m_size(0), m_capacity(0) {
    }

    vector(const vector& other)
        : m_data(nullptr), m_size(0), m_capacity(0) {
        reserve(other.m_size);
        m_size = other.m_size;
        mem_copy(m_data, other.m_data, m_size * sizeof(T));
    }

    vector& operator=(const vector& other) {
        if (this == &other)
            return *this;

        clear();
        reserve(other.m_size);
        m_size = other.m_size;
        mem_copy(m_data, other.m_data, m_size * sizeof(T));
        return *this;
    }

    ~vector() {
        if (m_data) {
            mem_free(m_data);
        }
    }

    inline u32 size() const {
        return m_size;
    }

    inline u32 capacity() const {
        return m_capacity;
    }

    inline bool empty() const {
        return m_size == 0;
    }

    void reserve(u32 new_capacity) {
        if (new_capacity <= m_capacity)
            return;

        T* new_data = (T*)mem_alloc(new_capacity * sizeof(T));
        if (!new_data)
            return;

        if (m_data) {
            mem_copy(new_data, m_data, m_size * sizeof(T));
            mem_free(m_data);
        }

        m_data = new_data;
        m_capacity = new_capacity;
    }

    void resize(u32 new_size) {
        if (new_size > m_capacity) {
            reserve(grow_capacity(new_size));
        }
        m_size = new_size;
    }

    void clear() {
        m_size = 0;
    }

    void push_back(const T& value) {
        if (m_size >= m_capacity) {
            reserve(grow_capacity(m_size + 1));
        }
        m_data[m_size++] = value;
    }

    void pop_back() {
        if (m_size > 0) {
            --m_size;
        }
    }

    void erase(u32 index) {
        if (index >= m_size)
            return;

        for (u32 i = index; i < m_size - 1; ++i) {
            m_data[i] = m_data[i + 1];
        }
        --m_size;
    }

    T& operator[](u32 index) {
        return m_data[index];
    }

    const T& operator[](u32 index) const {
        return m_data[index];
    }

    T* data() {
        return m_data;
    }

    const T* data() const {
        return m_data;
    }

    T* begin() {
        return m_data;
    }

    const T* begin() const {
        return m_data;
    }

    T* end() {
        return &m_data[m_size];
    }

    const T* end() const {
        return &m_data[m_size];
    }

private:
    T*  m_data;
    u32 m_size;
    u32 m_capacity;

    static u32 grow_capacity(u32 min_capacity) {
        u32 cap = min_capacity > 1 ? min_capacity : 1;
        cap |= cap >> 1;
        cap |= cap >> 2;
        cap |= cap >> 4;
        cap |= cap >> 8;
        cap |= cap >> 16;
        return cap + 1;
    }
};

template<typename T>
class free_list {
public:
    static_assert(sizeof(T) >= sizeof(size_t),
        "free_list<T> requires sizeof(T) >= sizeof(size_t)");

    free_list()
        : m_free_head(npos) {
    }

    size_t allocate() {
        if (m_free_head != npos) {
            const size_t index{ m_free_head };
            m_free_head{ read_next(index) };
            return index;
        }

        const size_t index{ m_data.size() };
        m_data.resize((u32)(index + 1));
        return index;
    }

    void free(size_t index) {
        write_next(index, m_free_head);
        m_free_head = index;
    }

    T& operator[](size_t index) {
        return m_data[(u32)index];
    }

    const T& operator[](size_t index) const {
        return m_data[(u32)index];
    }

    T* data() {
        return m_data.data();
    }

    const T* data() const {
        return m_data.data();
    }

    size_t size() const {
        return m_data.size();
    }

    void clear() {
        m_data.clear();
        m_free_head = npos;
    }

    bool valid(size_t index) const {
        return index < m_data.size();
    }

    static constexpr size_t npos = (size_t)-1;

private:
    vector<T> m_data;
    size_t    m_free_head;

    inline size_t read_next(size_t index) const {
        return *(const size_t*)(&m_data[(u32)index]);
    }

    inline void write_next(size_t index, size_t next) {
        *(size_t*)(&m_data[(u32)index]) = next;
    }
};
}
