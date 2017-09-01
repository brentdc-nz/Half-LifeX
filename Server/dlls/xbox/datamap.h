#ifndef H_DATAMAPXB
#define H_DATAMAPXB

#include <vector>

// Return the number of elements in a statically sized array.
//   DWORD Buffer[100];
//   RTL_NUMBER_OF(Buffer) == 100
// This is also popularly known as: NUMBER_OF, ARRSIZE, _countof, NELEM, etc.
//
#define SIZE_OF_ARRAY(A) (sizeof(A)/sizeof((A)[0]))

struct datamap_t;
struct typedescription_t;

//
// Function prototype for all input handlers.
//
typedef void (CBaseEntity::*inputfunc_t)(void);

typedef enum HL2_fieldtypes
{
	FIELD_VOID = 0,			// No type or value
} fieldtype_t;

#define FTYPEDESC_FUNCTIONTABLE		0x0020		// This is a table entry for a member function pointer

// This does the necessary casting / extract to grab a pointer to a member function as a void *
// UNDONE: Cast to BASEPTR or something else here?
#define EXTRACT_INPUTFUNC_FUNCTIONPTR(x)		(*(inputfunc_t **)(&(x)))

//-----------------------------------------------------------------------------
//
// Macros used to implement datadescs
//

#define DECLARE_SIMPLE_DATADESC() \
	static datamap_t m_DataMap; \
	static datamap_t *GetBaseMap(); \
	template <typename T> friend void DataMapAccess(T *, datamap_t **p); \
	template <typename T> friend datamap_t *DataMapInit(T *);

#define BEGIN_SIMPLE_DATADESC( className ) \
	datamap_t className::m_DataMap = { 0, 0, #className, NULL }; \
	datamap_t *className::GetBaseMap() { return NULL; } \
	BEGIN_DATADESC_GUTS( className )

#define BEGIN_SIMPLE_DATADESC_( className, BaseClass ) \
	datamap_t className::m_DataMap = { 0, 0, #className, NULL }; \
	datamap_t *className::GetBaseMap() { datamap_t *pResult; DataMapAccess((BaseClass *)NULL, &pResult); return pResult; } \
	BEGIN_DATADESC_GUTS( className )

#define	DECLARE_DATADESC() \
	DECLARE_SIMPLE_DATADESC() \
	virtual datamap_t *GetDataDescMap( void );

#define BEGIN_DATADESC_NO_BASE( className ) \
	datamap_t className::m_DataMap = { 0, 0, #className, NULL }; \
	datamap_t *className::GetDataDescMap( void ) { return &m_DataMap; } \
	datamap_t *className::GetBaseMap() { return NULL; } \
	BEGIN_DATADESC_GUTS( className )

#define BEGIN_DATADESC( className ) \
	datamap_t className::m_DataMap = { 0, 0, #className, NULL }; \
	datamap_t *className::GetDataDescMap( void ) { return &m_DataMap; } \
	datamap_t *className::GetBaseMap() { datamap_t *pResult; DataMapAccess((CBaseEntity *)NULL, &pResult); return pResult; } \
	BEGIN_DATADESC_GUTS( className )

#define BEGIN_DATADESC_GUTS( className ) \
	template <typename T> datamap_t *DataMapInit(T *); \
	template <> datamap_t *DataMapInit<className>( className * ); \
	namespace className##_DataDescInit \
	{ \
		datamap_t *g_DataMapHolder = DataMapInit( (className *)NULL ); /* This can/will be used for some clean up duties later */ \
	} \
	\
	template <> datamap_t *DataMapInit<className>( className * ) \
	{ \
		typedef className classNameTypedef; \
		static CDatadescGeneratedNameHolder nameHolder(#className); \
		className::m_DataMap.baseMap = className::GetBaseMap(); \
		static typedescription_t dataDesc[] = \
		{ \
		{ FIELD_VOID, 0, 0, 0, 0 }, /* so you can define "empty" tables */

#define END_DATADESC() \
		}; \
		\
		if ( sizeof( dataDesc ) > sizeof( dataDesc[0] ) ) \
		{ \
			classNameTypedef::m_DataMap.dataNumFields = SIZE_OF_ARRAY( dataDesc ) - 1; \
			classNameTypedef::m_DataMap.dataDesc 	  = &dataDesc[1]; \
		} \
		else \
		{ \
			classNameTypedef::m_DataMap.dataNumFields = 1; \
			classNameTypedef::m_DataMap.dataDesc 	  = dataDesc; \
		} \
		return &classNameTypedef::m_DataMap; \
	}

// replaces EXPORT table for portability and non-DLL based systems (xbox)
#define DEFINE_FUNCTION_RAW( function, func_type )			{ FIELD_VOID, nameHolder.GenerateName(#function), /*{ NULL, NULL },*/ 1, FTYPEDESC_FUNCTIONTABLE, /*NULL, NULL,*/ (inputfunc_t)((func_type)(&classNameTypedef::function))},

//------------------------------------------------------------------------------

struct typedescription_t
{
	fieldtype_t			fieldType;
	const char			*fieldName;
	unsigned short		fieldSize;
	short				flags;
	// for associating function with string names
	inputfunc_t			inputFunc; 
};

//-----------------------------------------------------------------------------
// Purpose: stores the list of objects in the hierarchy
//			used to iterate through an object's data descriptions
//-----------------------------------------------------------------------------
struct datamap_t
{
	typedescription_t	*dataDesc;
	int					dataNumFields;
	char const			*dataClassName;
	datamap_t			*baseMap;

	bool				chains_validated;
	// Have the "packed" offsets been computed
	bool				packed_offsets_computed;
	int					packed_size;
};

//-----------------------------------------------------------------------------

template <typename T> 
inline void DataMapAccess(T *ignored, datamap_t **p)
{
	*p = &T::m_DataMap;
}

//-----------------------------------------------------------------------------

class CDatadescGeneratedNameHolder
{
public:
	CDatadescGeneratedNameHolder( const char *pszBase )
	 : m_pszBase(pszBase)
	{
		m_nLenBase = strlen( m_pszBase );
	}
	
	~CDatadescGeneratedNameHolder()
	{
		for ( int i = 0; i < (int)m_Names.size(); i++ )
		{
			delete m_Names[i];
		}
	}
	
	const char *GenerateName( const char *pszIdentifier )
	{
		char *pBuf = new char[m_nLenBase + strlen(pszIdentifier) + 1];
		strcpy( pBuf, m_pszBase );
		strcat( pBuf, pszIdentifier );
		m_Names.push_back( pBuf );
		return pBuf;
	}
	
private:
	const char *m_pszBase;
	size_t m_nLenBase;
	std::vector<char *> m_Names;
};

//-----------------------------------------------------------------------------

#endif //H_DATAMAPXB