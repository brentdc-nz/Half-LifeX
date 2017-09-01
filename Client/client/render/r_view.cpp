//=======================================================================
//			Copyright (C) Shambler Team 2005
//			r_view.cpp - multipass renderer
//=======================================================================

#include "..\hud.h"
#include "..\utils.h"
#include "r_view.h"
#include "..\ref_params.h"
#include "..\..\game_shared\pm_movevars.h"
#include "..\..\game_shared\pm_defs.h"
#include "r_local.h"
#include "..\..\common\r_studioint.h"
#include "r_studio.h"
#include "..\..\game_shared\mathlib.h"

//#define min(a, b)  (((a) < (b)) ? (a) : (b))  //MARTY FIXME WIP
//#define max(a, b) (a) > (b) ? (a) : (b)  //MARTY FIXME WIP

// thirdperson camera
void StaticCAM_Think( void ) { R_ClearScene(); } //MARTY - Renamed Static
void StaticCL_CameraOffset( float *ofs ) { g_vecZero.CopyToArray( ofs ); } //MARTY - Renamed Static
int StaticCL_IsThirdPerson( void ){ return (gHUD.m_iCameraMode ? 1 : 0); } //MARTY - Renamed Static

cl_entity_t *v_intermission_spot;
float v_idlescale;
int pause = 0;

cvar_t	*r_test;
cvar_t	*cl_bobcycle;
cvar_t	*cl_bob;
cvar_t	*cl_bobup;
cvar_t	*cl_waterdist;
cvar_t	*cl_chasedist;
cvar_t	*cl_weaponlag;
cvar_t	*cl_vsmoothing;
cvar_t	*v_centermove;
cvar_t	*v_centerspeed;
cvar_t	*cl_viewsize;
cvar_t	*gl_renderer;
cvar_t	*r_overview;
cvar_t	*r_finish;
cvar_t	*r_fastsky;
cvar_t	*r_speeds;
cvar_t	*r_stencilbits;
cvar_t	*r_draw_beams;
cvar_t	*r_width;
cvar_t	*r_height;
cvar_t	*r_novis;
cvar_t	*r_nocull;
cvar_t	*r_lockpvs;
cvar_t	*r_dynamic;
cvar_t	*r_shadows;
cvar_t	*mod_allow_materials;
cvar_t	*r_lightmap;
cvar_t	*r_adjust_fov;
cvar_t	*r_wireframe;
cvar_t	*r_fullbright;
cvar_t	*r_allow_static;
cvar_t	*r_allow_mirrors;
cvar_t	*r_allow_portals;
cvar_t	*r_allow_screens;
cvar_t	*r_drawentities;
cvar_t	*r_faceplanecull;
cvar_t	*r_detailtextures;
cvar_t	*r_lighting_ambient;
cvar_t	*r_lighting_modulate;
cvar_t	*r_lighting_extended;
cvar_t	*r_lightstyle_lerping;
cvar_t	*r_bloom_alpha;
cvar_t	*r_bloom_diamond_size;
cvar_t	*r_bloom_intensity;
cvar_t	*r_bloom_darken;
cvar_t	*r_bloom_sample_size;
cvar_t	*r_bloom_fast_sample;
cvar_t	*r_bloom;
cvar_t	*r_grass;
cvar_t	*r_grass_alpha;
cvar_t	*r_grass_lighting;
cvar_t	*r_grass_shadows;
cvar_t	*r_grass_fade_start;
cvar_t	*r_grass_fade_dist;
cvar_t	*r_overbright;
cvar_t	*r_skyfog;

cvar_t	v_iyaw_cycle	= { "v_iyaw_cycle", "2", 0, 2 };
cvar_t	v_iroll_cycle	= { "v_iroll_cycle", "0.5", 0, 0.5 };
cvar_t	v_ipitch_cycle	= { "v_ipitch_cycle", "1", 0, 1 };
cvar_t	v_iyaw_level  	= { "v_iyaw_level", "0.3", 0, 0.3 };
cvar_t	v_iroll_level 	= { "v_iroll_level", "0.1", 0, 0.1 };
cvar_t	v_ipitch_level	= { "v_ipitch_level", "0.3", 0, 0.3 };

vec3_t ev_punchangle;

//============================================================================== 
//				VIEW RENDERING 
//============================================================================== 

//==========================
// V_ThirdPerson
//
// switch to third person
//==========================
void V_ThirdPerson( void )
{
	if( gEngfuncs.GetMaxClients() == 1 )
		gHUD.m_iCameraMode = 1;
}

//==========================
// V_FirstPerson
//
// switch to first person
//==========================
void V_FirstPerson( void )
{
	gHUD.m_iCameraMode = 0;
}

/*
=============
V_PunchAxis

Client side punch effect
=============
*/

void V_DropPunchAngle( float frametime,  Vector &punchangle )
{
    float	len;

    len = punchangle.Length();
    punchangle = punchangle.Normalize();

    len -= (10.0 + len * 0.5) * frametime;
    len = max( len, 0.0 );
    punchangle *= len;
}

void V_PunchAxis( int axis, float punch )
{
    ev_punchangle[ axis ] = punch;
}

//==========================
// V_Init
//==========================
void V_Init( void )
{
	v_centermove	= CVAR_REGISTER( "v_centermove", "0.15", 0 );
	v_centerspeed	= CVAR_REGISTER( "v_centerspeed","500", 0 );

	cl_bobcycle	= CVAR_REGISTER( "cl_bobcycle","0.8", 0 );
	cl_bob		= CVAR_REGISTER( "cl_bob","0.01", 0 );
	cl_bobup		= CVAR_REGISTER( "cl_bobup","0.5", 0 );
	cl_waterdist	= CVAR_REGISTER( "cl_waterdist","4", 0 );
	cl_chasedist	= CVAR_REGISTER( "cl_chasedist","112", 0 );

    #if XT_FEATURES //non-canon :)
    cl_weaponlag	= CVAR_REGISTER( "cl_weaponlag", "0.3", FCVAR_ARCHIVE );
    #else
    cl_weaponlag	= CVAR_REGISTER( "cl_weaponlag", "0", FCVAR_ARCHIVE );
    #endif

	// setup some engine cvars for custom rendering
	mod_allow_materials = CVAR_GET_POINTER( "host_allow_materials" );
	r_overview	= CVAR_GET_POINTER( "dev_overview" );
	r_finish		= CVAR_GET_POINTER( "gl_finish" );
	r_fastsky		= CVAR_GET_POINTER( "r_fastsky" );
	r_speeds		= CVAR_GET_POINTER( "r_speeds" );
	r_stencilbits	= CVAR_GET_POINTER( "gl_stencilbits" );
	r_width		= CVAR_GET_POINTER( "width" );
	r_height		= CVAR_GET_POINTER( "height" );
	r_test		= CVAR_GET_POINTER( "gl_test" );
	cl_viewsize	= CVAR_GET_POINTER( "viewsize" );

	r_novis		= CVAR_GET_POINTER( "r_novis" );
	r_nocull		= CVAR_GET_POINTER( "r_nocull" );
	r_lockpvs		= CVAR_GET_POINTER( "r_lockpvs" );
	r_dynamic		= CVAR_GET_POINTER( "r_dynamic" );
	r_lightmap	= CVAR_GET_POINTER( "r_lightmap" );
	r_wireframe	= CVAR_GET_POINTER( "gl_wireframe" );
	r_draw_beams	= CVAR_GET_POINTER( "cl_draw_beams" );
	r_adjust_fov	= CVAR_GET_POINTER( "r_adjust_fov" );

	r_fullbright	= CVAR_GET_POINTER( "r_fullbright" );
	r_drawentities	= CVAR_GET_POINTER( "r_drawentities" );
	r_allow_static	= CVAR_GET_POINTER( "gl_allow_static" );
	r_allow_mirrors	= CVAR_GET_POINTER( "gl_allow_mirrors" );
	r_detailtextures	= CVAR_GET_POINTER( "r_detailtextures" );
	r_faceplanecull	= CVAR_GET_POINTER( "r_faceplanecull" );
	r_lighting_ambient	= CVAR_GET_POINTER( "r_lighting_ambient" );
	r_lighting_modulate	= CVAR_GET_POINTER( "r_lighting_modulate" );
	r_lightstyle_lerping= CVAR_GET_POINTER( "cl_lightstyle_lerping" );
	r_lighting_extended	= CVAR_GET_POINTER( "r_lighting_extended" );

	cl_vsmoothing	= CVAR_REGISTER( "cl_vsmoothing", "0.05", FCVAR_ARCHIVE );
	r_allow_portals	= CVAR_REGISTER( "gl_allow_portals", "1", FCVAR_ARCHIVE|FCVAR_CLIENTDLL );
	r_allow_screens	= CVAR_REGISTER( "gl_allow_screens", "1", FCVAR_ARCHIVE|FCVAR_CLIENTDLL );
	gl_renderer	= CVAR_REGISTER( "gl_renderer", "1", FCVAR_CLIENTDLL|FCVAR_ARCHIVE ); 
	r_shadows		= CVAR_REGISTER( "r_shadows", "2", FCVAR_CLIENTDLL|FCVAR_ARCHIVE ); 

	r_bloom		= CVAR_REGISTER( "r_bloom", "0", FCVAR_ARCHIVE );
	r_bloom_alpha	= CVAR_REGISTER( "r_bloom_alpha", "0.1", FCVAR_ARCHIVE );
	r_bloom_diamond_size = CVAR_REGISTER( "r_bloom_diamond_size", "8", FCVAR_ARCHIVE );
	r_bloom_intensity	= CVAR_REGISTER( "r_bloom_intensity", "1", FCVAR_ARCHIVE );
	r_bloom_darken	= CVAR_REGISTER( "r_bloom_darken", "4", FCVAR_ARCHIVE );
	r_bloom_sample_size = CVAR_REGISTER( "r_bloom_sample_size", "320", FCVAR_ARCHIVE );
	r_bloom_fast_sample = CVAR_REGISTER( "r_bloom_fast_sample", "0", FCVAR_ARCHIVE );

	r_grass = CVAR_REGISTER( "r_grass", "1", FCVAR_ARCHIVE );
	r_grass_alpha = CVAR_REGISTER( "r_grass_alpha", "0.5", FCVAR_ARCHIVE );
	r_grass_lighting = CVAR_REGISTER( "r_grass_lighting", "1", FCVAR_ARCHIVE );
	r_grass_shadows = CVAR_REGISTER( "r_grass_shadows", "1", FCVAR_ARCHIVE );
	r_grass_fade_start = CVAR_REGISTER( "r_grass_fade_start", "1280", FCVAR_ARCHIVE );
	r_grass_fade_dist = CVAR_REGISTER( "r_grass_fade_dist", "512", FCVAR_ARCHIVE );

	r_overbright = CVAR_REGISTER( "r_overbright", "0", FCVAR_ARCHIVE );

	r_skyfog = CVAR_REGISTER( "r_skyfog", "0", FCVAR_ARCHIVE );

	ADD_COMMAND( "thirdperson", V_ThirdPerson );
	ADD_COMMAND( "firstperson", V_FirstPerson );
	ADD_COMMAND( "centerview", V_StartPitchDrift );
}

//==========================
// V_CalcBob
//
// Quakeworld bob code, this fixes jitters in the mutliplayer
// since the clock (pparams->time) isn't quite linear
//==========================
float V_CalcBob( struct ref_params_s *pparams )
{
	static double bobtime;
	static float bob, lasttime;
	float cycle;
	Vector vel;

	if( pparams->onground == -1 || pparams->time == lasttime )
		return bob;	

	lasttime = pparams->time;

	bobtime += pparams->frametime;
	cycle = bobtime - (int)( bobtime / cl_bobcycle->value ) * cl_bobcycle->value;
	cycle /= cl_bobcycle->value;
	
	if( cycle < cl_bobup->value )
	{
		cycle = M_PI * cycle / cl_bobup->value;
	}
	else
	{
		cycle = M_PI + M_PI * ( cycle - cl_bobup->value ) / ( 1.0f - cl_bobup->value );
	}

	vel = pparams->simvel;
	bob = sqrt( vel.x * vel.x + vel.y * vel.y ) * cl_bob->value;
	bob = bob * 0.3f + bob * 0.7f * sin( cycle );

	return bound( -7, bob, 4 );
}

extern cvar_t *cl_forwardspeed;

struct
{
	float	pitchvel;
	int  	nodrift;
	float	driftmove;
	double	laststop;
} pd;

//==========================
// V_StartPitchDrift
//==========================
void V_StartPitchDrift( void )
{
	if( pd.laststop == GET_CLIENT_TIME( ))
	{
		// something else is keeping it from drifting
		return;
	}

	if( pd.nodrift || !pd.pitchvel )
	{
		pd.pitchvel = v_centerspeed->value;
		pd.driftmove = 0;
		pd.nodrift = 0;
	}
}

//==========================
// V_StopPitchDrift
//==========================
void V_StopPitchDrift( void )
{
	pd.laststop = GET_CLIENT_TIME();
	pd.nodrift = 1;
	pd.pitchvel = 0;
}

/*
===============
V_DriftPitch

Moves the client pitch angle towards idealpitch sent by the server.

If the user is adjusting pitch manually, either with lookup/lookdown,
mlook and mouse, or klook and keyboard, pitch drifting is constantly stopped.
===============
*/
void V_DriftPitch( struct ref_params_s *pparams )
{
	if( gEngfuncs.IsNoClipping() || !pparams->onground || pparams->demoplayback )
	{
		pd.driftmove = 0;
		pd.pitchvel = 0;
		return;
	}

	if( pd.nodrift )
	{
		if( fabs( pparams->cmd->forwardmove ) < cl_forwardspeed->value )
		{
			pd.driftmove = 0;
		}
		else
		{
			pd.driftmove += pparams->frametime;
		}

		if( pd.driftmove > v_centermove->value )
			V_StartPitchDrift ();
		return;
	}
	
	float delta = pparams->idealpitch - pparams->cl_viewangles[PITCH];

	if( !delta )
	{
		pd.pitchvel = 0;
		return;
	}

	float move = pparams->frametime * pd.pitchvel;
	pd.pitchvel += pparams->frametime * v_centerspeed->value;
	
	if( delta > 0 )
	{
		if( move > delta )
		{
			pd.pitchvel = 0;
			move = delta;
		}

		pparams->cl_viewangles[PITCH] += move;
	}
	else if( delta < 0 )
	{
		if( move > -delta )
		{
			pd.pitchvel = 0;
			move = -delta;
		}

		pparams->cl_viewangles[PITCH] -= move;
	}
}

//==========================
// V_CalcGunAngle
//==========================
void V_CalcGunAngle( struct ref_params_s *pparams )
{	
	cl_entity_t *viewent;
	
	viewent = GET_VIEWMODEL();
	if( !viewent ) return;

	viewent->angles[YAW] = pparams->viewangles[YAW] + pparams->crosshairangle[YAW];
    viewent->angles[PITCH] = -pparams->viewangles[PITCH] + pparams->crosshairangle[PITCH] * 0.25f;  //wow, stupid quake bug again?
	viewent->angles[ROLL] -= v_idlescale * sin( pparams->time * v_iroll_cycle.value ) * v_iroll_level.value;
	
	// don't apply all of the v_ipitch to prevent normally unseen parts of viewmodel from coming into view.
	viewent->angles[PITCH] -= v_idlescale * sin( pparams->time*v_ipitch_cycle.value ) * (v_ipitch_level.value * 0.5f);
	viewent->angles[YAW] -= v_idlescale * sin( pparams->time*v_iyaw_cycle.value ) * v_iyaw_level.value;

	viewent->latched.prevangles = viewent->angles;
    viewent->curstate.angles = viewent->angles;
}

//==========================
// V_CalcViewModelLag
//==========================
void V_CalcViewModelLag( ref_params_t *pparams, Vector &origin, Vector &angles, const Vector &original_angles )
{
	static Vector m_vecLastFacing;
	Vector vOriginalOrigin = origin;
	Vector vOriginalAngles = angles;

	// Calculate our drift
	Vector forward, right, up;

	AngleVectors( angles, forward, right, up );

	if( pparams->frametime != 0.0f )	// not in paused
	{
		Vector vDifference;

		vDifference = forward - m_vecLastFacing;

		float flSpeed = 5.0f;

		// If we start to lag too far behind, we'll increase the "catch up" speed.
		// Solves the problem with fast cl_yawspeed, m_yaw or joysticks rotating quickly.
		// The old code would slam lastfacing with origin causing the viewmodel to pop to a new position
		float flDiff = vDifference.Length();

		if(( flDiff > cl_weaponlag->value ) && ( cl_weaponlag->value > 0.0f ))
		{
			float flScale = flDiff / cl_weaponlag->value;
			flSpeed *= flScale;
		}

		// FIXME:  Needs to be predictable?
		m_vecLastFacing = m_vecLastFacing + vDifference * ( flSpeed * pparams->frametime );
		// Make sure it doesn't grow out of control!!!
		m_vecLastFacing = m_vecLastFacing.Normalize();
		origin = origin + (vDifference * -1.0f) * flSpeed;
	}

	AngleVectors( original_angles, forward, right, up );

	float pitch = original_angles[PITCH];

	if( pitch > 180.0f )
	{
		pitch -= 360.0f;
	}
	else if( pitch < -180.0f )
	{
		pitch += 360.0f;
	}

	if( cl_weaponlag->value <= 0.0f )
	{
		origin = vOriginalOrigin;
		angles = vOriginalAngles;
	}
	else
	{
		// FIXME: These are the old settings that caused too many exposed polys on some models
		origin = origin + forward * ( -pitch * 0.035f );
		origin = origin + right * ( -pitch * 0.03f );
		origin = origin + up * ( -pitch * 0.02f );
	}
}

//==========================
// V_AddIdle
//==========================
void V_AddIdle( struct ref_params_s *pparams )
{
	pparams->viewangles[ROLL] += v_idlescale * sin(pparams->time*v_iroll_cycle.value) * v_iroll_level.value;
	pparams->viewangles[PITCH] += v_idlescale * sin(pparams->time*v_ipitch_cycle.value) * v_ipitch_level.value;
	pparams->viewangles[YAW] += v_idlescale * sin(pparams->time*v_iyaw_cycle.value) * v_iyaw_level.value;
}

//==========================
// V_CalcViewRoll
//==========================
void V_CalcViewRoll( struct ref_params_s *pparams )
{
	float   sign, side, value;
	Vector  right;
	
	cl_entity_t *viewentity = GET_ENTITY( pparams->viewentity );
	if( !viewentity ) return;

	AngleVectors( viewentity->angles, NULL, right, NULL );
	side = DotProduct( pparams->simvel, right );
	sign = side < 0 ? -1 : 1;
	side = fabs( side );
	value = pparams->movevars->rollangle;

	if( side < pparams->movevars->rollspeed )
		side = side * value / pparams->movevars->rollspeed;
	else side = value;

	side = side * sign;		
	pparams->viewangles[ROLL] += side;

	if( pparams->health <= 0 && ( pparams->viewheight[2] != 0 ))
	{
		// only roll the view if the player is dead and the viewheight[2] is nonzero 
		// this is so deadcam in multiplayer will work.
		pparams->viewangles[ROLL] = 80; // dead view angle
		return;
	}
}

//==========================
// V_CalcSendOrigin
//==========================
void V_CalcSendOrigin( struct ref_params_s *pparams )
{
	// never let view origin sit exactly on a node line, because a water plane can
	// dissapear when viewed with the eye exactly on it.
	// FIXME, we send origin at 1/128 now, change this?
	// the server protocol only specifies to 1/16 pixel, so add 1/32 in each axis
	pparams->vieworg[0] += 1.0f / 32;
	pparams->vieworg[1] += 1.0f / 32;
	pparams->vieworg[2] += 1.0f / 32;
}

//==========================
// V_GetChaseOrigin
//==========================
void V_GetChaseOrigin( const Vector &angles, const Vector &origin, float distance, Vector &returnvec )
{
	Vector vecStart, vecEnd;
	pmtrace_t *trace;
	int maxLoops = 8;

	Vector forward, right, up;	

	// trace back from the target using the player's view angles
	AngleVectors( angles, forward, right, up );
	forward = -forward;

	vecStart = origin;
	vecEnd = vecStart + forward * distance;

	int ignoreent = -1;	// first, ignore no entity
	cl_entity_t *ent = NULL;

	while( maxLoops > 0 )
	{
		trace = gEngfuncs.PM_TraceLine( vecStart, vecEnd, PM_TRACELINE_PHYSENTSONLY, 2, ignoreent );
		if( trace->ent <= 0 ) break; // we hit the world or nothing, stop trace

		ent = GET_ENTITY( PM_GetPhysEntInfo( trace->ent ));
		if( ent == NULL ) break;

		// hit non-player solid BSP, stop here
		if( ent->curstate.solid == SOLID_BSP && !ent->player )
			break;

		// if close enought to end pos, stop, otherwise continue trace
		if(( vecEnd - trace->endpos ).Length() < 1.0f )
		{
			break;
		}
		else
		{
			ignoreent = trace->ent;	// ignore last hit entity
			vecStart = trace->endpos;
		}
		maxLoops--;
	}  

	returnvec = trace->endpos + trace->plane.normal * 8;
}

//==========================
// V_CalcCameraRefdef
//==========================
void V_CalcCameraRefdef( struct ref_params_s *pparams )
{
	static float lasttime, oldz = 0;

	// get viewentity and monster eyeposition
	cl_entity_t *view = GET_ENTITY( pparams->viewentity );

 	if( view )
	{		 
		pparams->vieworg = view->origin;
		pparams->viewangles = view->angles;

		// interpolate position for monsters
		if( view->curstate.movetype == MOVETYPE_STEP ) 
		{
			float f;
                    
			// don't do it if the goalstarttime hasn't updated in a while.
			// NOTE:  Because we need to interpolate multiplayer characters, the interpolation time limit
			//  was increased to 1.0 s., which is 2x the max lag we are accounting for.
			if(( GET_CLIENT_TIME() < view->curstate.animtime + 1.0f ) && ( view->curstate.animtime != view->latched.prevanimtime ))
			{
				f = (GET_CLIENT_TIME() - view->curstate.animtime) / (view->curstate.animtime - view->latched.prevanimtime);
			}

			if( !( view->curstate.effects & EF_NOINTERP ))
			{
				// ugly hack to interpolate angle, position.
				// current is reached 0.1 seconds after being set
				f = f - 1.0f;
			}
			else
			{
				f = 0.0f;
			}

			InterpolateOrigin( view->latched.prevorigin, view->origin, pparams->vieworg, f, true );
			InterpolateAngles( view->latched.prevangles, view->angles, pparams->viewangles, f, true );
		}

		studiohdr_t *viewmonster = (studiohdr_t *)IEngineStudio.Mod_Extradata( view->model );

		if( viewmonster && view->curstate.eflags & EFLAG_SLERP )
		{
			Vector forward;
			AngleVectors( pparams->viewangles, forward, NULL, NULL );

			Vector viewpos = viewmonster->eyeposition;

			if( viewpos == g_vecZero )
				viewpos = Vector( 0, 0, 8 );	// monster_cockroach

			pparams->vieworg += viewpos + forward * 8;	// best value for humans
			pparams->fov_x = 100; // adjust fov for monster view
			pparams->fov_y = V_CalcFov( pparams->fov_x, pparams->viewport[2], pparams->viewport[3] );
		}

		// this is smooth stair climbing in thirdperson mode but not affected for client model :(
		if( !pparams->smoothing && pparams->onground && view->origin[2] - oldz > 0.0f )
		{
			float steptime;
		
			steptime = pparams->time - lasttime;
			if( steptime < 0 ) steptime = 0;

			oldz += steptime * 150.0f;

			if( oldz > view->origin[2] )
				oldz = view->origin[2];
			if( view->origin[2] - oldz > pparams->movevars->stepsize )
				oldz = view->origin[2] - pparams->movevars->stepsize;

			pparams->vieworg[2] += oldz - view->origin[2];
		}
		else
		{
			oldz = view->origin[2];
		}

		lasttime = pparams->time;

		if( view->curstate.effects & EF_NUKE_ROCKET )
			pparams->viewangles.x = -pparams->viewangles.x; // stupid quake bug!

		// g-cont. apply shake to camera
		gEngfuncs.V_CalcShake();
		gEngfuncs.V_ApplyShake( pparams->vieworg, pparams->viewangles, 1.0f );
	}
}

#define MAX_SPOTS	16

//==========================
// V_FindIntermisionSpot
//==========================
cl_entity_t *V_FindIntermisionSpot( struct ref_params_s *pparams )
{
	int spotindex[MAX_SPOTS];
	cl_entity_t *ent;

	// found intermission point
	for( int i = 0, j = 0; i < pparams->max_entities; i++ )
	{
		ent = GET_ENTITY( i );

		if( ent && ent->curstate.eflags & EFLAG_INTERMISSION )
		{
			spotindex[j] = ent->index;
			if( ++j >= MAX_SPOTS ) break; // full
		}
	}	
	
	// ok, we have list of intermission spots
	if( j != 0 )
	{
		if( j > 1 ) j = RANDOM_LONG( 0, j );
		ent = GET_ENTITY( spotindex[j-1] );
	}
	else
	{
		// defaulted to player view
		ent = gEngfuncs.GetLocalPlayer();
	}	

	return ent;
}

//==========================
// V_CalcIntermisionRefdef
//==========================
void V_CalcIntermisionRefdef( struct ref_params_s *pparams )
{
          if( !v_intermission_spot )
          	v_intermission_spot = V_FindIntermisionSpot( pparams );

	pparams->vieworg = v_intermission_spot->origin;
	pparams->viewangles = v_intermission_spot->angles;

	cl_entity_t *view = GET_VIEWMODEL();	
	view->model = NULL;

	// allways idle in intermission
	float old = v_idlescale;
	v_idlescale = 1;
	V_AddIdle( pparams );
	v_idlescale = old;
}

//==========================
// V_CalcThirdPersonRefdef
//==========================
void V_CalcThirdPersonRefdef( struct ref_params_s * pparams )
{
	static float lasttime, oldz = 0;

	pparams->vieworg = pparams->simorg;
	pparams->vieworg += pparams->viewheight;
	pparams->viewangles = pparams->cl_viewangles;

	V_CalcSendOrigin( pparams );

	// this is smooth stair climbing in thirdperson mode but not affected for client model :(
	if( !pparams->smoothing && pparams->onground && pparams->simorg[2] - oldz > 0.0f )
	{
		float steptime;
		
		steptime = pparams->time - lasttime;
		if( steptime < 0 ) steptime = 0;

		oldz += steptime * 150.0f;

		if( oldz > pparams->simorg[2] )
			oldz = pparams->simorg[2];
		if( pparams->simorg[2] - oldz > pparams->movevars->stepsize )
			oldz = pparams->simorg[2] - pparams->movevars->stepsize;

		pparams->vieworg[2] += oldz - pparams->simorg[2];
	}
	else
	{
		oldz = pparams->simorg[2];
	}

	lasttime = pparams->time;

	V_GetChaseOrigin( pparams->viewangles, pparams->vieworg, cl_chasedist->value, pparams->vieworg );

	float pitch = pparams->viewangles[PITCH];

	// normalize angles
	if( pitch > 180.0f ) 
		pitch -= 360.0f;
	else if( pitch < -180.0f )
		pitch += 360.0f;

	// player pitch is inverted
	pitch /= -3.0;

	cl_entity_t *ent = GET_LOCAL_PLAYER();

	// slam local player's pitch value
	ent->angles[PITCH] = pitch;
	ent->curstate.angles[PITCH] = pitch;
	ent->prevstate.angles[PITCH] = pitch;
	ent->latched.prevangles[PITCH] = pitch;
}

//==========================
// V_CalcWaterLevel
//==========================
float V_CalcWaterLevel( struct ref_params_s *pparams )
{
	float waterOffset = 0.0f;
	
	if( pparams->waterlevel >= 2 )
	{
		int waterEntity = WATER_ENTITY( pparams->simorg );
		float waterDist = cl_waterdist->value;

		if( waterEntity >= 0 && waterEntity < pparams->max_entities )
		{
			cl_entity_t *pwater = GET_ENTITY( waterEntity );
			if( pwater && ( pwater->model != NULL ))
				waterDist += ( pwater->curstate.scale * 16.0f );
		}

		Vector point = pparams->vieworg;

		// eyes are above water, make sure we're above the waves
		if( pparams->waterlevel == 2 )	
		{
			point.z -= waterDist;

			for( int i = 0; i < waterDist; i++ )
			{
				int contents = POINT_CONTENTS( point );
				if( contents > CONTENTS_WATER )
					break;
				point.z += 1;
			}
			waterOffset = (point.z + waterDist) - pparams->vieworg[2];
		}
		else
		{
			// eyes are under water. Make sure we're far enough under
			point[2] += waterDist;

			for( int i = 0; i < waterDist; i++ )
			{
				int contents = POINT_CONTENTS( point );
				if( contents <= CONTENTS_WATER )
					break;

				point.z -= 1;
			}
			waterOffset = (point.z - waterDist) - pparams->vieworg[2];
		}
	}

	return waterOffset;
}

#define ORIGIN_BACKUP	64
#define ORIGIN_MASK		( ORIGIN_BACKUP - 1 )

typedef struct 
{
	Vector	Origins[ORIGIN_BACKUP];
	float	OriginTime[ORIGIN_BACKUP];

	Vector	Angles[ORIGIN_BACKUP];
	float	AngleTime[ORIGIN_BACKUP];

	int	CurrentOrigin;
	int	CurrentAngle;
} viewinterp_t;

//==========================
// V_InterpolatePos
//==========================
void V_InterpolatePos( struct ref_params_s *pparams )
{
	static Vector lastorg;
	static viewinterp_t	ViewInterp;

	// view is the weapon model (only visible from inside body )
	cl_entity_t *view = GET_VIEWMODEL();

	Vector delta = pparams->simorg - lastorg;

	if( delta.Length() != 0.0f )
	{
		ViewInterp.Origins[ViewInterp.CurrentOrigin & ORIGIN_MASK] = pparams->simorg;
		ViewInterp.OriginTime[ViewInterp.CurrentOrigin & ORIGIN_MASK] = pparams->time;
		ViewInterp.CurrentOrigin++;

		lastorg = pparams->simorg;
	}

	if( cl_vsmoothing->value && pparams->smoothing && ( pparams->maxclients > 1 ))
	{
		int i, foundidx;
		float t;

		if( cl_vsmoothing->value < 0.0f )
			CVAR_SET_FLOAT( "cl_vsmoothing", 0.0 );

		t = pparams->time - cl_vsmoothing->value;

		for( i = 1; i < ORIGIN_MASK; i++ )
		{
			foundidx = ViewInterp.CurrentOrigin - 1 - i;

			if( ViewInterp.OriginTime[foundidx & ORIGIN_MASK] <= t )
				break;
		}

		if( i < ORIGIN_MASK &&  ViewInterp.OriginTime[foundidx & ORIGIN_MASK] != 0.0f )
		{
			// Interpolate
			Vector delta, neworg;
			double dt, frac;

			dt = ViewInterp.OriginTime[(foundidx + 1) & ORIGIN_MASK] - ViewInterp.OriginTime[foundidx & ORIGIN_MASK];
			if( dt > 0.0f )
			{
				frac = ( t - ViewInterp.OriginTime[foundidx & ORIGIN_MASK] ) / dt;
				delta = ViewInterp.Origins[( foundidx + 1 ) & ORIGIN_MASK] - ViewInterp.Origins[foundidx & ORIGIN_MASK];
				frac = min( 1.0, frac );

				neworg = ViewInterp.Origins[foundidx & ORIGIN_MASK] + delta * frac;

				// dont interpolate large changes (less than 64 units)
				if( delta.Length() < 64 )
				{
					delta = neworg - pparams->simorg;
					pparams->simorg += delta;
					pparams->vieworg += delta;
					view->origin += delta;
				}
			}
		}
	}
}

//==========================
// V_CalcFirstPersonRefdef
//==========================
void V_CalcFirstPersonRefdef( struct ref_params_s *pparams )
{
	V_DriftPitch( pparams );

	float bob = V_CalcBob( pparams );

	pparams->vieworg = pparams->simorg;
	pparams->vieworg += pparams->viewheight;
	pparams->vieworg.z += bob;

	pparams->viewangles = pparams->cl_viewangles;

	gEngfuncs.V_CalcShake();
	gEngfuncs.V_ApplyShake( pparams->vieworg, pparams->viewangles, 1.0f );

	V_CalcSendOrigin( pparams );

	float waterOffset = V_CalcWaterLevel( pparams );
	pparams->vieworg[2] += waterOffset;

	V_CalcViewRoll( pparams );
	V_AddIdle( pparams );

	// offsets
	AngleVectors( pparams->cl_viewangles, pparams->forward, pparams->right, pparams->up );

	cl_entity_t *view = GET_VIEWMODEL();
	Vector lastAngles = view->angles = pparams->cl_viewangles;

	V_CalcGunAngle( pparams );

	// use predicted origin as view origin.
	view->origin = pparams->simorg;      
	view->origin += pparams->viewheight;
	view->origin.z += waterOffset;

	// Let the viewmodel shake at about 10% of the amplitude
	gEngfuncs.V_ApplyShake( view->origin, view->angles, 0.9f );

	view->origin += pparams->forward * bob * 0.4f;
	view->origin.z += bob;

	view->angles[PITCH] -= bob * 0.3f;
	view->angles[YAW] -= bob * 0.5f;
	view->angles[ROLL] -= bob * 1.0f;
	view->origin.z -= 1;

	// fudge position around to keep amount of weapon visible
	// roughly equal with different FOV
	if( pparams->viewsize == 110 )
	{
		view->origin[2] += 1;
	}
	else if( pparams->viewsize == 100 )
	{
		view->origin[2] += 2;
	}
	else if( pparams->viewsize == 90 )
	{
		view->origin[2] += 1;
	}
	else if( pparams->viewsize == 80 )
	{
		view->origin[2] += 0.5;
	}

	V_CalcViewModelLag( pparams, view->origin, view->angles, lastAngles );
		
	pparams->viewangles += pparams->punchangle;
    pparams->viewangles += ev_punchangle;

    V_DropPunchAngle(pparams->frametime,ev_punchangle);

	static float lasttime, oldz = 0;

	if( !pparams->smoothing && pparams->onground && pparams->simorg[2] - oldz > 0.0f )
	{
		float steptime;
		
		steptime = pparams->time - lasttime;
		if( steptime < 0 ) steptime = 0;

		oldz += steptime * 150.0f;

		if( oldz > pparams->simorg[2] )
			oldz = pparams->simorg[2];
		if( pparams->simorg[2] - oldz > pparams->movevars->stepsize )
			oldz = pparams->simorg[2] - pparams->movevars->stepsize;

		pparams->vieworg[2] += oldz - pparams->simorg[2];
		view->origin.z += oldz - pparams->simorg[2];
	}
	else
	{
		oldz = pparams->simorg[2];
	}

	lasttime = pparams->time;

	// smooth player view in multiplayer
	V_InterpolatePos( pparams );
}

//==========================
// V_CalcGlobalFog
//==========================
void V_CalcGlobalFog( struct ref_params_s *pparams )
{

}

//==========================
// V_CalcRefdef
//==========================
void StaticV_CalcRefdef( struct ref_params_s *pparams ) //MARTY - Static rename
{
    //pause = pparams->paused;
    //if( pause ) return;

	if( pparams->intermission )
	{
		V_CalcIntermisionRefdef( pparams );
	}
	else if( pparams->viewentity > pparams->maxclients )
	{
		V_CalcCameraRefdef( pparams );
	}
	else if( gHUD.m_iCameraMode )
	{
		V_CalcThirdPersonRefdef( pparams );
	}
	else
	{
		V_CalcFirstPersonRefdef( pparams );
	}

	// fog that can be controlled from server-side
	V_CalcGlobalFog( pparams );
}
