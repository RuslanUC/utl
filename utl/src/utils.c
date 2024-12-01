#include "utils.h"

bool is_big_endian() {
    int32_t i = 1;
    return *(char*)&i != 1;
}
