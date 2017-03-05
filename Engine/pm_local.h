/*
pm_local.h - player move interface
Copyright (C) 2010 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef PM_LOCAL_H
#define PM_LOCAL_H

#include "pm_defs.h"

typedef int (*pfnIgnore)( physent_t *pe );	// custom trace filter

//
// pm_trace.c
//
void Pmove_Init( void );
void PM_InitBoxHull( void );
hull_t *PM_HullForBsp( physent_t *pe, playermove_t *pmove, float *offset );
pmtrace_t PM_PlayerTraceExt( playermove_t *pm, vec3_t p1, vec3_t p2, int flags, int numents, physent_t *ents, int ignore_pe, pfnIgnore pmFilter );
int PM_TestPlayerPosition( playermove_t *pmove, vec3_t pos, pmtrace_t *ptrace, pfnIgnore pmFilter );
int PM_HullPointContents( hull_t *hull, int num, const vec3_t p );

//
// pm_surface.c
//
const char *PM_TraceTexture( physent_t *pe, vec3_t vstart, vec3_t vend );
msurface_t *PM_RecursiveSurfCheck( model_t *model, mnode_t *node, vec3_t p1, vec3_t p2 );

#endif//PM_LOCAL_H