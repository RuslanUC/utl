#pragma once

#include <decoder.h>
#include <def_pool.h>
#include <message_def.h>
#include <stdio.h>

bool utl_RoMessage_get_positions(utl_MessageDef* def, utl_DefPool* def_pool, utl_DecodeBuf* buffer, ssize_t* positions);
bool utl_RoVector_get_positions(utl_MessageDefVector* def, utl_DefPool* def_pool, utl_DecodeBuf* buffer, size_t elements_count, ssize_t* positions);
