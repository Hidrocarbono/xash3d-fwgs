/*
weaponscript.h - data-driven weapon/ammo script system (Xash Weapon System)
Copyright (C) 2026 Brother Hermes - sistema de armas por script, por Hermes e Hidrocarboneto

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This header mirrors the script format used by Uncle Mike in Paranoia 2
(ammodesc.txt and weapon_*.txt) so existing mods can be imported without
changing their script files.
*/

#ifndef WEAPONSCRIPT_H
#define WEAPONSCRIPT_H

#include "const.h"

#define MAX_AMMO_TYPES		64
#define MAX_WEAPON_SPRITES	8
#define MAX_WEAPON_NAME		64
#define MAX_WEAPON_MODELS	64
#define MAX_AMMO_NAME		32
#define MAX_SOUND_PATH		64
#define MAX_HUDSPRITE_NAME	32

#define WIF_IRONSIGHT	(1<<0)
#define WIF_AUTOAIM		(1<<1)
#define WIF_AUTOFIRE	(1<<2)

typedef struct ammoinfo_s
{
	char	name[MAX_AMMO_NAME];
	int	MaxCarry;
	int	PlayerDamage;
	int	MonsterDamage;
	int	Damage;
	float	Distance;
	int	NumShots;
	char	ShellModel[MAX_WEAPON_MODELS];
	char	Missile[MAX_AMMO_NAME];
	int	count;
} ammoinfo_t;

typedef struct ammopickup_s
{
	char	classname[MAX_WEAPON_NAME];
	char	model[MAX_WEAPON_MODELS];
	char	sound[MAX_SOUND_PATH];
	char	type[MAX_AMMO_NAME];
	int	count;
} ammopickup_t;

typedef struct weaponattack_s
{
	char	action[32];
	float	nextattack;
	float	PunchAngle[3];
	float	PunchAngleIS[3];
	float	SpreadRange[2];
	float	SpreadExpand;
	float	SpreadRangeIS[2];
	float	SpreadExpandIS;
} weaponattack_t;

typedef struct weaponsound_s
{
	char	shootsound1[MAX_SOUND_PATH];
	char	shootsound2[MAX_SOUND_PATH];
	char	emptysound[MAX_SOUND_PATH];
} weaponsound_t;

typedef struct weaponsprite_s
{
	char	name[MAX_HUDSPRITE_NAME];
	char	file[MAX_SOUND_PATH];
	int	x, y, width, height;
} weaponsprite_t;

typedef struct weaponinfo_s
{
	char	viewmodel[MAX_WEAPON_MODELS];
	char	playermodel[MAX_WEAPON_MODELS];
	char	worldmodel[MAX_WEAPON_MODELS];
	char	anim_prefix[32];
	int	bucket;
	int	bucket_position;
	int	clip_size;
	int	defaultammo;
	char	primary_ammo[MAX_AMMO_NAME];
	char	secondary_ammo[MAX_AMMO_NAME];
	int	weight;
	float	SpreadTime;
	int	item_flags;
	float	MaxSpeed;
	float	MaxSpeedIS;
	char	volume[16];
	char	flash[16];

	weaponattack_t	primary;
	weaponattack_t	secondary;
	weaponsound_t	sound;
	weaponsprite_t	sprites[MAX_WEAPON_SPRITES];
	int	num_sprites;
} weaponinfo_t;

extern ammoinfo_t		gAmmoInfo[MAX_AMMO_TYPES];
extern int			gNumAmmoInfo;
extern ammopickup_t		gAmmoPickups[MAX_AMMO_TYPES];
extern int			gNumAmmoPickups;
extern weaponinfo_t		gWeaponInfo[MAX_AMMO_TYPES];
extern int			gNumWeaponInfo;

ammoinfo_t *WeaponScript_FindAmmo( const char *name );
int  WeaponScript_ParseAmmoDesc( const char *filename );
int  WeaponScript_ParseWeapon( const char *filename );
void WeaponScript_LoadAll( void );

ammoinfo_t *WeaponScript_FindAmmo( const char *name );
weaponinfo_t *WeaponScript_FindWeapon( const char *name );
void WeaponScript_Init( void );

#endif // WEAPONSCRIPT_H
