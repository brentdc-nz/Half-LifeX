/*
r_misc.cpp - renderer misceallaneous
Copyright (C) 2011 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "..\hud.h"
#include "..\utils.h"
#include "r_local.h"
#include "..\..\common\entity_types.h"
#include "..\..\common\event_api.h"
#include "..\..\common\r_efx.h"
#include "..\..\game_shared\pm_defs.h"
#include "..\..\game_shared\mathlib.h"
#include "r_view.h"

#define max(a, b)  (((a) > (b)) ? (a) : (b))

//#define FLASHLIGHT_DISTANCE		2048	// in units

// experimental stuff
//#define FLASHLIGHT_SPOT		// use new-style flashlight like in Paranoia

/*
================
HUD_UpdateFlashlight

update client flashlight
================
*/
/*void HUD_UpdateFlashlight( cl_entity_t *pEnt )
{
	Vector	v_angles, forward, right, up;
	Vector	v_origin;

	if( UTIL_IsLocal( pEnt->index ))
	{
		ref_params_t tmpRefDef = RI.refdef;

		// player seen through camera. Restore firstperson view here
		if( RI.refdef.viewentity > RI.refdef.maxclients )
			V_CalcFirstPersonRefdef( &tmpRefDef );

		v_angles = tmpRefDef.viewangles;
		v_origin = tmpRefDef.vieworg;
	}
	else
	{
		// restore viewangles from angles
		v_angles[PITCH] = -pEnt->angles[PITCH] * 3;
		v_angles[YAW] = pEnt->angles[YAW];
		v_angles[ROLL] = 0;	// no roll
		v_origin = pEnt->origin;
	}

	AngleVectors( v_angles, forward, NULL, NULL );

	Vector vecEnd = v_origin + forward * FLASHLIGHT_DISTANCE;

	pmtrace_t	trace;
	int traceFlags = PM_STUDIO_BOX;

	if( r_lighting_extended->value < 2 )
		traceFlags |= PM_GLASS_IGNORE;

	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
	gEngfuncs.pEventAPI->EV_PlayerTrace( v_origin, vecEnd, traceFlags, -1, &trace );
	float falloff = trace.fraction * FLASHLIGHT_DISTANCE;

	if( falloff < 250.0f ) falloff = 1.0f;
	else falloff = 250.0f / falloff;

	falloff *= falloff;

	// update flashlight endpos
	dlight_t	*dl = gEngfuncs.pEfxAPI->CL_AllocDlight( pEnt->curstate.number );
	dl->origin = trace.endpos;
	dl->die = GET_CLIENT_TIME() + 0.01f; // die on next frame
	dl->color.r = bound( 0, 255 * falloff, 255 );
	dl->color.g = bound( 0, 255 * falloff, 255 );
	dl->color.b = bound( 0, 255 * falloff, 255 );
	dl->radius = 72;
}*/

/*
========================
HUD_AddEntity

Return 0 to filter entity from visible list for rendering
========================
*/
int StaticHUD_AddEntity( int type, struct cl_entity_s *ent, const char *modelname ) //MARTY - Renamed Static
{
	return 1;
}

/*
=========================
HUD_TxferLocalOverrides

The server sends us our origin with extra precision as part of the clientdata structure, not during the normal
playerstate update in entity_state_t.  In order for these overrides to eventually get to the appropriate playerstate
structure, we need to copy them into the state structure at this point.
=========================
*/
void StaticHUD_TxferLocalOverrides( struct entity_state_s *state, const struct clientdata_s *client )  //MARTY - Renamed Static
{
	state->origin = client->origin;
	state->velocity = client->velocity;

	// Duck prevention
	state->iuser3 = client->iuser3;

	// Fire prevention
	state->iuser4 = client->iuser4;

	// always have valid PVS message
	r_currentMessageNum = state->messagenum;
}

/*
=========================
HUD_ProcessPlayerState

We have received entity_state_t for this player over the network.  We need to copy appropriate fields to the
playerstate structure
=========================
*/
void StaticHUD_ProcessPlayerState( struct entity_state_s *dst, const struct entity_state_s *src )   //MARTY - Renamed Static
{
	// Copy in network data
	dst->origin	= src->origin;
	dst->angles	= src->angles;

	dst->velocity	= src->velocity;
          dst->basevelocity	= src->basevelocity;
          	
	dst->frame	= src->frame;
	dst->modelindex	= src->modelindex;
	dst->skin		= src->skin;
	dst->effects	= src->effects;
	dst->weaponmodel	= src->weaponmodel;
	dst->movetype	= src->movetype;
	dst->sequence	= src->sequence;
	dst->animtime	= src->animtime;
	
	dst->solid	= src->solid;
	
	dst->rendermode	= src->rendermode;
	dst->renderamt	= src->renderamt;	
	dst->rendercolor.r	= src->rendercolor.r;
	dst->rendercolor.g	= src->rendercolor.g;
	dst->rendercolor.b	= src->rendercolor.b;
	dst->renderfx	= src->renderfx;

	dst->framerate	= src->framerate;
	dst->body		= src->body;

	dst->friction	= src->friction;
	dst->gravity	= src->gravity;
	dst->gaitsequence	= src->gaitsequence;
	dst->usehull	= src->usehull;
	dst->playerclass	= src->playerclass;
	dst->team		= src->team;
	dst->colormap	= src->colormap;

	memcpy( &dst->controller[0], &src->controller[0], 4 * sizeof( byte ));
	memcpy( &dst->blending[0], &src->blending[0], 2 * sizeof( byte ));

	// Save off some data so other areas of the Client DLL can get to it
	cl_entity_t *player = GET_LOCAL_PLAYER(); // Get the local player's index

	if( dst->number == player->index )
	{
		// always have valid PVS message
		r_currentMessageNum = src->messagenum;
	}
}

/*
=========================
HUD_TxferPredictionData

Because we can predict an arbitrary number of frames before the server responds with an update, we need to be able to copy client side prediction data in
 from the state that the server ack'd receiving, which can be anywhere along the predicted frame path ( i.e., we could predict 20 frames into the future and the server ack's
 up through 10 of those frames, so we need to copy persistent client-side only state from the 10th predicted frame to the slot the server
 update is occupying.
=========================
*/
void StaticHUD_TxferPredictionData( entity_state_t *ps, const entity_state_t *pps, clientdata_t *pcd, const clientdata_t *ppcd, weapon_data_t *wd, const weapon_data_t *pwd ) //MARTY - Renamed Static
{
	ps->oldbuttons	= pps->oldbuttons;
	ps->flFallVelocity	= pps->flFallVelocity;
	ps->iStepLeft	= pps->iStepLeft;
	ps->playerclass	= pps->playerclass;

	pcd->viewmodel	= ppcd->viewmodel;
	pcd->m_iId	= ppcd->m_iId;
	pcd->ammo_shells	= ppcd->ammo_shells;
	pcd->ammo_nails	= ppcd->ammo_nails;
	pcd->ammo_cells	= ppcd->ammo_cells;
	pcd->ammo_rockets	= ppcd->ammo_rockets;
	pcd->m_flNextAttack	= ppcd->m_flNextAttack;
	pcd->fov		= ppcd->fov;
	pcd->weaponanim	= ppcd->weaponanim;
	pcd->tfstate	= ppcd->tfstate;
	pcd->maxspeed	= ppcd->maxspeed;

	pcd->deadflag	= ppcd->deadflag;

	// Duck prevention
	pcd->iuser3 	= ppcd->iuser3;

	// Fire prevention
	pcd->iuser4 	= ppcd->iuser4;

	pcd->fuser2	= ppcd->fuser2;
	pcd->fuser3	= ppcd->fuser3;

	pcd->vuser1 = ppcd->vuser1;
	pcd->vuser2 = ppcd->vuser2;
	pcd->vuser3 = ppcd->vuser3;
	pcd->vuser4 = ppcd->vuser4;

	memcpy( wd, pwd, 32 * sizeof( weapon_data_t ));
}

/*
=========================
HUD_CreateEntities
	
Gives us a chance to add additional entities to the render this frame
=========================
*/
void StaticHUD_CreateEntities( void ) //MARTY - Renamed Static
{
	// e.g., create a persistent cl_entity_t somewhere.
	// Load an appropriate model into it ( gEngfuncs.CL_LoadModel )
	// Call gEngfuncs.CL_CreateVisibleEntity to add it to the visedicts list

	if( tr.world_has_portals || tr.world_has_screens )
		StaticHUD_AddEntity( ET_PLAYER, GET_LOCAL_PLAYER(), GET_LOCAL_PLAYER()->model->name ); //MARTY - Renamed Static
}

//======================
//	DRAW BEAM EVENT
//
// special effect for displacer
//======================
void HUD_DrawBeam( void )
{
	cl_entity_t *view = GET_VIEWMODEL();
	int m_iBeam = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/plasma.spr" );

	gEngfuncs.pEfxAPI->R_BeamEnts( view->index|0x1000, view->index|0x2000, m_iBeam, 1.05f, 0.8f, 0.5f, 0.5f, 0.6f, 0, 10, 2, 10, 0 );
	gEngfuncs.pEfxAPI->R_BeamEnts( view->index|0x1000, view->index|0x3000, m_iBeam, 1.05f, 0.8f, 0.5f, 0.5f, 0.6f, 0, 10, 2, 10, 0 );
	gEngfuncs.pEfxAPI->R_BeamEnts( view->index|0x1000, view->index|0x4000, m_iBeam, 1.05f, 0.8f, 0.5f, 0.5f, 0.6f, 0, 10, 2, 10, 0 );
}

//======================
//	Eject Shell
//
// eject specified shell from viewmodel
// NOTE: shell model must be precached on a server
//======================
void HUD_EjectShell( const struct mstudioevent_s *event, const struct cl_entity_s *entity )
{
	if( entity != GET_VIEWMODEL( ))
		return; // can eject shells only from viewmodel

	Vector view_ofs, ShellOrigin, ShellVelocity, forward, right, up;
	Vector angles = Vector( 0, entity->angles[YAW], 0 );
	Vector origin = entity->origin;
	
	float fR, fU;

          int shell = gEngfuncs.pEventAPI->EV_FindModelIndex( event->options );

	if( !shell )
	{
		ALERT( at_error, "model %s not precached\n", event->options );
		return;
	}

	for( int i = 0; i < 3; i++ )
	{
		if( angles[i] < -180 ) angles[i] += 360; 
		else if( angles[i] > 180 ) angles[i] -= 360;
          }

	angles.x = -angles.x;
	AngleVectors( angles, forward, right, up );
          
	fR = RANDOM_FLOAT( 50, 70 );
	fU = RANDOM_FLOAT( 100, 150 );

	for( i = 0; i < 3; i++ )
	{
		ShellVelocity[i] = RI.refdef.simvel[i] + right[i] * fR + up[i] * fU + forward[i] * 25;
		ShellOrigin[i]   = RI.refdef.vieworg[i] + up[i] * -12 + forward[i] * 20 + right[i] * 4;
	}

	gEngfuncs.pEfxAPI->R_TempModel( ShellOrigin, ShellVelocity, angles, RANDOM_LONG( 5, 10 ), shell, TE_BOUNCE_SHELL );
}

/*
=========================
HUD_StudioEvent

The entity's studio model description indicated an event was
fired during this frame, handle the event by it's tag ( e.g., muzzleflash, sound )
=========================
*/
void StaticHUD_StudioEvent( const struct mstudioevent_s *event, const struct cl_entity_s *entity ) //MARTY - Renamed Static 
{
	switch( event->event )
	{
	case 5001:
		gEngfuncs.pEfxAPI->R_MuzzleFlash( (float *)&entity->attachment[0], Q_atoi( event->options ));
		break;
	case 5011:
		gEngfuncs.pEfxAPI->R_MuzzleFlash( (float *)&entity->attachment[1], Q_atoi( event->options ));
		break;
	case 5021:
		gEngfuncs.pEfxAPI->R_MuzzleFlash( (float *)&entity->attachment[2], Q_atoi( event->options ));
		break;
	case 5031:
		gEngfuncs.pEfxAPI->R_MuzzleFlash( (float *)&entity->attachment[3], Q_atoi( event->options ));
		break;
	case 5002:
		gEngfuncs.pEfxAPI->R_SparkEffect( (float *)&entity->attachment[0], Q_atoi( event->options ), -100, 100 );
		break;
	case 5004: // client side sound		
		gEngfuncs.pfnPlaySoundByNameAtLocation( (char *)event->options, 1.0, (float *)&entity->attachment[0] );
		break;
	case 5005: // client side sound with random pitch		
		gEngfuncs.pEventAPI->EV_PlaySound( entity->index, (float *)&entity->attachment[0], CHAN_WEAPON, (char *)event->options,
		RANDOM_FLOAT( 0.7f, 0.9f ), ATTN_NORM, 0, 85 + RANDOM_LONG( 0, 0x1f ));
		break;
	case 5050: // Special event for displacer
		HUD_DrawBeam();
		break;
	case 5060:
	          HUD_EjectShell( event, entity );
		break;
	default:
		ALERT( at_error, "Unknown event %i with options %i\n", event->event, event->options );
		break;
	}
}