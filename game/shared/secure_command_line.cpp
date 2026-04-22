//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"

#include "fmtstr.h"
#include "secure_command_line.h"

CSecureLaunchSystem::CSecureLaunchSystem( FnUnsafeCmdLineProcessor *pfnUnsafeCmdLineProcessor )
: CAutoGameSystem( "secure_launch_system" )
, m_pfnUnsafeCmdLineProcessor( pfnUnsafeCmdLineProcessor )
{ }


CSecureLaunchSystem::~CSecureLaunchSystem()
{ 
	m_pfnUnsafeCmdLineProcessor = nullptr;
}


void CSecureLaunchSystem::OnNewUrlLaunchParameters( NewUrlLaunchParameters_t * )
{
	int nBytes = steamapicontext->SteamApps()->GetLaunchCommandLine( nullptr, 0 );
	CUtlString strBuffer;
	strBuffer.SetLength( nBytes );

	int nBytesWritten = steamapicontext->SteamApps()->GetLaunchCommandLine( strBuffer.GetForModify(), nBytes );
	Assert( nBytes == nBytesWritten );
	
	CSteamID steamIDUnknown( ~0u, steamapicontext->SteamUtils()->GetConnectedUniverse(), k_EAccountTypeIndividual );
	( *m_pfnUnsafeCmdLineProcessor )( strBuffer.Get(), nBytesWritten, steamIDUnknown );
}


void CSecureLaunchSystem::OnGameRichPresenceJoinRequested( GameRichPresenceJoinRequested_t *pCallback )
{
	if ( !pCallback || !pCallback->m_rgchConnect[0] )
		return;

	char const *szConCommand = pCallback->m_rgchConnect;

	//
	// Work around Steam Overlay bug that it doesn't replace %20 characters
	//
	CFmtStr fmtCommand;
	if ( strstr( szConCommand, "%20" ) )
	{
		fmtCommand.AppendFormat( "%s", szConCommand );
		while ( char *pszReplace = strstr( fmtCommand.Access(), "%20" ) )
		{
			*pszReplace = ' ';
			Q_memmove( pszReplace + 1, pszReplace + 3, Q_strlen( pszReplace + 3 ) + 1 );
		}
		szConCommand = fmtCommand.Access();
	}

	int nBufferSize = V_strlen( szConCommand ) + 1;

	( *m_pfnUnsafeCmdLineProcessor )( szConCommand, nBufferSize, pCallback->m_steamIDFriend );
}

void CSecureLaunchSystem::ProcessCommandLine()
{
	OnNewUrlLaunchParameters( nullptr );
}


CSecureLaunchSystem &SecureLaunchSystem( FnUnsafeCmdLineProcessor *pfnUnsafeCmdLineProcessor = nullptr )
{
	static bool s_bInited = ( pfnUnsafeCmdLineProcessor != nullptr );
	if ( !s_bInited )
	{
		DebuggerBreakIfDebugging();
		// TODO: Do better than this. But OSX doesn't have Plat_ExitProcessWithError.
		Plat_ExitProcess( 3 );
	}


	// Our singleton. 
	static CSecureLaunchSystem s_SecureLaunchSystem( pfnUnsafeCmdLineProcessor );
	return s_SecureLaunchSystem;
}

void RegisterSecureLaunchProcessFunc( FnUnsafeCmdLineProcessor *pfnUnsafeCmdLineProcessor )
{
	SecureLaunchSystem( pfnUnsafeCmdLineProcessor );
}

void ProcessInsecureLaunchParameters()
{
	SecureLaunchSystem().ProcessCommandLine();
}

static bool BHelperCheckSafeUserCmdString( char const *ipconnect )
{
	bool bConnectValid = true;
	while ( *ipconnect )
	{
		if ( ( ( *ipconnect >= '0' ) && ( *ipconnect <= '9' ) ) ||
			( ( *ipconnect >= 'a' ) && ( *ipconnect <= 'z' ) ) ||
			( ( *ipconnect >= 'A' ) && ( *ipconnect <= 'Z' ) ) ||
			( *ipconnect == '_' ) || ( *ipconnect == '-' ) || ( *ipconnect == '.' ) ||
			( *ipconnect == ':' ) || ( *ipconnect == '?' ) || ( *ipconnect == '%' ) ||
			( *ipconnect == '/' ) || ( *ipconnect == '=' ) || ( *ipconnect == ' ' ) ||
			( *ipconnect == '[' ) || ( *ipconnect == ']' ) || ( *ipconnect == '@' ) ||
			( *ipconnect == '"' ) || ( *ipconnect == '\'' ) || ( *ipconnect == '#' ) ||
			( *ipconnect == '(' ) || ( *ipconnect == ')' ) || ( *ipconnect == '!' ) ||
			( *ipconnect == '\\' ) || ( *ipconnect == '$' )
			)
			++ipconnect;
		else
		{
			bConnectValid = false;
			break;
		}
	}
	return bConnectValid;
}

void UnsafeCmdLineProcessor( const char *pchUnsafeCmdLine, int cubSize, CSteamID srcSteamID )
{
	if ( cubSize <= 1 )
	{
		DevMsg( "Received empty command line from steam\n" );
		return;
	}

	DevMsg( "Received command line request[%d]: '%s'\n", cubSize, pchUnsafeCmdLine );

	if ( pchUnsafeCmdLine[ 0 ] != '+' )
		return;

	char const *szConCommand = pchUnsafeCmdLine + 1;

	if ( char const *ipconnect = StringAfterPrefix( szConCommand, "connect " ) )
	{
		if ( BHelperCheckSafeUserCmdString( ipconnect ) )
		{
			engine->ClientCmd_Unrestricted( szConCommand );
		}
	}
}