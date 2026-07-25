/*
weaponscript.c - data-driven weapon/ammo script system (Xash Weapon System)
Copyright (C) 2026 Hidrocarbono

Parser for Uncle Mike's Paranoia 2 script format:
  ammodesc.txt -> ammoinfo { }  and  ammo_<name> { }
  weapon_*.txt -> WeaponData { } PrimaryAttack { } SecondaryAttack { }
                 SoundData { } hudsprite { }
Scripts are line-oriented key/value pairs inside { } blocks.
*/

#include "common.h"
#include "crtlib.h"
#include "filesystem.h"
#include "weaponscript.h"

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

static int WS_FlagsFromString( const char *val )
{
	int f = 0;
	// format: "IronSight|AutoAim|AutoFire"
	char buf[128];
	Q_strncpy( buf, val, sizeof( buf ) );
	char *tok = strtok( buf, "|" );
	while( tok )
	{
		if( !Q_stricmp( tok, "IronSight" ) ) f |= WIF_IRONSIGHT;
		else if( !Q_stricmp( tok, "AutoAim" ) ) f |= WIF_AUTOAIM;
		else if( !Q_stricmp( tok, "AutoFire" ) ) f |= WIF_AUTOFIRE;
		tok = strtok( NULL, "|" );
	}
	return f;
}

static void WS_ApplyWeaponData( void *out, const char *key, const char *val )
{
	weaponinfo_t *w = (weaponinfo_t *)out;
	if( !Q_stricmp( key, "viewmodel" ) ) Q_strncpy( w->viewmodel, val, sizeof( w->viewmodel ) );
	else if( !Q_stricmp( key, "playermodel" ) ) Q_strncpy( w->playermodel, val, sizeof( w->playermodel ) );
	else if( !Q_stricmp( key, "worldmodel" ) ) Q_strncpy( w->worldmodel, val, sizeof( w->worldmodel ) );
	else if( !Q_stricmp( key, "anim_prefix" ) ) Q_strncpy( w->anim_prefix, val, sizeof( w->anim_prefix ) );
	else if( !Q_stricmp( key, "bucket" ) ) w->bucket = atoi( val );
	else if( !Q_stricmp( key, "bucket_position" ) ) w->bucket_position = atoi( val );
	else if( !Q_stricmp( key, "clip_size" ) ) w->clip_size = atoi( val );
	else if( !Q_stricmp( key, "defaultammo" ) ) w->defaultammo = atoi( val );
	else if( !Q_stricmp( key, "primary_ammo" ) ) Q_strncpy( w->primary_ammo, val, sizeof( w->primary_ammo ) );
	else if( !Q_stricmp( key, "secondary_ammo" ) ) Q_strncpy( w->secondary_ammo, val, sizeof( w->secondary_ammo ) );
	else if( !Q_stricmp( key, "weight" ) ) w->weight = atoi( val );
	else if( !Q_stricmp( key, "SpreadTime" ) ) w->SpreadTime = atof( val );
	else if( !Q_stricmp( key, "item_flags" ) ) w->item_flags = WS_FlagsFromString( val );
	else if( !Q_stricmp( key, "MaxSpeed" ) ) w->MaxSpeed = atof( val );
	else if( !Q_stricmp( key, "MaxSpeedIS" ) ) w->MaxSpeedIS = atof( val );
	else if( !Q_stricmp( key, "volume" ) ) Q_strncpy( w->volume, val, sizeof( w->volume ) );
	else if( !Q_stricmp( key, "flash" ) ) Q_strncpy( w->flash, val, sizeof( w->flash ) );
}

static void WS_ApplyAttack( void *out, const char *key, const char *val )
{
	weaponattack_t *at = (weaponattack_t *)out;
	float lo, hi;
	if( !Q_stricmp( key, "action" ) ) Q_strncpy( at->action, val, sizeof( at->action ) );
	else if( !Q_stricmp( key, "nextattack" ) ) at->nextattack = atof( val );
	else if( !Q_stricmp( key, "PunchAngle" ) )
	{
		// "a..b" "c..d" "e"
		float v[3] = { 0, 0, 0 };
		WS_ParseRange( val, &lo, &hi ); v[0] = (lo + hi) * 0.5f;
		char *t2 = WS_NextToken( &((char *){0}) ); // unused placeholder
	}
	else if( !Q_stricmp( key, "PunchAngleIS" ) )
	{
		WS_ParseRange( val, &lo, &hi ); at->PunchAngleIS[0] = (lo + hi) * 0.5f;
	}
	else if( !Q_stricmp( key, "SpreadRange" ) )
	{
		WS_ParseRange( val, &at->SpreadRange[0], &at->SpreadRange[1] );
	}
	else if( !Q_stricmp( key, "SpreadExpand" ) ) at->SpreadExpand = atof( val );
	else if( !Q_stricmp( key, "SpreadRangeIS" ) )
	{
		WS_ParseRange( val, &at->SpreadRangeIS[0], &at->SpreadRangeIS[1] );
	}
	else if( !Q_stricmp( key, "SpreadExpandIS" ) ) at->SpreadExpandIS = atof( val );
}

static void WS_ApplySound( void *out, const char *key, const char *val )
{
	weaponsound_t *s = (weaponsound_t *)out;
	if( !Q_stricmp( key, "shootsound1" ) )
	{
		if( !s->shootsound1[0] ) Q_strncpy( s->shootsound1, val, sizeof( s->shootsound1 ) );
		else Q_strncpy( s->shootsound2, val, sizeof( s->shootsound2 ) );
	}
	else if( !Q_stricmp( key, "emptysound" ) ) Q_strncpy( s->emptysound, val, sizeof( s->emptysound ) );
}

static void WS_ApplySprite( void *out, const char *key, const char *val )
{
	weaponsprite_t *sp = (weaponsprite_t *)out;
	if( !Q_stricmp( key, "name" ) ) Q_strncpy( sp->name, val, sizeof( sp->name ) );
	else if( !Q_stricmp( key, "file" ) ) Q_strncpy( sp->file, val, sizeof( sp->file ) );
	else if( !Q_stricmp( key, "x" ) ) sp->x = atoi( val );
	else if( !Q_stricmp( key, "y" ) ) sp->y = atoi( val );
	else if( !Q_stricmp( key, "width" ) ) sp->width = atoi( val );
	else if( !Q_stricmp( key, "height" ) ) sp->height = atoi( val );
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
				WS_ParseKVBlock( &p, &pk, WS_ApplyAmmoPickup );
			}
		}
	}

	Mem_Free( text );
	Con_Printf( "WeaponScript: parsed %d ammo definitions from %s\n", parsed, filename );
	return parsed;
}

int WeaponScript_ParseWeapon( const char *filename )
{
	char *text = WS_LoadText( filename );
	char *p;
	weaponinfo_t w;
	weaponsound_t snd;
	weaponsprite_t spr;
	qboolean haveSoundBlock = false;

	if( !text )
	{
		Con_Printf( "WeaponScript: cannot open %s\n", filename );
		return -1;
	}

	memset( &w, 0, sizeof( w ) );
	memset( &snd, 0, sizeof( snd ) );
	w.sound = snd;

	p = text;
	while( true )
	{
		char *t = WS_NextToken( &p );
		if( !t )
			break;

		if( !Q_stricmp( t, "WeaponData" ) )
		{
			WS_ParseKVBlock( &p, &w, WS_ApplyWeaponData );
		}
		else if( !Q_stricmp( t, "PrimaryAttack" ) )
		{
			WS_ParseKVBlock( &p, &w.primary, WS_ApplyAttack );
		}
		else if( !Q_stricmp( t, "SecondaryAttack" ) )
		{
			WS_ParseKVBlock( &p, &w.secondary, WS_ApplyAttack );
		}
		else if( !Q_stricmp( t, "SoundData" ) )
		{
			memset( &snd, 0, sizeof( snd ) );
			WS_ParseKVBlock( &p, &snd, WS_ApplySound );
			w.sound = snd;
		}
		else if( !Q_stricmp( t, "hudsprite" ) )
		{
			if( w.num_sprites < MAX_WEAPON_SPRITES )
			{
				memset( &spr, 0, sizeof( spr ) );
				WS_ParseKVBlock( &p, &spr, WS_ApplySprite );
				w.sprites[w.num_sprites++] = spr;
			}
			else
			{
				WS_ParseKVBlock( &p, &spr, WS_ApplySprite );
			}
		}
	}

	if( gNumWeaponInfo < MAX_AMMO_TYPES )
	{
		gWeaponInfo[gNumWeaponInfo++] = w;
		Con_Printf( "WeaponScript: loaded weapon (%d sprites)\n", w.num_sprites );
	}

	Mem_Free( text );
	return 0;
}

void WeaponScript_LoadAll( void )
{
	// Default script locations. The mod may call the single-file parsers
	// directly with its own paths; these are the upstream defaults.
	WeaponScript_ParseAmmoDesc( "ammodesc.txt" );
	if( FS_FileExists( "weapon_m4a2.txt", false ) )
		WeaponScript_ParseWeapon( "weapon_m4a2.txt" );
}
