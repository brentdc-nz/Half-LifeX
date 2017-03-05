#define AS_TO_TITLE		1
#define AS_TO_BUTTON	2

void UI_SetTitleAnim( int anim_state, menuPicButton_s *picid );
void UI_DrawTitleAnim( void );
void UI_InitTitleAnim( void );
void UI_TACheckMenuDepth( void );
float UI_GetTitleTransFraction( void );

typedef struct  
{
	int x, y, lx, ly;
} quad_t;

void UI_PopPButtonStack( void );
void UI_ClearButtonStack( void );

// ������������ ����� �� btns_main.bmp ������� head_%s.bmp
//#define TA_ALT_MODE 1