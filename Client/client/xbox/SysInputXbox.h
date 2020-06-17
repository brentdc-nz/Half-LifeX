#ifndef H_SYSXBOXINPUT
#define H_SYSXBOXINPUT

#include "..\..\engine\keydefs.h"

#ifdef __cplusplus
extern "C" 
{
#endif
		 
typedef struct XBGamepadInfo_s
{
	//Buttons
	int iKey;
	int iButtonDown;

	//Thumbsticks
	float fThumbLX;
	float fThumbLY;
	float fThumbRX;
	float fThumbRY;
} XBGamepadInfo_t;

int UpdateGamepad(XBGamepadInfo_t *pGamepadInfo, int key);

#ifdef __cplusplus
}
#endif

#endif //H_SYSXBOXINPUT