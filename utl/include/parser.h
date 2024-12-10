#pragma once

#include "message_def.h"
#include "def_pool.h"
#include "status.h"

utl_MessageDef* utl_parse_line(utl_DefPool* def_pool, char* line, size_t size, utl_Status* status);
//void utl_parse_file(utl_DefPool* def_pool, const char* file_name);