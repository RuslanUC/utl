#pragma once

#include <decoder.h>
#include <def_pool.h>
#include <message_def.h>

bool utl_RoMessage_get_positions(utl_MessageDef* def, utl_DefPool* def_pool, utl_DecodeBuf* buffer, int32_t* positions);
bool utl_RoVector_get_positions(utl_MessageDefVector* def, utl_DefPool* def_pool, utl_DecodeBuf* buffer, int32_t elements_count, int32_t* positions);
