#pragma once

#include <inttypes.h>
#include <stdbool.h>
#include "arena.h"

bool is_big_endian();
int utl_arena_delete(struct arena *a);