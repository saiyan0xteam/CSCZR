//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Contains an interface to log some events to the Steam Share 
//			timeline via ISteamTimeline 
//
//===========================================================================//

#include "cbase.h"
#include "shareddefs.h"
#include "view.h"
#include "steamshare.h"
#include <vgui/ILocalize.h>
#include <tier1/fmtstr.h>
#include "basecombatweapon_shared.h"
#include "ammodef.h"
#include "c_ai_basenpc.h"

#if !defined( _X360 ) && !defined( NO_STEAM )
#include "steam/isteamtimeline.h"
#include "steam/steam_api.h"
#endif

#define TIMELINE_NO_PRIORITY	50
#define TIMELINE_LOW_PRIORITY	75
#define TIMELINE_MED_PRIORITY	100
#define TIMELINE_HIGH_PRIORITY	125

bool ShouldHaveLocalPlayerPickupTimelineEvents();

extern vgui::ILocalize *g_pVGuiLocalize;

static const char *GetLocalizedTitleString( const char *pszToken, const char *pszArg1 = NULL )
{
	wchar_t wszLocalizedArg1[256];

	const wchar_t *pwszLocalizedArg1 = nullptr;
	if ( pszArg1 )
	{
		pwszLocalizedArg1 = g_pVGuiLocalize->Find( pszArg1 );

		if ( !pwszLocalizedArg1 )
		{
			g_pVGuiLocalize->ConvertANSIToUnicode( pszArg1, wszLocalizedArg1, sizeof( wszLocalizedArg1 ) );
			pwszLocalizedArg1 = wszLocalizedArg1;
		}
	}

	wchar_t wszLocalizedString[ 256 ];
	const wchar_t *pwszLocalizedString = nullptr;
	if ( pwszLocalizedArg1 )
	{	
		const wchar_t *pwszLocalizedToken = nullptr;
		pwszLocalizedToken = g_pVGuiLocalize->Find( pszToken );
		if ( !pwszLocalizedToken )
			return pszToken;

		g_pVGuiLocalize->ConstructString( wszLocalizedString, sizeof( wszLocalizedString ), pwszLocalizedToken, 1, pwszLocalizedArg1 );
		pwszLocalizedString = wszLocalizedString;
	}
	else
	{
		pwszLocalizedString = g_pVGuiLocalize->Find( pszToken );
		if ( !pwszLocalizedString )
			return pszToken;
	}

	static char szTitleString[ 256 ];
	g_pVGuiLocalize->ConvertUnicodeToANSI( pwszLocalizedString, szTitleString, sizeof( szTitleString ) );
	return szTitleString;
}

//-----------------------------------------------------------------------------
// Singleton
//-----------------------------------------------------------------------------
static CSteamShareSystem g_SteamShareSystem;

IGameSystem *SteamShareSystem()
{
	return &g_SteamShareSystem;
}

#define STEAMSHARE_MULTIPLAYER 1

bool CSteamShareSystem::Init()
{
	
#if !defined( NO_STEAM )
	// if we don't have the version of ISteamTimeline we need, don't bother listening for any events
	// and just stub out this whole system.
	if ( !steamapicontext || !SteamTimeline() )
		return true;

	ListenForGameEvent( "gameui_activate" );
	ListenForGameEvent( "gameui_hide" );
	ListenForGameEvent( "server_spawn" );
	ListenForGameEvent( "player_death" );
	ListenForGameEvent( "weapon_equipped" );
	SteamTimeline()->SetTimelineGameMode( k_ETimelineGameMode_Menus );
#endif // !NO_STEAM

	return true;
}

void CSteamShareSystem::FireGameEvent( IGameEvent *event )
{
#if !defined( NO_STEAM )
	if ( !V_stricmp( event->GetName(), "gameui_activate" ) )
	{
		SteamTimeline()->SetTimelineGameMode( k_ETimelineGameMode_Menus );
	}
	else if ( !V_stricmp( event->GetName(), "gameui_hide" ) )
	{
		SteamTimeline()->SetTimelineGameMode( k_ETimelineGameMode_Playing );
	}
	else if ( !V_stricmp( event->GetName(), "server_spawn" ) )
	{
#if defined( STEAMSHARE_MULTIPLAYER )
		const char *pszMapName = event->GetString( "mapname" );
		if ( pszMapName )
		{
			SteamTimeline()->SetTimelineStateDescription( pszMapName, 0 );
		}
#else // defined( STEAMSHARE_MULTIPLAYER )
		char buf[ 1024 ];
		engine->GetChapterName( buf, sizeof( buf ) );
		TrimRight( buf ); // why are chapter names left padded?
		const char *pszText = nullptr;
		if ( !V_isempty( buf ) )
		{
			pszText = g_pVGuiLocalize->FindAsUTF8( buf );
		}
		else
		{
			pszText = event->GetString( "mapname" );
		}

		if ( !V_isempty( pszText ) )
		{
			char buf2[ 1024 ];
			if ( pszText[ 0 ] == '"' )
			{
				// If the map name is in quotes like "A RED LETTER DAY", lose the 
				// quotes before we had this off to Steam.
				int len = V_strlen( pszText );
				V_strncpy( buf2, pszText + 1, len - 2 );
			}
			else
			{
				V_strcpy_safe( buf2, pszText );
			}

			// TODO(joe): This is a terrible idea and will mess up UTF-8 text something fierce. 
			// But we only care about English for our Steam Share testing, and the
			// all uppercase localized names look terrible there, so...
			V_strtitlecase( buf2 );

			SteamTimeline()->SetTimelineStateDescription( buf2, 0 );
		}
#endif // !TF_CLIENT_DLL
	}

	else if ( !V_stricmp( event->GetName(), "player_death" ) )
	{
		int nLocalPlayer = GetLocalPlayerIndex();
		int nAttacker = engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
		int nAssister = engine->GetPlayerForUserID( event->GetInt( "assister" ) );
		int nVictim = engine->GetPlayerForUserID( event->GetInt( "userid" ) );

		if ( ( nAttacker == nLocalPlayer || nAssister == nLocalPlayer ) && ( nVictim != nLocalPlayer ) )
		{
			C_BasePlayer *pVictim = ToBasePlayer( ClientEntityList().GetEnt( nVictim ) );
			if ( pVictim )
			{
				const char *pszTitle = GetLocalizedTitleString( "#Valve_Timeline_Killed", pVictim->GetPlayerName() );
				SteamTimeline()->AddTimelineEvent( "steam_attack", pszTitle, "", TIMELINE_LOW_PRIORITY, 0.f, 0.f, k_ETimelineEventClipPriority_Featured );
			}
		}
		else if ( nVictim == nLocalPlayer )
		{
			const char *pszTitle = "";
			C_BasePlayer *pAttacker = ToBasePlayer( ClientEntityList().GetEnt( nAttacker ) );
			if ( pAttacker )
			{
				const char *pszTempTitle = "#Valve_Timeline_WereKilled";
				if ( nAttacker == nLocalPlayer )
				{
					pszTempTitle = "#Valve_Timeline_Suicide";
				}
				pszTitle = GetLocalizedTitleString( pszTempTitle, pAttacker->GetPlayerName() );
			}
			else if ( nAttacker == 0 ) // world
			{
				pszTitle = GetLocalizedTitleString( "#Valve_Timeline_Suicide" );
			}
			SteamTimeline()->AddTimelineEvent( "steam_death", pszTitle, "", TIMELINE_MED_PRIORITY, 0.f, 0.f, k_ETimelineEventClipPriority_Featured );
		}
	}
	else if ( !V_stricmp( event->GetName(), "weapon_equipped" ) )
	{
		int nWeaponEntindex = event->GetInt( "entindex" );
		int nOwnerEntIndex = event->GetInt( "owner_entindex" );
		C_BaseCombatWeapon* pWeapon = dynamic_cast<C_BaseCombatWeapon *>( ClientEntityList().GetEnt( nWeaponEntindex ) );
		C_BasePlayer* pOwner = dynamic_cast<C_BasePlayer *>( ClientEntityList().GetEnt( nOwnerEntIndex ) );
		if ( pWeapon && pOwner && pOwner == C_BasePlayer::GetLocalPlayer() && ShouldHaveLocalPlayerPickupTimelineEvents() )
		{
			const char* pchWeaponName = "weapon";
			const char* pchIcon = "steam_combat";
			if ( pWeapon )
			{
				const char* pchActualName = g_pVGuiLocalize->FindAsUTF8( pWeapon->GetPrintName() );
				if ( pchActualName )
				{
					pchWeaponName = pchActualName;
				}

				const char* pszTitle = GetLocalizedTitleString( "#Valve_Timeline_Pickup", pchWeaponName );

				SteamTimeline()->AddTimelineEvent( pchIcon, pszTitle, nullptr, TIMELINE_NO_PRIORITY, 0, 0, k_ETimelineEventClipPriority_Standard );
			}
		}
	}
#endif // !NO_STEAM
}
