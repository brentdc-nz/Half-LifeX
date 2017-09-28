/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "extdll.h"
#include "basemenu.h"
#include "utils.h"
#include ".\common\keydefs.h"
#include "menu_btnsbmp_table.h"

//MARTY - Fixed slashes for the below block
#define ART_MINIMIZE_N	"gfx\\shell\\min_n" 
#define ART_MINIMIZE_F	"gfx\\shell\\min_f"
#define ART_MINIMIZE_D	"gfx\\shell\\min_d"
#define ART_CLOSEBTN_N	"gfx\\shell\\cls_n"
#define ART_CLOSEBTN_F	"gfx\\shell\\cls_f"
#define ART_CLOSEBTN_D	"gfx\\shell\\cls_d"
//MARTY END

#define ID_BACKGROUND	0
#define ID_CONSOLE		1
#define ID_RESUME		2
#define ID_NEWGAME		3
#define ID_HAZARDCOURSE	4
#define ID_CONFIGURATION	5
#define ID_SAVERESTORE	6	
#define ID_MULTIPLAYER	7
#define ID_CUSTOMGAME	8
#define ID_CREDITS		9
#define ID_QUIT		10
#define ID_QUIT_BUTTON	11
#define ID_MINIMIZE		12
#define ID_MSGBOX	 	13
#define ID_MSGTEXT	 	14
#define ID_YES	 	130
#define ID_NO	 	131

typedef struct
{
	menuFramework_s	menu;

	menuBitmap_s	background;
	menuPicButton_s	console;
	menuPicButton_s	resumeGame;
	menuPicButton_s	newGame;
	menuPicButton_s	hazardCourse;
	menuPicButton_s	configuration;
	menuPicButton_s	saveRestore;
	menuPicButton_s	multiPlayer;
	menuPicButton_s	customGame;
	menuPicButton_s	credits;
	menuPicButton_s	quit;

#ifndef _XBOX //MARTY - Remove mouse close and exit buttons
	menuBitmap_s	minimizeBtn;
	menuBitmap_s	quitButton;
#endif

	// quit dialog
	menuAction_s	msgBox;
	menuAction_s	quitMessage;
	menuAction_s	dlgMessage1;
	menuPicButton_s	yes;
	menuPicButton_s	no;
} uiMain_t;

static uiMain_t		uiMain;

/*
=================
UI_MsgBox_Ownerdraw
=================
*/
static void UI_MsgBox_Ownerdraw( void *self )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	UI_FillRect( item->x, item->y, item->width, item->height, uiPromptBgColor );
}

/*
=================
UI_Background_Ownerdraw
=================
*/
static void UI_Background_Ownerdraw( void *self )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	// map has background
	if( CVAR_GET_FLOAT( "sv_background" ))
		return;

	UI_DrawBackground_Callback( self );

	if (uiStatic.m_fHaveSteamBackground || uiStatic.m_fDisableLogo)
		return; // no logos for steam background

	if( GetLogoLength() <= 0.05f || GetLogoWidth() <= 32 )
		return;	// don't draw stub logo (GoldSrc rules)

	float	logoWidth, logoHeight, logoPosY;
	float	scaleX, scaleY;

	scaleX = ScreenWidth / 640.0f;
	scaleY = ScreenHeight / 480.0f;

	logoWidth = GetLogoWidth() * scaleX;
	logoHeight = GetLogoHeight() * scaleY;
	logoPosY = 70 * scaleY;	// 70 it's empirically determined value (magic number)

	DRAW_LOGO( "logo.avi", 0, logoPosY, logoWidth, logoHeight );
}

static void UI_QuitDialog( void )
{
	// toggle main menu between active\inactive
	// show\hide quit dialog
	uiMain.console.generic.flags ^= QMF_INACTIVE; 
	uiMain.resumeGame.generic.flags ^= QMF_INACTIVE;
	uiMain.newGame.generic.flags ^= QMF_INACTIVE;
	uiMain.hazardCourse.generic.flags ^= QMF_INACTIVE;
	uiMain.saveRestore.generic.flags ^= QMF_INACTIVE;
	uiMain.configuration.generic.flags ^= QMF_INACTIVE;
	uiMain.multiPlayer.generic.flags ^= QMF_INACTIVE;
	uiMain.customGame.generic.flags ^= QMF_INACTIVE;
	uiMain.credits.generic.flags ^= QMF_INACTIVE;
	uiMain.quit.generic.flags ^= QMF_INACTIVE;
#ifndef _XBOX //MARTY - Remove mouse close and exit buttons
	uiMain.minimizeBtn.generic.flags ^= QMF_INACTIVE;
	uiMain.quitButton.generic.flags ^= QMF_INACTIVE;
#endif

	uiMain.msgBox.generic.flags ^= QMF_HIDDEN;
	uiMain.quitMessage.generic.flags ^= QMF_HIDDEN;
	uiMain.no.generic.flags ^= QMF_HIDDEN;
	uiMain.yes.generic.flags ^= QMF_HIDDEN;

}

static void UI_PromptDialog( void )
{
	// toggle main menu between active\inactive
	// show\hide quit dialog
	uiMain.console.generic.flags ^= QMF_INACTIVE; 
	uiMain.resumeGame.generic.flags ^= QMF_INACTIVE;
	uiMain.newGame.generic.flags ^= QMF_INACTIVE;
	uiMain.hazardCourse.generic.flags ^= QMF_INACTIVE;
	uiMain.saveRestore.generic.flags ^= QMF_INACTIVE;
	uiMain.configuration.generic.flags ^= QMF_INACTIVE;
	uiMain.multiPlayer.generic.flags ^= QMF_INACTIVE;
	uiMain.customGame.generic.flags ^= QMF_INACTIVE;
	uiMain.credits.generic.flags ^= QMF_INACTIVE;
	uiMain.quit.generic.flags ^= QMF_INACTIVE;
#ifndef _XBOX //MARTY - Remove mouse close and exit buttons
	uiMain.minimizeBtn.generic.flags ^= QMF_INACTIVE;
	uiMain.quitButton.generic.flags ^= QMF_INACTIVE;
#endif

	uiMain.msgBox.generic.flags ^= QMF_HIDDEN;
	uiMain.dlgMessage1.generic.flags ^= QMF_HIDDEN;
	uiMain.no.generic.flags ^= QMF_HIDDEN;
	uiMain.yes.generic.flags ^= QMF_HIDDEN;

}

/*
=================
UI_Main_KeyFunc
=================
*/
static const char *UI_Main_KeyFunc( int key, int down )
{
	if( down && key == K_ESCAPE )
	{
		if ( CL_IsActive( ))
		{
			if(!( uiMain.dlgMessage1.generic.flags & QMF_HIDDEN ))
				UI_PromptDialog();
			else if(!( uiMain.quitMessage.generic.flags & QMF_HIDDEN ))
				UI_QuitDialog();
			else UI_CloseMenu();
		}
		else UI_QuitDialog();
		return uiSoundNull;
	}
	return UI_DefaultKey( &uiMain.menu, key, down );
}

/*
=================
UI_Main_ActivateFunc
=================
*/
static void UI_Main_ActivateFunc( void )
{
	if ( !CL_IsActive( ))
		uiMain.resumeGame.generic.flags |= QMF_HIDDEN;

	if( gpGlobals->developer )
	{
		uiMain.console.generic.y = CL_IsActive() ? 180 : 230;
		UI_ScaleCoords( NULL, &uiMain.console.generic.y, NULL, NULL );
	}
}

/*
=================
UI_Main_HazardCourse
=================
*/
static void UI_Main_HazardCourse( void )
{
	if( CVAR_GET_FLOAT( "host_serverstate" ) && CVAR_GET_FLOAT( "maxplayers" ) > 1 )
		HOST_ENDGAME( "end of the game" );

	CVAR_SET_FLOAT( "skill", 1.0f );
	CVAR_SET_FLOAT( "deathmatch", 0.0f );
	CVAR_SET_FLOAT( "teamplay", 0.0f );
	CVAR_SET_FLOAT( "pausable", 1.0f ); // singleplayer is always allowing pause
	CVAR_SET_FLOAT( "coop", 0.0f );

	CLIENT_COMMAND( FALSE, "hazardcourse\n" );
}

/*
=================
UI_Main_Callback
=================
*/
static void UI_Main_Callback( void *self, int event )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	switch( item->id )
	{
	case ID_QUIT_BUTTON:
		if( event == QM_PRESSED )
			((menuBitmap_s *)self)->focusPic = ART_CLOSEBTN_D;
		else ((menuBitmap_s *)self)->focusPic = ART_CLOSEBTN_F;
		break;
	case ID_MINIMIZE:
		if( event == QM_PRESSED )
			((menuBitmap_s *)self)->focusPic = ART_MINIMIZE_D;
		else ((menuBitmap_s *)self)->focusPic = ART_MINIMIZE_F;
		break;
	}

	if( event != QM_ACTIVATED )
		return;

	switch( item->id )
	{
	case ID_CONSOLE:
		UI_SetActiveMenu( FALSE );
		KEY_SetDest( KEY_CONSOLE );
		break;
	case ID_RESUME:
		UI_CloseMenu();
		break;
	case ID_NEWGAME:
		UI_NewGame_Menu();
		break;
	case ID_HAZARDCOURSE:
		if( CL_IsActive( ))
			UI_PromptDialog();
		else UI_Main_HazardCourse();
		break;
	case ID_MULTIPLAYER:
		UI_MultiPlayer_Menu();
		break;
	case ID_CONFIGURATION:
		UI_Options_Menu();
		break;
	case ID_SAVERESTORE:
		if( CL_IsActive( ))
			UI_SaveLoad_Menu();
		else UI_LoadGame_Menu();
		break;
	case ID_CUSTOMGAME:
		UI_CustomGame_Menu();
		break;
	case ID_CREDITS:
		UI_Credits_Menu();
		break;
	case ID_QUIT:
	case ID_QUIT_BUTTON:
		UI_QuitDialog();
		break;
	case ID_MINIMIZE:
		CLIENT_COMMAND( FALSE, "minimize\n" );
		break;
	case ID_YES:
		if( !( uiMain.quitMessage.generic.flags & QMF_HIDDEN ))
			CLIENT_COMMAND( FALSE, "quit\n" );
		else UI_Main_HazardCourse();
		break;
	case ID_NO:
		if( !( uiMain.quitMessage.generic.flags & QMF_HIDDEN ))
			UI_QuitDialog();
		else UI_PromptDialog();
		break;
	}
}

/*
=================
UI_Main_Init
=================
*/
static void UI_Main_Init( void )
{
	bool bTrainMap;
	bool bCustomGame;

	memset( &uiMain, 0, sizeof( uiMain_t ));

	// training map is present and not equal to startmap
	if( strlen( gMenu.m_gameinfo.trainmap ) && stricmp( gMenu.m_gameinfo.trainmap, gMenu.m_gameinfo.startmap ))
		bTrainMap = true;
	else bTrainMap = false;

	if( CVAR_GET_FLOAT( "host_allow_changegame" ))
		bCustomGame = true;
	else bCustomGame = false;

	// precache .avi file and get logo width and height
	PRECACHE_LOGO( "logo.avi" );

	uiMain.menu.vidInitFunc = UI_Main_Init;
	uiMain.menu.keyFunc = UI_Main_KeyFunc;
	uiMain.menu.activateFunc = UI_Main_ActivateFunc;

	uiMain.background.generic.id = ID_BACKGROUND;
	uiMain.background.generic.type = QMTYPE_BITMAP;
	uiMain.background.generic.flags = QMF_INACTIVE;
	uiMain.background.generic.x = 0;
	uiMain.background.generic.y = 0;
	uiMain.background.generic.width = 1024;
	uiMain.background.generic.height = 768;
#ifdef _XBOX_NOAVILOGO //MARTY
	uiMain.background.pic = ART_BACKGROUNDLOGO;
#else
	uiMain.background.pic = ART_BACKGROUND;
#endif
	uiMain.background.generic.ownerdraw = UI_Background_Ownerdraw;

	uiMain.console.generic.id = ID_CONSOLE;
	uiMain.console.generic.type = QMTYPE_BM_BUTTON;
	uiMain.console.generic.name = "Console";
	uiMain.console.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiMain.console.generic.x = 72;
	uiMain.console.generic.y = CL_IsActive() ? 180 : 230;
	uiMain.console.generic.callback = UI_Main_Callback;

	UI_UtilSetupPicButton( &uiMain.console, PC_CONSOLE );

	uiMain.resumeGame.generic.id = ID_RESUME;
	uiMain.resumeGame.generic.type = QMTYPE_BM_BUTTON;
	uiMain.resumeGame.generic.name = "Resume game";
	uiMain.resumeGame.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY;
	uiMain.resumeGame.generic.statusText = "Return to game.";
	uiMain.resumeGame.generic.x = 72;
	uiMain.resumeGame.generic.y = 230;
	uiMain.resumeGame.generic.callback = UI_Main_Callback;

	UI_UtilSetupPicButton( &uiMain.resumeGame, PC_RESUME_GAME );

	uiMain.newGame.generic.id = ID_NEWGAME;
	uiMain.newGame.generic.type = QMTYPE_BM_BUTTON;
	uiMain.newGame.generic.name = "New game";
	uiMain.newGame.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY;
	uiMain.newGame.generic.statusText = "Start a new game.";
	uiMain.newGame.generic.x = 72;
	uiMain.newGame.generic.y = 280;
	uiMain.newGame.generic.callback = UI_Main_Callback;

	UI_UtilSetupPicButton( &uiMain.newGame, PC_NEW_GAME );

	if ( gMenu.m_gameinfo.gamemode == GAME_MULTIPLAYER_ONLY || !strlen( gMenu.m_gameinfo.startmap ))
		uiMain.newGame.generic.flags |= QMF_GRAYED;

	uiMain.hazardCourse.generic.id = ID_HAZARDCOURSE;
	uiMain.hazardCourse.generic.type = QMTYPE_BM_BUTTON;
	uiMain.hazardCourse.generic.name = "Hazard course";
	uiMain.hazardCourse.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY;
	uiMain.hazardCourse.generic.statusText = "Learn how to play the game";
	uiMain.hazardCourse.generic.x = 72;
	uiMain.hazardCourse.generic.y = 330;
	uiMain.hazardCourse.generic.callback = UI_Main_Callback;
	
	UI_UtilSetupPicButton( &uiMain.hazardCourse, PC_HAZARD_COURSE );

	uiMain.saveRestore.generic.id = ID_SAVERESTORE;
	uiMain.saveRestore.generic.type = QMTYPE_BM_BUTTON;
	uiMain.saveRestore.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY;

	// server.dll needs for reading savefiles or startup newgame
	if( !CheckGameDll( ))
	{
		uiMain.saveRestore.generic.flags |= QMF_GRAYED;
		uiMain.hazardCourse.generic.flags |= QMF_GRAYED;
		uiMain.newGame.generic.flags |= QMF_GRAYED;
	}

	if( CL_IsActive( ))
	{
		uiMain.saveRestore.generic.name = "Save\\Load Game";
		uiMain.saveRestore.generic.statusText = "Load a saved game, save the current game.";
		UI_UtilSetupPicButton(&uiMain.saveRestore,PC_SAVE_LOAD_GAME);
	}
	else
	{
		uiMain.saveRestore.generic.name = "Load Game";
		uiMain.saveRestore.generic.statusText = "Load a previously saved game.";
		uiMain.resumeGame.generic.flags |= QMF_HIDDEN;
		UI_UtilSetupPicButton( &uiMain.saveRestore, PC_LOAD_GAME );
	}

	uiMain.saveRestore.generic.x = 72;
	uiMain.saveRestore.generic.y = bTrainMap ? 380 : 330;
	uiMain.saveRestore.generic.callback = UI_Main_Callback;

	uiMain.configuration.generic.id = ID_CONFIGURATION;
	uiMain.configuration.generic.type = QMTYPE_BM_BUTTON;
	uiMain.configuration.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY;
	uiMain.configuration.generic.name = "Configuration";
	uiMain.configuration.generic.statusText = "Change game settings, configure controls";
	uiMain.configuration.generic.x = 72;
	uiMain.configuration.generic.y = bTrainMap ? 430 : 380;
	uiMain.configuration.generic.callback = UI_Main_Callback;

	UI_UtilSetupPicButton( &uiMain.configuration, PC_CONFIG );

	uiMain.multiPlayer.generic.id = ID_MULTIPLAYER;
	uiMain.multiPlayer.generic.type = QMTYPE_BM_BUTTON;
	uiMain.multiPlayer.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY;
	uiMain.multiPlayer.generic.name = "Multiplayer";
	uiMain.multiPlayer.generic.statusText = "Search for internet servers, configure character";
	uiMain.multiPlayer.generic.x = 72;
	uiMain.multiPlayer.generic.y = bTrainMap ? 480 : 430;
	uiMain.multiPlayer.generic.callback = UI_Main_Callback;

	UI_UtilSetupPicButton( &uiMain.multiPlayer, PC_MULTIPLAYER );

	if ( gMenu.m_gameinfo.gamemode == GAME_SINGLEPLAYER_ONLY )
		uiMain.multiPlayer.generic.flags |= QMF_GRAYED;

	if ( gMenu.m_gameinfo.gamemode == GAME_MULTIPLAYER_ONLY )
	{
		uiMain.saveRestore.generic.flags |= QMF_GRAYED;
		uiMain.hazardCourse.generic.flags |= QMF_GRAYED;
	}

	uiMain.customGame.generic.id = ID_CUSTOMGAME;
	uiMain.customGame.generic.type = QMTYPE_BM_BUTTON;
	uiMain.customGame.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY;
	uiMain.customGame.generic.name = "Custom Game";
	uiMain.customGame.generic.statusText = "Select a custom game";
	uiMain.customGame.generic.x = 72;
	uiMain.customGame.generic.y = bTrainMap ? 530 : 480;
	uiMain.customGame.generic.callback = UI_Main_Callback;

	UI_UtilSetupPicButton( &uiMain.customGame, PC_CUSTOM_GAME );

	uiMain.credits.generic.id = ID_CREDITS;
	uiMain.credits.generic.type = QMTYPE_BM_BUTTON;
	uiMain.credits.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY;
	uiMain.credits.generic.name = "About";
	uiMain.credits.generic.statusText = "Game credits";
	uiMain.credits.generic.x = 72;
	uiMain.credits.generic.y = (bCustomGame) ? (bTrainMap ? 580 : 530) : (bTrainMap ? 530 : 480);
	uiMain.credits.generic.callback = UI_Main_Callback;
	// too short execute string - not a real command

	UI_UtilSetupPicButton( &uiMain.credits, PC_VIEW_README );

	uiMain.quit.generic.id = ID_QUIT;
	uiMain.quit.generic.type = QMTYPE_BM_BUTTON;
	uiMain.quit.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY;
	uiMain.quit.generic.name = "Quit";
	uiMain.quit.generic.statusText = "Quit from game";
	uiMain.quit.generic.x = 72;
	uiMain.quit.generic.y = (bCustomGame) ? (bTrainMap ? 630 : 580) : (bTrainMap ? 580 : 530);
	uiMain.quit.generic.callback = UI_Main_Callback;

	UI_UtilSetupPicButton( &uiMain.quit, PC_QUIT );

#ifndef _XBOX //MARTY - Remove mouse close and exit buttons
	uiMain.minimizeBtn.generic.id = ID_MINIMIZE;
	uiMain.minimizeBtn.generic.type = QMTYPE_BITMAP;
	uiMain.minimizeBtn.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_MOUSEONLY|QMF_ACT_ONRELEASE;
	uiMain.minimizeBtn.generic.x = 952;
	uiMain.minimizeBtn.generic.y = 13;
	uiMain.minimizeBtn.generic.width = 32;
	uiMain.minimizeBtn.generic.height = 32;
	uiMain.minimizeBtn.generic.callback = UI_Main_Callback;
	uiMain.minimizeBtn.pic = ART_MINIMIZE_N;
	uiMain.minimizeBtn.focusPic = ART_MINIMIZE_F;

	uiMain.quitButton.generic.id = ID_QUIT_BUTTON;
	uiMain.quitButton.generic.type = QMTYPE_BITMAP;
	uiMain.quitButton.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_MOUSEONLY|QMF_ACT_ONRELEASE;
	uiMain.quitButton.generic.x = 984;
	uiMain.quitButton.generic.y = 13;
	uiMain.quitButton.generic.width = 32;
	uiMain.quitButton.generic.height = 32;
	uiMain.quitButton.generic.callback = UI_Main_Callback;
	uiMain.quitButton.pic = ART_CLOSEBTN_N;
	uiMain.quitButton.focusPic = ART_CLOSEBTN_F;
#endif

	uiMain.msgBox.generic.id = ID_MSGBOX;
	uiMain.msgBox.generic.type = QMTYPE_ACTION;
	uiMain.msgBox.generic.flags = QMF_INACTIVE|QMF_HIDDEN;
	uiMain.msgBox.generic.ownerdraw = UI_MsgBox_Ownerdraw; // just a fill rectangle
	uiMain.msgBox.generic.x = 192;
	uiMain.msgBox.generic.y = 256;
	uiMain.msgBox.generic.width = 640;
	uiMain.msgBox.generic.height = 256;

	uiMain.quitMessage.generic.id = ID_MSGBOX;
	uiMain.quitMessage.generic.type = QMTYPE_ACTION;
	uiMain.quitMessage.generic.flags = QMF_INACTIVE|QMF_DROPSHADOW|QMF_HIDDEN|QMF_CENTER_JUSTIFY;
	uiMain.quitMessage.generic.name = "Are you sure you want to quit?";
	uiMain.quitMessage.generic.x = 192;
	uiMain.quitMessage.generic.y = 280;
	uiMain.quitMessage.generic.width = 640;
	uiMain.quitMessage.generic.height = 256;

	uiMain.dlgMessage1.generic.id = ID_MSGTEXT;
	uiMain.dlgMessage1.generic.type = QMTYPE_ACTION;
	uiMain.dlgMessage1.generic.flags = QMF_INACTIVE|QMF_DROPSHADOW|QMF_HIDDEN|QMF_CENTER_JUSTIFY;
	uiMain.dlgMessage1.generic.name = "Starting a Hazard Course will exit";
	uiMain.dlgMessage1.generic.x = 192;
	uiMain.dlgMessage1.generic.y = 280;
	uiMain.dlgMessage1.generic.width = 640;
	uiMain.dlgMessage1.generic.height = 256;

	uiMain.yes.generic.id = ID_YES;
	uiMain.yes.generic.type = QMTYPE_BM_BUTTON;
	uiMain.yes.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_HIDDEN;
	uiMain.yes.generic.name = "Ok";
	uiMain.yes.generic.x = 380;
	uiMain.yes.generic.y = 460;
	uiMain.yes.generic.callback = UI_Main_Callback;

	UI_UtilSetupPicButton( &uiMain.yes, PC_OK );

	uiMain.no.generic.id = ID_NO;
	uiMain.no.generic.type = QMTYPE_BM_BUTTON;
	uiMain.no.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_HIDDEN;
	uiMain.no.generic.name = "Cancel";
	uiMain.no.generic.x = 530;
	uiMain.no.generic.y = 460;
	uiMain.no.generic.callback = UI_Main_Callback;
	
	UI_UtilSetupPicButton( &uiMain.no, PC_CANCEL );

	UI_AddItem( &uiMain.menu, (void *)&uiMain.background );

	if ( gpGlobals->developer )
		UI_AddItem( &uiMain.menu, (void *)&uiMain.console );

	UI_AddItem( &uiMain.menu, (void *)&uiMain.resumeGame );
	UI_AddItem( &uiMain.menu, (void *)&uiMain.newGame );

	if ( bTrainMap )
		UI_AddItem( &uiMain.menu, (void *)&uiMain.hazardCourse );

	UI_AddItem( &uiMain.menu, (void *)&uiMain.saveRestore );
	UI_AddItem( &uiMain.menu, (void *)&uiMain.configuration );
	UI_AddItem( &uiMain.menu, (void *)&uiMain.multiPlayer );

	if ( bCustomGame )
		UI_AddItem( &uiMain.menu, (void *)&uiMain.customGame );

	UI_AddItem( &uiMain.menu, (void *)&uiMain.credits );
	UI_AddItem( &uiMain.menu, (void *)&uiMain.quit );
#ifndef _XBOX //MARTY - Remove mouse close and exit buttons
	UI_AddItem( &uiMain.menu, (void *)&uiMain.minimizeBtn );
	UI_AddItem( &uiMain.menu, (void *)&uiMain.quitButton );
#endif
	UI_AddItem( &uiMain.menu, (void *)&uiMain.msgBox );
	UI_AddItem( &uiMain.menu, (void *)&uiMain.quitMessage );
	UI_AddItem( &uiMain.menu, (void *)&uiMain.dlgMessage1 );
	UI_AddItem( &uiMain.menu, (void *)&uiMain.no );
	UI_AddItem( &uiMain.menu, (void *)&uiMain.yes );
}

/*
=================
UI_Main_Precache
=================
*/
void UI_Main_Precache( void )
{
#ifdef _XBOX_NOAVILOGO //MARTY
	PIC_Load( ART_BACKGROUNDLOGO );
#endif
	PIC_Load( ART_BACKGROUND );
	PIC_Load( ART_MINIMIZE_N );
	PIC_Load( ART_MINIMIZE_F );
	PIC_Load( ART_MINIMIZE_D );
	PIC_Load( ART_CLOSEBTN_N );
	PIC_Load( ART_CLOSEBTN_F );
	PIC_Load( ART_CLOSEBTN_D );

	// precache .avi file and get logo width and height
	PRECACHE_LOGO( "logo.avi" );
}

/*
=================
UI_Main_Menu
=================
*/
void UI_Main_Menu( void )
{
	UI_Main_Precache();
	UI_Main_Init();

	UI_PushMenu( &uiMain.menu );
}        