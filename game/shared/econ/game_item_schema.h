//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================
#ifndef GAME_ITEM_SCHEMA_H
#define GAME_ITEM_SCHEMA_H
#ifdef _WIN32
#pragma once
#endif

class CEconItemSchema;
class CEconItemDefinition;
class CEconItemSystem;
typedef CEconItemSchema		GameItemSchema_t;
typedef CEconItemDefinition	GameItemDefinition_t;
typedef CEconItemSystem		GameItemSystem_t;

#include "econ_item_schema.h"

extern GameItemSchema_t *GetItemSchema();

#endif // GAME_ITEM_SYSTEM_H
