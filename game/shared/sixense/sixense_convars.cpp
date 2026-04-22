//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "convar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef SIXENSE

//
// general sixense convars
//

ConVar sixense_enabled( "sixense_enabled", "0", FCVAR_ARCHIVE );
ConVar sixense_filter_level( "sixense_filter_level", "0.5", FCVAR_ARCHIVE );
ConVar sixense_features_enabled( "sixense_features_enabled", "0", FCVAR_REPLICATED );
ConVar sixense_mode( "sixense_mode", "0", FCVAR_ARCHIVE );

#endif // SIXENSE
