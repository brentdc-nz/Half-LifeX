#include "Utils.h"
#include <xtl.h>

#define MB	(1024*1024)
#define AddStr(a,b) (pstrOut += wsprintf( pstrOut, a, b ))

void GetMemUsage()
{
		MEMORYSTATUS stat;
		CHAR strOut[1024], *pstrOut;

		// Get the memory status.
		GlobalMemoryStatus( &stat );

		// Setup the output string.
		pstrOut = strOut;
		AddStr( "%4d total MB of virtual memory.\n", stat.dwTotalVirtual / MB );
		AddStr( "%4d  free MB of virtual memory.\n", stat.dwAvailVirtual / MB );
		AddStr( "%4d total MB of physical memory.\n", stat.dwTotalPhys / MB );
		AddStr( "%4d  free MB of physical memory.\n", stat.dwAvailPhys / MB );
		AddStr( "%4d total MB of paging file.\n", stat.dwTotalPageFile / MB );
		AddStr( "%4d  free MB of paging file.\n", stat.dwAvailPageFile / MB );
		AddStr( "%4d  percent of memory is in use.\n", stat.dwMemoryLoad );

		// Output the string.
		OutputDebugString( strOut );
}

