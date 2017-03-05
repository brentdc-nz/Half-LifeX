#include "system.h"

typedef void (*pfnChangeGame)( const char *progname );

int /*EXPORT*/ Host_Main( const char *progname, int bChangeGame, pfnChangeGame func );

