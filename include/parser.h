#pragma once

#include "message_def.h"
#include "def_pool.h"

utl_MessageDef* utl_parse_line(utl_DefPool* def_pool, char* line, size_t size);
