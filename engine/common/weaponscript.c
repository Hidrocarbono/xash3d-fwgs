/*
weaponscript.c - data-driven weapon/ammo script system (Xash Weapon System)
Copyright (C) 2026 Hidrocarbono

Parser for Uncle Mike's Paranoia 2 script format:
  ammodesc.txt -> ammoinfo { }  and  ammo_<name> { }
  weapon_*.txt -> WeaponData { } PrimaryAttack { } SecondaryAttack { }
                 SoundData { } hudsprite { }
Scripts are line-oriented key/value pairs inside { } blocks. This file
covers the ammo parser (Fase 2). The weapon parser is stubbed for the
next iteration but the structures are fully declared in weaponscript.h.
*/

#include "weaponscript.h"
#include "filesystem/filesystem.h"
#include "stringlib.h"

ammoinfo_t	gAmmoInfo[MAX_AMMO_TYPES];
int		gNumAmmoInfo = 0;
ammopickup_t	gAmmoPickups[MAX_AMMO_TYPES];
int		gNumAmmoPickups = 0;
weaponinfo_t	gWeaponInfo[MAX_AMMO_TYPES];
int		gNumWeaponInfo = 0;

ammoinfo_t *WeaponScript_FindAmmo( const char *name )
{
	int i;
	for( i = 0; i < gNumAmmoInfo; i++ )
	{
		if( !Q_stricmp( gAmmoInfo[i].name, name ) )
			return &gAmmoInfo[i];
	}
	return NULL;
}

// Read a whole file (engine VFS) into a NUL-terminated buffer. Mem_Free() it.
static char *WS_LoadText( const char *filename )
{
	fs_offset_t size;
	byte *buf = FS_LoadFile( filename, &size, false );
	char *text;

	if( !buf )
		return NULL;

	text = (char *)Mem_Malloc( host.mempool, size + 1 );
	memcpy( text, buf, size );
	text[size] = '\0';
	Mem_Free( buf );
	return text;
}

// Advance *pp past whitespace and comments; return next token (in place) or NULL.
static char *WS_NextToken( char **pp )
{
	char *p = *pp;

	while( *p )
	{
		while( *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' )
			p++;

		if( *p == '/' && p[1] == '*' )
		{
			p += 2;
			while( *p && !(*p == '*' && p[1] == '/') )
				p++;
			if( *p ) p += 2;
			continue;
		}
		if( *p == '/' && p[1] == '/' )
		{
			while( *p && *p != '\n' )
				p++;
			continue;
		}

		if( *p == '"' )
		{
			char *start = p + 1;
			char *dst = p;
			p++;
			while( *p && *p != '"' )
				*dst++ = *p++;
			if( *p == '"' ) p++;
			*dst = '\0';
			*pp = p;
			return start;
		}

		if( *p != '{' && *p != '}' )
		{
			char *start = p;
			while( *p && *p != ' ' && *p != '\t' && *p != '\r'
				&& *p != '\n' && *p != '{' && *p != '}' )
				p++;
			*pp = p;
			return start;
		}

		{ char *start = p; p++; *pp = p; return start; }
	}
	*pp = p;
	return NULL;
}

// Parse "a..b" into outMin/outMax (midpoint when no range).
static void WS_ParseRange( const char *s, float *outMin, float *outMax )
{
	float a, b;
	char *dot = Q_strchr( s, '.' );
	if( dot && dot[1] == '.' )
	{
		char buf[64];
		Q_strncpy( buf, s, sizeof( buf ) );
		buf[dot - s] = '\0';
		a = atof( buf );
		b = atof( dot + 2 );
	}
	else
	{
		a = b = atof( s );
	}
	*outMin = a;
	*outMax = b;
}

// Read a { } block of key/value pairs, calling apply() for each pair.
static qboolean WS_ParseKVBlock( char **pp, void *out,
	void (*apply)( void *out, const char *key, const char *val ) )
{
	char *t = WS_NextToken( pp );
	if( !t || t[0] != '{' )
		return false;

	while( true )
	{
		char *k = WS_NextToken( pp );
		if( !k ) return false;
		if( k[0] == '}' )
			break;
		char *v = WS_NextToken( pp );
		if( !v ) return false;
		apply( out, k, v );
	}
	return true;
}

static void WS_ApplyAmmoInfo( void *out, const char *key, const char *val )
{
	ammoinfo_t *a = (ammoinfo_t *)out;
	if( !Q_stricmp( key, "name" ) ) Q_strncpy( a->name, val, sizeof( a->name ) );
	else if( !Q_stricmp( key, "MaxCarry" ) ) a->MaxCarry = atoi( val );
	else if( !Q_stricmp( key, "PlayerDamage" ) ) a->PlayerDamage = atoi( val );
	else if( !Q_stricmp( key, "MonsterDamage" ) ) a->MonsterDamage = atoi( val );
	else if( !Q_stricmp( key, "Damage" ) ) a->Damage = atoi( val );
	else if( !Q_stricmp( key, "Distance" ) ) a->Distance = atof( val );
	else if( !Q_stricmp( key, "NumShots" ) ) a->NumShots = atoi( val );
	else if( !Q_stricmp( key, "ShellModel" ) ) Q_strncpy( a->ShellModel, val, sizeof( a->ShellModel ) );
	else if( !Q_stricmp( key, "Missile" ) ) Q_strncpy( a->Missile, val, sizeof( a->Missile ) );
	else if( !Q_stricmp( key, "count" ) ) a->count = atoi( val );
}

static void WS_ApplyAmmoPickup( void *out, const char *key, const char *val )
{
	ammopickup_t *p = (ammopickup_t *)out;
	if( !Q_stricmp( key, "model" ) ) Q_strncpy( p->model, val, sizeof( p->model ) );
	else if( !Q_stricmp( key, "sound" ) ) Q_strncpy( p->sound, val, sizeof( p->sound ) );
	else if( !Q_stricmp( key, "type" ) ) Q_strncpy( p->type, val, sizeof( p->type ) );
	else if( !Q_stricmp( key, "count" ) ) p->count = atoi( val );
}

int WeaponScript_ParseAmmoDesc( const char *filename )
{
	char *text = WS_LoadText( filename );
	char *p;
	int parsed = 0;

	if( !text )
	{
		Con_Printf( "WeaponScript: cannot open %s\n", filename );
		return -1;
	}

	p = text;
	while( true )
	{
		char *t = WS_NextToken( &p );
		if( !t )
			break;

		if( !Q_stricmp( t, "ammoinfo" ) )
		{
			if( gNumAmmoInfo >= MAX_AMMO_TYPES )
			{
				Con_Printf( "WeaponScript: ammo type limit reached\n" );
				break;
			}
			if( WS_ParseKVBlock( &p, &gAmmoInfo[gNumAmmoInfo], WS_ApplyAmmoInfo ) )
				gNumAmmoInfo++;
			parsed++;
		}
		else if( !Q_strnicmp( t, "ammo_", 5 ) )
		{
			ammopickup_t pk;
			memset( &pk, 0, sizeof( pk ) );
			Q_strncpy( pk.classname, t, sizeof( pk.classname ) );
			if( gNumAmmoPickups < MAX_AMMO_TYPES )
			{
				if( WS_ParseKVBlock( &p, &pk, WS_ApplyAmmoPickup ) )
					gAmmoPickups[gNumAmmoPickups++] = pk;
			}
			else
			{
				WS_ParseKVBlock( &p, &pk, WS_ApplyAmmoPickup ); // drain
			}
		}
	}

	Mem_Free( text );
	Con_Printf( "WeaponScript: parsed %d ammo definitions from %s\n", parsed, filename );
	return parsed;
}

int WeaponScript_ParseWeapon( const char *filename )
{
	// Fase 2 follow-up: full weapon_*.txt parser (WeaponData/PrimaryAttack/...).
	Con_Printf( "WeaponScript: weapon parser for %s pending (next iteration)\n", filename );
	return 0;
}

void WeaponScript_LoadAll( void )
{
	WeaponScript_ParseAmmoDesc( "ammodesc.txt" );
}
