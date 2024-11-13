#include "utils.h"

#include <knob.h>
#include <sys/mman.h>

bool is_big_endian() {
    int32_t i = 1;
    return (*((char *)&i) != 1);
}

int utl_arena_delete(struct arena* a) {
    if (!(a->flags & ARENA_GROW)) {
        return -1;
    }
    a->cap  = -1;
    a->size = -1;
    if (munmap(a->data, KNOB_MMAP_SIZE) == -1) {
        //arena_err("munmap");
        return -1;
    }
    return 0;
}
