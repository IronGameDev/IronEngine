#include <Iron.Core/Core.h>

#include <memory>

namespace Iron {
void*
MemAlloc(size_t Size) {
    return malloc(Size);
}

void
MemFree(void* Block) {
    free(Block);
}

void
MemSet(void* Dst, u8 Value, size_t Size) {
    memset(Dst, (int)Value, Size);
}

void
MemCopy(void* Dst, const void* Src, size_t Size) {
    memcpy(Dst, Src, Size);
}

void
MemCopyS(void* Dst, const void* Src, size_t DstSize, size_t SrcSize) {
    memcpy_s(Dst, DstSize, Src, SrcSize);
}
}
