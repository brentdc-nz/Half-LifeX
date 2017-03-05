#include "TempSaveHack.h"
#include "SavedFunc.h"

#include <vector>

static std::vector<CSaveFunc*> g_vecSavedFuncs;
static int iFuncCount = 0;

const char* TSH_NameForFunction( dword function )
{
	CSaveFunc* pSaveFunc = NULL;

	pSaveFunc = new CSaveFunc;
	
	pSaveFunc->m_pFunc = function;
	pSaveFunc->m_strFuncName = NumberToString(iFuncCount);

	g_vecSavedFuncs.push_back(pSaveFunc);

	iFuncCount++;

	return pSaveFunc->m_strFuncName.c_str();
}

dword TSH_FunctionFromName( const char* pName )
{
	for(int i=0; i < (int)g_vecSavedFuncs.size(); i++)
	{
		CSaveFunc* pSaveFunc = g_vecSavedFuncs[i];

		if(!Q_strcmp( pSaveFunc->m_strFuncName.c_str(), pName ))
			return pSaveFunc->m_pFunc;
	}

	return 0;
}

void TSH_CleanUp()
{
	for(int i=0; i < (int)g_vecSavedFuncs.size(); i++)
	{
		CSaveFunc* pSaveFunc = g_vecSavedFuncs[i];
		
		delete pSaveFunc;
		pSaveFunc = NULL;
	}
}