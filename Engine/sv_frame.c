/*
sv_frame.c - server world snapshot
Copyright (C) 2008 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "common.h"
#include "server.h"
#include "const.h"
#include "net_encode.h"

typedef struct
{
	int		num_entities;
	entity_state_t	entities[MAX_VISIBLE_PACKET];	
} sv_ents_t;

static byte *clientpvs;	// FatPVS
static byte *clientphs;	// FatPHS

int	c_fullsend;	// just a debug counter

/*
=======================
SV_EntityNumbers
=======================
*/
static int SV_EntityNumbers( const void *a, const void *b )
{
	int	ent1, ent2;

	ent1 = ((entity_state_t *)a)->number;
	ent2 = ((entity_state_t *)b)->number;

	if( ent1 == ent2 )
		Host_Error( "SV_SortEntities: duplicated entity\n" );

	if( ent1 < ent2 )
		return -1;
	return 1;
}

/*
=============
SV_AddEntitiesToPacket

=============
*/
static void SV_AddEntitiesToPacket( edict_t *pViewEnt, edict_t *pClient, client_frame_t *frame, sv_ents_t *ents )
{
	edict_t		*ent;
	byte		*pset;
	qboolean		fullvis = false;
	sv_client_t	*netclient;
	sv_client_t	*cl = NULL;
	entity_state_t	*state;
	int		e, player;

	// during an error shutdown message we may need to transmit
	// the shutdown message after the server has shutdown, so
	// specifically check for it
	if( sv.state == ss_dead )
		return;

	cl = SV_ClientFromEdict( pClient, true );

	ASSERT( cl != NULL );

	if( pClient && !( sv.hostflags & SVF_MERGE_VISIBILITY ))
	{
		// portals can't change hostflags
		sv.hostflags &= ~SVF_SKIPLOCALHOST;

		// setup hostflags
		if( FBitSet( cl->flags, FCL_LOCAL_WEAPONS ))
			SetBits( sv.hostflags, SVF_SKIPLOCALHOST );
		else ClearBits( sv.hostflags, SVF_SKIPLOCALHOST );

		// reset viewents each frame
		cl->num_cameras = 0;
	}

	svgame.dllFuncs.pfnSetupVisibility( pViewEnt, pClient, &clientpvs, &clientphs );
	if( !clientpvs ) fullvis = true;

	// don't send the world
	for( e = 1; e < svgame.numEntities; e++ )
	{
		ent = EDICT_NUM( e );
		if( ent->free ) continue;

		// don't double add an entity through portals (already added)
		// HACHACK: use pushmsec to keep net_framenum
		if( ent->v.pushmsec == sv.net_framenum )
			continue;

		if( FBitSet( ent->v.effects, EF_REQUEST_PHS ))
			pset = clientphs;
		else pset = clientpvs;

		state = &ents->entities[ents->num_entities];
		netclient = SV_ClientFromEdict( ent, true );
		player = ( netclient != NULL );

		// add entity to the net packet
		if( svgame.dllFuncs.pfnAddToFullPack( state, e, ent, pClient, sv.hostflags, player, pset ))
		{
			// to prevent adds it twice through portals
			ent->v.pushmsec = sv.net_framenum;

			if( netclient && netclient->modelindex ) // apply custom model if present
				state->modelindex = netclient->modelindex;

			if( SV_IsValidEdict( ent->v.aiment ) && ( ent->v.aiment->v.effects & EF_MERGE_VISIBILITY ))
			{
				if( cl != NULL && cl->num_cameras < MAX_VIEWENTS )
				{
					cl->cameras[cl->num_cameras] = ent->v.aiment;
					cl->num_cameras++;
				}
				else MsgDev( D_ERROR, "SV_AddEntitiesToPacket: too many viewentities!\n" );
			}

			// if we are full, silently discard entities
			if( ents->num_entities < MAX_VISIBLE_PACKET )
			{
				ents->num_entities++;	// entity accepted
				c_fullsend++;		// debug counter
				
			}
			else
			{
				// visibility list is full
				MsgDev( D_ERROR, "too many entities in visible packet list\n" );
				break;
			}
		}

		if( fullvis ) continue; // portal ents will be added anyway, ignore recursion

		// if its a portal entity, add everything visible from its camera position
		if( !( sv.hostflags & SVF_MERGE_VISIBILITY ) && ent->v.effects & EF_MERGE_VISIBILITY )
		{
			SetBits( sv.hostflags, SVF_MERGE_VISIBILITY );
			SV_AddEntitiesToPacket( ent, pClient, frame, ents );
			ClearBits( sv.hostflags, SVF_MERGE_VISIBILITY );
		}
	}
}

/*
=============================================================================

Encode a client frame onto the network channel

=============================================================================
*/
/*
=============
SV_EmitPacketEntities

Writes a delta update of an entity_state_t list to the message->
=============
*/
void SV_EmitPacketEntities( sv_client_t *cl, client_frame_t *to, sizebuf_t *msg )
{
	entity_state_t	*oldent, *newent;
	int		oldindex, newindex;
	int		from_num_entities;
	int		oldnum, newnum;
	client_frame_t	*from;

	// this is the frame that we are going to delta update from
	if( cl->delta_sequence != -1 )
	{
		from = &cl->frames[cl->delta_sequence & SV_UPDATE_MASK];
		from_num_entities = from->num_entities;

		// the snapshot's entities may still have rolled off the buffer, though
		if( from->first_entity <= svs.next_client_entities - svs.num_client_entities )
		{
			MsgDev( D_WARN, "%s: delta request from out of date entities.\n", cl->name );

			from = NULL;
			from_num_entities = 0;

			MSG_WriteByte( msg, svc_packetentities );
			MSG_WriteWord( msg, to->num_entities );
		}
		else
		{
			MSG_WriteByte( msg, svc_deltapacketentities );
			MSG_WriteWord( msg, to->num_entities );
			MSG_WriteByte( msg, cl->delta_sequence );
		}
	}
	else
	{
		from = NULL;
		from_num_entities = 0;

		MSG_WriteByte( msg, svc_packetentities );
		MSG_WriteWord( msg, to->num_entities );
	}

	newent = NULL;
	oldent = NULL;
	newindex = 0;
	oldindex = 0;

	while( newindex < to->num_entities || oldindex < from_num_entities )
	{
		if( newindex >= to->num_entities )
		{
			newnum = MAX_ENTNUMBER;
		}
		else
		{
			newent = &svs.packet_entities[(to->first_entity+newindex)%svs.num_client_entities];
			newnum = newent->number;
		}

		if( oldindex >= from_num_entities )
		{
			oldnum = MAX_ENTNUMBER;
		}
		else
		{
			oldent = &svs.packet_entities[(from->first_entity+oldindex)%svs.num_client_entities];
			oldnum = oldent->number;
		}

		if( newnum == oldnum )
		{	
			// delta update from old position
			// because the force parm is false, this will not result
			// in any bytes being emited if the entity has not changed at all
			MSG_WriteDeltaEntity( oldent, newent, msg, false, SV_IsPlayerIndex( newent->number ), sv.time );
			oldindex++;
			newindex++;
			continue;
		}

		if( newnum < oldnum )
		{	
			// this is a new entity, send it from the baseline
			MSG_WriteDeltaEntity( &svs.baselines[newnum], newent, msg, true, SV_IsPlayerIndex( newent->number ), sv.time );
			newindex++;
			continue;
		}

		if( newnum > oldnum )
		{	
			qboolean	force;

			if( EDICT_NUM( oldent->number )->free )
				force = true;	// entity completely removed from server
			else force = false;		// just removed from delta-message 

			// remove from message
			MSG_WriteDeltaEntity( oldent, NULL, msg, force, false, sv.time );
			oldindex++;
			continue;
		}
	}

	MSG_WriteWord( msg, 0 ); // end of packetentities
}

/*
=============
SV_EmitEvents

=============
*/
static void SV_EmitEvents( sv_client_t *cl, client_frame_t *to, sizebuf_t *msg )
{
	event_state_t	*es;
	event_info_t	*info;
	entity_state_t	*state;
	event_args_t	nullargs;
	int		ev_count = 0;
	int		count, ent_index;
	int		i, j, ev;

	memset( &nullargs, 0, sizeof( nullargs ));

	es = &cl->events;

	// count events
	for( ev = 0; ev < MAX_EVENT_QUEUE; ev++ )
	{
		if( es->ei[ev].index ) ev_count++;
	}

	// nothing to send
	if( !ev_count ) return; // nothing to send

	if( ev_count >= 31 ) ev_count = 31;

	for( i = 0; i < MAX_EVENT_QUEUE; i++ )
	{
		info = &es->ei[i];
		if( info->index == 0 )
			continue;

		ent_index = info->entity_index;

		for( j = 0; j < to->num_entities; j++ )
		{
			state = &svs.packet_entities[(to->first_entity+j)%svs.num_client_entities];
			if( state->number == ent_index )
				break;
		}

		if( j >= to->num_entities )
		{
			// couldn't find
			info->packet_index = to->num_entities;
			info->args.entindex = ent_index;
		}
		else
		{
			info->packet_index = j;
			info->args.ducking = 0;

			if( !FBitSet( info->args.flags, FEVENT_ORIGIN ))
				VectorClear( info->args.origin );

			if( !FBitSet( info->args.flags, FEVENT_ANGLES ))
				VectorClear( info->args.angles );

			VectorClear( info->args.velocity );
		}
	}

	MSG_WriteByte( msg, svc_event );	// create message
	MSG_WriteUBitLong( msg, ev_count, 5 );	// up to MAX_EVENT_QUEUE events

	for( count = i = 0; i < MAX_EVENT_QUEUE; i++ )
	{
		info = &es->ei[i];

		if( info->index == 0 )
		{
			info->packet_index = -1;
			info->entity_index = -1;
			continue;
		}

		// only send if there's room
		if( count < ev_count )
		{
			MSG_WriteUBitLong( msg, info->index, MAX_EVENT_BITS ); // 1024 events

			if( info->packet_index == -1 )
			{
				MSG_WriteOneBit( msg, 0 );
			}
			else
			{
				MSG_WriteOneBit( msg, 1 );
				MSG_WriteUBitLong( msg, info->packet_index, MAX_ENTITY_BITS );

				if( !memcmp( &nullargs, &info->args, sizeof( event_args_t )))
				{
					MSG_WriteOneBit( msg, 0 );
				}
				else
				{
					MSG_WriteOneBit( msg, 1 );
					MSG_WriteDeltaEvent( msg, &nullargs, &info->args );
				}
			}

			if( info->fire_time )
			{
				MSG_WriteOneBit( msg, 1 );
				MSG_WriteWord( msg, Q_rint( info->fire_time * 100.0f ));
			}
			else MSG_WriteOneBit( msg, 0 );
		}

		info->index = 0;
		info->packet_index = -1;
		info->entity_index = -1;
		count++;
	}
}

/*
=============
SV_EmitPings

=============
*/
void SV_EmitPings( sizebuf_t *msg )
{
	sv_client_t	*cl;
	int		packet_loss;
	int		i, ping;

	MSG_WriteByte( msg, svc_updatepings );

	for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		if( cl->state != cs_spawned )
			continue;

		SV_GetPlayerStats( cl, &ping, &packet_loss );

		// there are 25 bits for each client
		MSG_WriteOneBit( msg, 1 );
		MSG_WriteUBitLong( msg, i, MAX_CLIENT_BITS );
		MSG_WriteUBitLong( msg, ping, 12 );
		MSG_WriteUBitLong( msg, packet_loss, 7 );
	}

	// end marker
	MSG_WriteOneBit( msg, 0 );
}

/*
==================
SV_WriteClientdataToMessage

==================
*/
void SV_WriteClientdataToMessage( sv_client_t *cl, sizebuf_t *msg )
{
	clientdata_t	nullcd;
	clientdata_t	*from_cd, *to_cd;
	weapon_data_t	nullwd;
	weapon_data_t	*from_wd, *to_wd;
	client_frame_t	*frame;
	edict_t		*clent;
	int		i;

	memset( &nullcd, 0, sizeof( nullcd ));

	clent = cl->edict;
	frame = &cl->frames[cl->netchan.outgoing_sequence & SV_UPDATE_MASK];

	frame->senttime = host.realtime;
	frame->ping_time = -1.0f;
	frame->latency = -1.0f;

	if( cl->chokecount != 0 )
	{
		MSG_WriteByte( msg, svc_chokecount );
		MSG_WriteByte( msg, cl->chokecount );
		cl->chokecount = 0;
	}

	// update client fixangle
	switch( clent->v.fixangle )
	{
	case 1:
		MSG_WriteByte( msg, svc_setangle );
		MSG_WriteBitAngle( msg, clent->v.angles[0], 16 );
		MSG_WriteBitAngle( msg, clent->v.angles[1], 16 );
		MSG_WriteBitAngle( msg, clent->v.angles[2], 16 );
		clent->v.effects |= EF_NOINTERP;
		break;
	case 2:
		MSG_WriteByte( msg, svc_addangle );
		MSG_WriteBitAngle( msg, clent->v.avelocity[1], 16 );
		clent->v.avelocity[1] = 0.0f;
		break;
	}

	clent->v.fixangle = 0; // reset fixangle

	memset( &frame->clientdata, 0, sizeof( frame->clientdata ));
	clent->v.pushmsec = 0; // reset net framenum

	// update clientdata_t
	svgame.dllFuncs.pfnUpdateClientData( clent, FBitSet( cl->flags, FCL_LOCAL_WEAPONS ), &frame->clientdata );

	MSG_WriteByte( msg, svc_clientdata );
	if( FBitSet( cl->flags, FCL_HLTV_PROXY )) return;	// don't send more nothing

	if( cl->delta_sequence == -1 ) from_cd = &nullcd;
	else from_cd = &cl->frames[cl->delta_sequence & SV_UPDATE_MASK].clientdata;
	to_cd = &frame->clientdata;

	if( cl->delta_sequence == -1 )
	{
		MSG_WriteOneBit( msg, 0 );	// no delta-compression
	}
	else
	{
		MSG_WriteOneBit( msg, 1 );	// we are delta-ing from
		MSG_WriteByte( msg, cl->delta_sequence );
	}

	// write clientdata_t
	MSG_WriteClientData( msg, from_cd, to_cd, sv.time );

	if( FBitSet( cl->flags, FCL_LOCAL_WEAPONS ) && svgame.dllFuncs.pfnGetWeaponData( clent, frame->weapondata ))
	{
		memset( &nullwd, 0, sizeof( nullwd ));

		for( i = 0; i < MAX_LOCAL_WEAPONS; i++ )
		{
			if( cl->delta_sequence == -1 ) from_wd = &nullwd;
			else from_wd = &cl->frames[cl->delta_sequence & SV_UPDATE_MASK].weapondata[i];
			to_wd = &frame->weapondata[i];

			MSG_WriteWeaponData( msg, from_wd, to_wd, sv.time, i );
		}
	}

	// end marker
	MSG_WriteOneBit( msg, 0 );
}

/*
==================
SV_WriteEntitiesToClient

==================
*/
void SV_WriteEntitiesToClient( sv_client_t *cl, sizebuf_t *msg )
{
	edict_t		*clent;
	edict_t		*viewent;	// may be NULL
	client_frame_t	*frame;
	entity_state_t	*state;
	static sv_ents_t	frame_ents;
	int		i, send_pings;

	clent = cl->edict;
	viewent = cl->pViewEntity;	// himself or trigger_camera

	frame = &cl->frames[cl->netchan.outgoing_sequence & SV_UPDATE_MASK];

	send_pings = SV_ShouldUpdatePing( cl );

	ClearBits( sv.hostflags, SVF_MERGE_VISIBILITY );
	sv.net_framenum++;	// now all portal-through entities are invalidate

	// clear everything in this snapshot
	frame_ents.num_entities = c_fullsend = 0;

	// add all the entities directly visible to the eye, which
	// may include portal entities that merge other viewpoints
	SV_AddEntitiesToPacket( viewent, clent, frame, &frame_ents );
   
	// if there were portals visible, there may be out of order entities
	// in the list which will need to be resorted for the delta compression
	// to work correctly.  This also catches the error condition
	// of an entity being included twice.
	qsort( frame_ents.entities, frame_ents.num_entities, sizeof( frame_ents.entities[0] ), SV_EntityNumbers );

	// copy the entity states out
	frame->num_entities = 0;
	frame->first_entity = svs.next_client_entities;

	for( i = 0; i < frame_ents.num_entities; i++ )
	{
		// add it to the circular packet_entities array
		state = &svs.packet_entities[svs.next_client_entities % svs.num_client_entities];
		*state = frame_ents.entities[i];
		svs.next_client_entities++;

		// this should never hit, map should always be restarted first in SV_Frame
		if( svs.next_client_entities >= 0x7FFFFFFE )
			Host_Error( "svs.next_client_entities wrapped\n" );
		frame->num_entities++;
	}

	SV_EmitPacketEntities( cl, frame, msg );
	SV_EmitEvents( cl, frame, msg );
	if( send_pings ) SV_EmitPings( msg );
}

/*
===============================================================================

FRAME UPDATES

===============================================================================
*/
/*
=======================
SV_SendClientDatagram
=======================
*/
void SV_SendClientDatagram( sv_client_t *cl )
{
	static byte    	msg_buf[NET_MAX_PAYLOAD];
	sizebuf_t		msg;

	svs.currentPlayerNum = (cl - svs.clients);
	svs.currentPlayer = cl;

	MSG_Init( &msg, "Datagram", msg_buf, sizeof( msg_buf ));

	// always send servertime at new frame
	MSG_WriteByte( &msg, svc_time );
	MSG_WriteFloat( &msg, sv.time );

	SV_WriteClientdataToMessage( cl, &msg );
	SV_WriteEntitiesToClient( cl, &msg );

	// copy the accumulated multicast datagram
	// for this client out to the message
	if( MSG_CheckOverflow( &cl->datagram )) MsgDev( D_WARN, "datagram overflowed for %s\n", cl->name );
	else MSG_WriteBits( &msg, MSG_GetData( &cl->datagram ), MSG_GetNumBitsWritten( &cl->datagram ));
	MSG_Clear( &cl->datagram );

	if( MSG_CheckOverflow( &msg ))
	{	
		// must have room left for the packet header
		MsgDev( D_WARN, "msg overflowed for %s\n", cl->name );
		MSG_Clear( &msg );
	}

	// send the datagram
	Netchan_TransmitBits( &cl->netchan, MSG_GetNumBitsWritten( &msg ), MSG_GetData( &msg ));
}

/*
=======================
SV_UpdateToReliableMessages
=======================
*/
void SV_UpdateToReliableMessages( void )
{
	sv_client_t	*cl;
	int		i;

	// check for changes to be sent over the reliable streams to all clients
	for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		if( !cl->edict ) continue;	// not in game yet

		if( cl->state != cs_spawned )
			continue;

		if( FBitSet( cl->flags, FCL_RESEND_USERINFO ))
		{
			SV_FullClientUpdate( cl, &sv.reliable_datagram );
			ClearBits( cl->flags, FCL_RESEND_USERINFO );
		}

		if( FBitSet( cl->flags, FCL_RESEND_MOVEVARS ))
		{
			SV_FullUpdateMovevars( cl, &cl->netchan.message );
			ClearBits( cl->flags, FCL_RESEND_MOVEVARS );
		}
	}

	// 1% chanse for simulate random network bugs
	if( sv.write_bad_message && Com_RandomLong( 0, 512 ) == 404 )
	{
		// just for network debugging (send only for local client)
		MSG_WriteByte( &sv.datagram, svc_bad );
		MSG_WriteLong( &sv.datagram, rand( ));		// send some random data
		MSG_WriteString( &sv.datagram, host.finalmsg );	// send final message
		sv.write_bad_message = false;
	}

	// clear the server datagram if it overflowed.
	if( MSG_CheckOverflow( &sv.datagram ))
	{
		MsgDev( D_ERROR, "sv.datagram overflowed!\n" );
		MSG_Clear( &sv.datagram );
	}

	// clear the server datagram if it overflowed.
	if( MSG_CheckOverflow( &sv.spectator_datagram ))
	{
		MsgDev( D_ERROR, "sv.spectator_datagram overflowed!\n" );
		MSG_Clear( &sv.spectator_datagram );
	}

	// now send the reliable and server datagrams to all clients.
	for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		if( cl->state < cs_connected || FBitSet( cl->flags, FCL_FAKECLIENT ))
			continue;	// reliables go to all connected or spawned

		MSG_WriteBits( &cl->netchan.message, MSG_GetData( &sv.reliable_datagram ), MSG_GetNumBitsWritten( &sv.reliable_datagram ));
		MSG_WriteBits( &cl->datagram, MSG_GetData( &sv.datagram ), MSG_GetNumBitsWritten( &sv.datagram ));

		if( FBitSet( cl->flags, FCL_HLTV_PROXY ))
			MSG_WriteBits( &cl->datagram, MSG_GetData( &sv.spectator_datagram ), MSG_GetNumBitsWritten( &sv.spectator_datagram ));
	}

	// now clear the reliable and datagram buffers.
	MSG_Clear( &sv.spectator_datagram );
	MSG_Clear( &sv.reliable_datagram );
	MSG_Clear( &sv.datagram );
}

/*
=======================
SV_SendClientMessages
=======================
*/
void SV_SendClientMessages( void )
{
	sv_client_t	*cl;
	int		i;

	svs.currentPlayer = NULL;
	svs.currentPlayerNum = 0;

	if( sv.state == ss_dead )
		return;

	SV_UpdateToReliableMessages ();

	// send a message to each connected client
	for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		if( !cl->state || FBitSet( cl->flags, FCL_FAKECLIENT ))
			continue;

		if( FBitSet( cl->flags, FCL_SKIP_NET_MESSAGE ))
		{
			ClearBits( cl->flags, FCL_SKIP_NET_MESSAGE );
			continue;
		}

		if( !host_limitlocal->integer && NET_IsLocalAddress( cl->netchan.remote_address ))
			SetBits( cl->flags, FCL_SEND_NET_MESSAGE );

		if( cl->state == cs_spawned )
		{
			// Try to send a message as soon as we can.
			// If the target time for sending is within the next frame interval ( based on last frame ),
			// trigger the send now. Note that in single player,
			// send_message is also set to true any time a packet arrives from the client.
			float	time_unti_next_message = cl->next_messagetime - (host.realtime + host.frametime);

		}

		// if the reliable message overflowed, drop the client
		if( MSG_CheckOverflow( &cl->netchan.message ))
		{
			MSG_Clear( &cl->netchan.message );
			MSG_Clear( &cl->datagram );
			SV_BroadcastPrintf( PRINT_HIGH, "%s overflowed\n", cl->name );
			MsgDev( D_WARN, "reliable overflow for %s\n", cl->name );
			SV_DropClient( cl );
			SetBits( cl->flags, FCL_SEND_NET_MESSAGE );
			cl->netchan.cleartime = 0.0;	// don't choke this message
		}
		else if( FBitSet( cl->flags, FCL_SEND_NET_MESSAGE ))
		{
			// If we haven't gotten a message in sv_failuretime seconds, then stop sending messages to this client
			// until we get another packet in from the client. This prevents crash/drop and reconnect where they are
			// being hosed with "sequenced packet without connection" packets.
			if(( host.realtime - cl->netchan.last_received ) > sv_failuretime->value )
				ClearBits( cl->flags, FCL_SEND_NET_MESSAGE );
		}

		// only send messages if the client has sent one
		// and the bandwidth is not choked
		if( FBitSet( cl->flags, FCL_SEND_NET_MESSAGE ))
		{
			// bandwidth choke active?
			if( !Netchan_CanPacket( &cl->netchan ))
			{
				cl->chokecount++;
				continue;
			}

			ClearBits( cl->flags, FCL_SEND_NET_MESSAGE );

			// Now that we were able to send, reset timer to point to next possible send time.
			cl->next_messagetime = host.realtime + host.frametime + cl->cl_updaterate;

			if( cl->state == cs_spawned )
			{
				SV_SendClientDatagram( cl );
			}
			else
			{
				// just update reliable
				Netchan_Transmit( &cl->netchan, 0, NULL );
			}
		}
	}

	// reset current client
	svs.currentPlayer = NULL;
	svs.currentPlayerNum = 0;
}

/*
=======================
SV_SendMessagesToAll

e.g. before changing level
=======================
*/
void SV_SendMessagesToAll( void )
{
	sv_client_t	*cl;
	int		i;

	if( sv.state == ss_dead )
		return;

	for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		if( cl->state >= cs_connected )
			SetBits( cl->flags, FCL_SEND_NET_MESSAGE );
	}	

	SV_SendClientMessages();
}

/*
=======================
SV_SkipUpdates

used before changing level
=======================
*/
void SV_SkipUpdates( void )
{
	sv_client_t	*cl;
	int		i;

	if( sv.state == ss_dead )
		return;

	for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		if( cl->state != cs_spawned || FBitSet( cl->flags, FCL_FAKECLIENT ))
			continue;

		SetBits( cl->flags, FCL_SKIP_NET_MESSAGE );
	}
}

/*
=======================
SV_InactivateClients

Purpose: Prepare for level transition, etc.
=======================
*/
void SV_InactivateClients( void )
{
	int		i;
	sv_client_t	*cl;

	if( sv.state == ss_dead )
		return;

	// send a message to each connected client
	for( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
	{
		if( !cl->state || !cl->edict )
			continue;
			
		if( !cl->edict || FBitSet( cl->edict->v.flags, FL_FAKECLIENT ))
			continue;

		if( svs.clients[i].state > cs_connected )
			svs.clients[i].state = cs_connected;

		// clear netchan message (but keep other buffers)
		MSG_Clear( &cl->netchan.message );
	}
}