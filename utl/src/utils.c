#include "utils.h"

#ifdef UTL_DONT_USE_COMPILE_TIME_ENDIANNESS_CHECK
bool is_big_endian() {
    int32_t i = 1;
    return *(char*)&i != 1;
}
#endif
