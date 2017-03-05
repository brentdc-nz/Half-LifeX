#ifndef TEMPSAVEHACK_H
#define TEMPSAVEHACK_H

#include "common.h"

#ifdef __cplusplus
extern "C"
{
#endif

const char* TSH_NameForFunction( dword function );
dword TSH_FunctionFromName( const char* pName );
void TSH_CleanUp();

#ifdef __cplusplus
}
#endif

#endif //TEMPSAVEHACK_H
