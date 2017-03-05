#include "common.h"

#include <string>
#include <sstream>

#ifdef __cplusplus
extern "C"
{
#endif

class CSaveFunc
{
public:
	CSaveFunc()
	{
		m_pFunc = 0;
		m_strFuncName = "";
	}

	~CSaveFunc()
	{
		m_pFunc = 0;
		m_strFuncName = "";
	}

	std::string m_strFuncName;
	dword m_pFunc;
};

#ifdef __cplusplus
}
#endif

template <typename T>
std::string NumberToString ( T Number )
{
	std::ostringstream ss;
	ss << Number;
	return ss.str();
}