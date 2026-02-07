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

typedef u32 type_id;

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

struct input_type {
    enum type : u32 {
        press = 0,
        release,
        hold
    };
};

struct key {
    enum code : u32 {
        key_a,
        key_b,
        key_c,
        key_d,
        key_e,
        key_f,
        key_g,
        key_h,
        key_i,
        key_j,
        key_k,
        key_l,
        key_m,
        key_n,
        key_o,
        key_p,
        key_q,
        key_r,
        key_s,
        key_t,
        key_u,
        key_v,
        key_w,
        key_x,
        key_y,
        key_z,
        key_1,
        key_2,
        key_3,
        key_4,
        key_5,
        key_6,
        key_7,
        key_8,
        key_9,
        key_0,
        key_f1,
        key_f2,
        key_f3,
        key_f4,
        key_f5,
        key_f6,
        key_f7,
        key_f8,
        key_f9,
        key_f10,
        key_f11,
        key_f12,
        key_shift,
        key_alt,
        key_control,
        key_enter,
        key_space,
        key_num_1,
        key_num_2,
        key_num_3,
        key_num_4,
        key_num_5,
        key_num_6,
        key_num_7,
        key_num_8,
        key_num_9,
        key_num_0,
        key_caps,
        key_home,
        key_page_up,
        key_page_down,
        key_delete,
        key_escape,
    };
};

struct modifier_key {
    enum code : u32 {
        shift = key::key_shift,
        alt = key::key_alt,
        control = key::key_control,
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
            m_free_head = read_next(index);
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

namespace math {
CORE_API f32 sqrt_f(f32 value);

constexpr f32 constexpr_rsqrt(f32 x) noexcept {
    if (x <= 0.0f) return 0.0f;

    f32 y{ 1.0f };
    for (u32 i{ 0 }; i < 6; ++i) {
        y = y * (1.5f - 0.5f * x * y * y);
    }
    return y;
}

template <typename V, int N>
struct vec_base;

template <typename V>
struct vec_base<V, 2> {
    f32 x, y;

    constexpr vec_base() noexcept : x(), y() {}
    constexpr explicit vec_base(f32 v) noexcept : x(v), y(v) {}
    constexpr vec_base(f32 x_, f32 y_) noexcept : x(x_), y(y_) {}

    constexpr bool operator==(const V& o) const noexcept {
        return x == o.x && y == o.y;
    }

    constexpr bool operator!=(const V& o) const noexcept {
        return !(*this == o);
    }

    constexpr V& operator+=(const V& o) noexcept {
        x += o.x; y += o.y;
        return static_cast<V&>(*this);
    }

    constexpr V& operator-=(const V& o) noexcept {
        x -= o.x; y -= o.y;
        return static_cast<V&>(*this);
    }

    constexpr V& operator*=(f32 s) noexcept {
        x *= s; y *= s;
        return static_cast<V&>(*this);
    }

    constexpr V& operator/=(f32 s) noexcept {
        x /= s; y /= s;
        return static_cast<V&>(*this);
    }
};

template <typename V>
struct vec_base<V, 3> {
    f32 x, y, z;

    constexpr vec_base() noexcept : x(), y(), z() {}
    constexpr explicit vec_base(f32 v) noexcept : x(v), y(v), z(v) {}
    constexpr vec_base(f32 x_, f32 y_, f32 z_) noexcept : x(x_), y(y_), z(z_) {}

    constexpr bool operator==(const V& o) const noexcept {
        return x == o.x && y == o.y && z == o.z;
    }

    constexpr bool operator!=(const V& o) const noexcept {
        return !(*this == o);
    }

    constexpr V& operator+=(const V& o) noexcept {
        x += o.x; y += o.y; z += o.z;
        return static_cast<V&>(*this);
    }

    constexpr V& operator-=(const V& o) noexcept {
        x -= o.x; y -= o.y; z -= o.z;
        return static_cast<V&>(*this);
    }

    constexpr V& operator*=(f32 s) noexcept {
        x *= s; y *= s; z *= s;
        return static_cast<V&>(*this);
    }

    constexpr V& operator/=(f32 s) noexcept {
        x /= s; y /= s; z /= s;
        return static_cast<V&>(*this);
    }
};

struct v2 : vec_base<v2, 2> {
    using vec_base::vec_base;
};

struct v3 : vec_base<v3, 3> {
    using vec_base::vec_base;
};

template <typename V>
constexpr V operator+(V a, const V& b) noexcept {
    return a += b;
}

template <typename V>
constexpr V operator-(V a, const V& b) noexcept {
    return a -= b;
}

template <typename V>
constexpr V operator*(V v, f32 s) noexcept {
    return v *= s;
}

template <typename V>
constexpr V operator*(f32 s, V v) noexcept {
    return v *= s;
}

template <typename V>
constexpr V operator/(V v, f32 s) noexcept {
    return v /= s;
}

template <typename V>
constexpr V operator-(const V& v) noexcept {
    V r = v;
    r *= -1.0f;
    return r;
}

template <typename V>
constexpr V operator+(const V& v) noexcept {
    return v;
}

constexpr f32 dot(const v2& a, const v2& b) noexcept {
    return a.x * b.x + a.y * b.y;
}

constexpr f32 dot(const v3& a, const v3& b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

constexpr f32 cross(const v2& a, const v2& b) noexcept {
    return a.x * b.y - a.y * b.x;
}

constexpr v3 cross(const v3& a, const v3& b) noexcept {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

constexpr f32 length_sq(const v2& v) noexcept {
    return dot(v, v);
}

constexpr f32 length_sq(const v3& v) noexcept {
    return dot(v, v);
}

inline f32 length(const v2& v) noexcept {
    return sqrt_f(length_sq(v));
}

inline f32 length(const v3& v) noexcept {
    return sqrt_f(length_sq(v));
}

inline v2 normalize(const v2& v) noexcept {
    const f32 len_sq = length_sq(v);
    if (len_sq == 0.0f) return {};

    return v / sqrt_f(len_sq);
}

inline v3 normalize(const v3& v) noexcept {
    const f32 len_sq = length_sq(v);
    if (len_sq == 0.0f) return {};

    return v / sqrt_f(len_sq);
}

constexpr v2 normalize_constexpr(const v2& v) noexcept {
    const f32 len_sq = length_sq(v);
    if (len_sq == 0.0f) return {};

    return v * constexpr_rsqrt(len_sq);
}

constexpr v3 normalize_constexpr(const v3& v) noexcept {
    const f32 len_sq = length_sq(v);
    if (len_sq == 0.0f) return {};

    return v * constexpr_rsqrt(len_sq);
}

constexpr v2 xy(const v2& v) noexcept { return v; }
constexpr v2 yx(const v2& v) noexcept { return { v.y, v.x }; }

constexpr v2 xy(const v3& v) noexcept { return { v.x, v.y }; }
constexpr v2 xz(const v3& v) noexcept { return { v.x, v.z }; }
constexpr v2 yz(const v3& v) noexcept { return { v.y, v.z }; }

constexpr v2 yx(const v3& v) noexcept { return { v.y, v.x }; }
constexpr v2 zx(const v3& v) noexcept { return { v.z, v.x }; }
constexpr v2 zy(const v3& v) noexcept { return { v.z, v.y }; }

constexpr v3 xyz(const v3& v) noexcept { return v; }
constexpr v3 xzy(const v3& v) noexcept { return { v.x, v.z, v.y }; }
constexpr v3 yxz(const v3& v) noexcept { return { v.y, v.x, v.z }; }
constexpr v3 yzx(const v3& v) noexcept { return { v.y, v.z, v.x }; }
constexpr v3 zxy(const v3& v) noexcept { return { v.z, v.x, v.y }; }
constexpr v3 zyx(const v3& v) noexcept { return { v.z, v.y, v.x }; }
}//math namespace
}//iron namespace
