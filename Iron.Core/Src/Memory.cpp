#include <Iron.Core/Core.h>

#include <memory>

namespace iron {
void*
mem_alloc(size_t size) {
    return malloc(size);
}

void
mem_free(void* block) {
    return free(block);
}

void
mem_set(void* dst, u8 value, size_t size) {
    memset(dst, (int)value, size);
}

void
mem_copy(void* dst, const void* src, size_t size) {
    memcpy(dst, src, size);
}

void
mem_copy_s(void* dst, const void* src, size_t dst_size, size_t src_size)
{
    memcpy_s(dst, dst_size, src, src_size);
}
}
