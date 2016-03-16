#pragma once

class MAKFC_CString;
class MAKFC_CInputStream;
class MAKFC_COutputStream;

typedef std::vector<MAKFC_CString> MAKFC_CStringVector;
typedef std::set<MAKFC_CString> MAKFC_CStringSet;
typedef std::map<MAKFC_CString, MAKFC_CString>	MAPSTRSTR;
typedef std::map<MAKFC_CString, MAPSTRSTR>		MAPSTR_MAPSTR;

template <class _InIt>
inline size_t _Hash_value(_InIt _Begin, _InIt _End)
{	// hash range of elements
	size_t _Val = 2166136261U;
	while(_Begin != _End)
		_Val = 16777619U * _Val ^ (size_t)*_Begin++;
	return (_Val);
}


#pragma pack(push, 1)

class MAKFC_CString : public CAtlString
{
	mutable LPBYTE m_pNetData;
public:
	static const MAKFC_CString EmptyString;
	static const MAKFC_CString DefaultDelimeter;

	enum TrimDirection
	{
		TrimFromRight=-1,
		TrimLeftRight=0,
		TrimFromLeft=1,

	};
public:

	MAKFC_CString();
	~MAKFC_CString();

	MAKFC_CString( _In_opt_z_ const XCHAR* pszSrc );
	MAKFC_CString( const MAKFC_CString& strSrc );
	MAKFC_CString( _In_ const CAtlString& strSrc );
	CSTRING_EXPLICIT MAKFC_CString( _In_opt_z_ const YCHAR* pszSrc );
	explicit MAKFC_CString( IAtlStringMgr* pStringMgr ) throw();
	MAKFC_CString( _In_opt_z_ LPCSTR pszSrc, _In_ IAtlStringMgr* pStringMgr );
	explicit MAKFC_CString( std::wstring &s ) throw();
		
	MAKFC_CString& operator=( _In_ const MAKFC_CString& strSrc );
	MAKFC_CString& operator=( _In_ const CAtlString& strSrc );
	MAKFC_CString& operator=( _In_opt_z_ PCXSTR pszSrc );
	MAKFC_CString& operator=( _In_opt_z_ PCYSTR pszSrc );
	MAKFC_CString& operator=( _In_ const std::wstring& strSrc );

	MAKFC_CString& operator+=( _In_ const CAtlString& str );
	MAKFC_CString& operator+=( _In_ const MAKFC_CString& str );
	MAKFC_CString& operator+=( _In_opt_z_ PCXSTR pszSrc );
	MAKFC_CString& operator+=( _In_opt_z_ PCYSTR pszSrc );
	MAKFC_CString& operator+=( _In_ char ch );
	MAKFC_CString& operator+=( _In_ unsigned char ch );
	MAKFC_CString& operator+=( _In_ wchar_t ch );
	
	template< int t_nSize >
	MAKFC_CString& operator+=( _In_ const CStaticString< XCHAR, t_nSize >& strSrc )
	{
		CAtlString::operator+=( strSrc );

		return( *this );
	}

	friend MAKFC_CString operator+( _In_ const MAKFC_CString& str1, _In_ const MAKFC_CString& str2 )
	{
		MAKFC_CString strResult( str1.GetManager() );

		Concatenate( strResult, str1, str1.GetLength(), str2, str2.GetLength() );

		return( strResult );
	}

	friend MAKFC_CString operator+( _In_ const CAtlString& str1, _In_ const MAKFC_CString& str2 )
	{
		MAKFC_CString strResult( str1.GetManager() );

		Concatenate( strResult, str1, str1.GetLength(), str2, str2.GetLength() );

		return( strResult );
	}

	friend MAKFC_CString operator+( _In_ const MAKFC_CString& str1, _In_ const CAtlString& str2 )
	{
		MAKFC_CString strResult( str1.GetManager() );

		Concatenate( strResult, str1, str1.GetLength(), str2, str2.GetLength() );

		return( strResult );
	}

	friend MAKFC_CString operator+( _In_ const MAKFC_CString& str1, _In_ PCXSTR psz2 )
	{
		MAKFC_CString strResult( str1.GetManager() );

		Concatenate( strResult, str1, str1.GetLength(), psz2, StringLength( psz2 ) );

		return( strResult );
	}

	friend MAKFC_CString operator+( _In_ PCXSTR psz1, _In_ const MAKFC_CString& str2 )
	{
		MAKFC_CString strResult( str2.GetManager() );

		Concatenate( strResult, psz1, StringLength( psz1 ), str2, str2.GetLength() );

		return( strResult );
	}

	friend MAKFC_CString operator+( _In_ const MAKFC_CString& str1, _In_ wchar_t ch2 )
	{
		MAKFC_CString strResult( str1.GetManager() );
		XCHAR chTemp = XCHAR( ch2 );

		Concatenate( strResult, str1, str1.GetLength(), &chTemp, 1 );

		return( strResult );
	}

	friend MAKFC_CString operator+( _In_ const MAKFC_CString& str1, _In_ char ch2 )
	{
		MAKFC_CString strResult( str1.GetManager() );
		XCHAR chTemp = XCHAR( ch2 );

		Concatenate( strResult, str1, str1.GetLength(), &chTemp, 1 );

		return( strResult );
	}

	friend MAKFC_CString operator+( _In_ wchar_t ch1, _In_ const MAKFC_CString& str2 )
	{
		MAKFC_CString strResult( str2.GetManager() );
		XCHAR chTemp = XCHAR( ch1 );

		Concatenate( strResult, &chTemp, 1, str2, str2.GetLength() );

		return( strResult );
	}

	friend MAKFC_CString operator+( _In_ char ch1, _In_ const MAKFC_CString& str2 )
	{
		MAKFC_CString strResult( str2.GetManager() );
		XCHAR chTemp = XCHAR( ch1 );

		Concatenate( strResult, &chTemp, 1, str2, str2.GetLength() );

		return( strResult );
	}

	friend bool operator==( _In_ const MAKFC_CString& str1, _In_ const MAKFC_CString& str2 ) throw()
	{
		return( str1.Compare( str2 ) == 0 );
	}

	friend bool operator==( _In_ const MAKFC_CString& str1, _In_ const CAtlString& str2 ) throw()
	{
		return( str1.Compare( str2 ) == 0 );
	}

	friend bool operator==( _In_ const CAtlString& str1, _In_ const MAKFC_CString& str2 ) throw()
	{
		return( str1.Compare( str2 ) == 0 );
	}

	friend bool operator==( _In_ const std::string& str1, _In_ const MAKFC_CString& str2 ) throw()
	{
		CAtlString altStr1(str1.c_str());

		return( altStr1 == str2 );
	}

	friend bool operator==( _In_ const MAKFC_CString& str1, _In_ const std::string& str2 ) throw()
	{
		CAtlString altStr2(str2.c_str());

		return( str1 == altStr2 );
	}

	
	friend bool operator==(	_In_ const MAKFC_CString& str1, _In_ PCXSTR psz2 ) throw()
	{
		return( str1.Compare( psz2 ) == 0 );
	}

	friend bool operator==(
		_In_ PCXSTR psz1, _In_ const MAKFC_CString& str2 ) throw()
	{
		return( str2.Compare( psz1 ) == 0 );
	}

	friend bool operator==(	_In_ const MAKFC_CString& str1, _In_ PCYSTR psz2 ) throw( ... )
	{
		MAKFC_CString str2( psz2, str1.GetManager() );

		return( str1 == str2 );
	}

	friend bool operator==(	_In_ PCYSTR psz1, _In_ const MAKFC_CString& str2 ) throw( ... )
	{
		MAKFC_CString str1( psz1, str2.GetManager() );

		return( str1 == str2 );
	}

	friend bool operator!=(	_In_ const MAKFC_CString& str1, _In_ const MAKFC_CString& str2 ) throw()
	{
		return( str1.Compare( str2 ) != 0 );
	}

	friend bool operator!=(	_In_ const MAKFC_CString& str1, _In_ const CAtlString& str2 ) throw()
	{
		return( str1.Compare( str2 ) != 0 );
	}

	friend bool operator!=(	_In_ const CAtlString& str1, _In_ const MAKFC_CString& str2 ) throw()
	{
		return( str1.Compare( str2 ) != 0 );
	}

	friend bool operator!=(	_In_ const std::string& str1, _In_ const MAKFC_CString& str2 ) throw()
	{
		CAtlString altStr1(str1.c_str());

		return( altStr1 != str2 );
	}

	friend bool operator!=(	_In_ const MAKFC_CString& str1, _In_ const std::string& str2 ) throw()
	{
		CAtlString altStr2(str2.c_str());

		return( str1 != altStr2 );
	}

	friend bool operator!=(	_In_ const MAKFC_CString& str1, _In_ PCXSTR psz2 ) throw()
	{
		return( str1.Compare( psz2 ) != 0 );
	}

	friend bool operator!=(	_In_ const CAtlString& str1, _In_ PCXSTR psz2 ) throw()
	{
		return( str1.Compare( psz2 ) != 0 );
	}

	friend bool operator!=(	_In_ PCXSTR psz1, _In_ const MAKFC_CString& str2 ) throw()
	{
		return( str2.Compare( psz1 ) != 0 );
	}

	friend bool operator!=(	_In_ PCXSTR psz1, _In_ const CAtlString& str2 ) throw()
	{
		return( str2.Compare( psz1 ) != 0 );
	}

	friend bool operator!=(	_In_ const MAKFC_CString& str1, _In_ PCYSTR psz2 ) throw( ... )
	{
		MAKFC_CString str2( psz2, str1.GetManager() );

		return( str1 != str2 );
	}

	friend bool operator!=(	_In_ const CAtlString& str1, _In_ PCYSTR psz2 ) throw( ... )
	{
		MAKFC_CString str2( psz2, str1.GetManager() );

		return( str1 != str2 );
	}

	friend bool operator!=(	_In_ PCYSTR psz1, _In_ const MAKFC_CString& str2 ) throw( ... )
	{
		MAKFC_CString str1( psz1, str2.GetManager() );

		return( str1 != str2 );
	}

	friend bool operator!=(	_In_ PCYSTR psz1, _In_ const CAtlString& str2 ) throw( ... )
	{
		MAKFC_CString str1( psz1, str2.GetManager() );

		return( str1 != str2 );
	}

	friend bool operator<( _In_ const MAKFC_CString& str1, _In_ const MAKFC_CString& str2 ) throw()
	{
		return( str1.Compare( str2 ) < 0 );
	}

	friend bool operator<( _In_ const MAKFC_CString& str1, _In_ const CAtlString& str2 ) throw()
	{
		return( str1.Compare( str2 ) < 0 );
	}

	friend bool operator<( _In_ const CAtlString& str1, _In_ const MAKFC_CString& str2 ) throw()
	{
		return( str1.Compare( str2 ) < 0 );
	}


	friend bool operator<( _In_ const MAKFC_CString& str1, _In_ PCXSTR psz2 ) throw()
	{
		return( str1.Compare( psz2 ) < 0 );
	}

	friend bool operator<( _In_ PCXSTR psz1, _In_ const MAKFC_CString& str2 ) throw()
	{
		return( str2.Compare( psz1 ) > 0 );
	}

	friend bool operator>( _In_ const MAKFC_CString& str1, _In_ const MAKFC_CString& str2 ) throw()
	{
		return( str1.Compare( str2 ) > 0 );
	}

	friend bool operator>( _In_ const MAKFC_CString& str1, _In_ const CAtlString& str2 ) throw()
	{
		return( str1.Compare( str2 ) > 0 );
	}

	friend bool operator>( _In_ const CAtlString& str1, _In_ const MAKFC_CString& str2 ) throw()
	{
		return( str1.Compare( str2 ) > 0 );
	}

	friend bool operator>( _In_ const MAKFC_CString& str1, _In_ PCXSTR psz2 ) throw()
	{
		return( str1.Compare( psz2 ) > 0 );
	}

	friend bool operator>( _In_ PCXSTR psz1, _In_ const MAKFC_CString& str2 ) throw()
	{
		return( str2.Compare( psz1 ) < 0 );
	}

	friend bool operator<=( _In_ const MAKFC_CString& str1, _In_ const MAKFC_CString& str2 ) throw()
	{
		return( str1.Compare( str2 ) <= 0 );
	}

	friend bool operator<=( _In_ const CAtlString& str1, _In_ const MAKFC_CString& str2 ) throw()
	{
		return( str1.Compare( str2 ) <= 0 );
	}

	friend bool operator<=( _In_ const MAKFC_CString& str1, _In_ const CAtlString& str2 ) throw()
	{
		return( str1.Compare( str2 ) <= 0 );
	}

	friend bool operator<=( _In_ const MAKFC_CString& str1, _In_ PCXSTR psz2 ) throw()
	{
		return( str1.Compare( psz2 ) <= 0 );
	}

	friend bool operator<=( _In_ PCXSTR psz1, _In_ const MAKFC_CString& str2 ) throw()
	{
		return( str2.Compare( psz1 ) >= 0 );
	}

	friend bool operator>=( _In_ const MAKFC_CString& str1, _In_ const MAKFC_CString& str2 ) throw()
	{
		return( str1.Compare( str2 ) >= 0 );
	}

	friend bool operator>=( _In_ const CAtlString& str1, _In_ const MAKFC_CString& str2 ) throw()
	{
		return( str1.Compare( str2 ) >= 0 );
	}

	friend bool operator>=( _In_ const MAKFC_CString& str1, _In_ const CAtlString& str2 ) throw()
	{
		return( str1.Compare( str2 ) >= 0 );
	}

	friend bool operator>=( _In_ const MAKFC_CString& str1, _In_ PCXSTR psz2 ) throw()
	{
		return( str1.Compare( psz2 ) >= 0 );
	}

	friend bool operator>=( _In_ PCXSTR psz1, _In_ const MAKFC_CString& str2 ) throw()
	{
		return( str2.Compare( psz1 ) <= 0 );
	}

	friend bool operator==( _In_ XCHAR ch1, _In_ const MAKFC_CString& str2 ) throw()
	{
		return( (str2.GetLength() == 1) && (str2[0] == ch1) );
	}

	friend bool operator==( _In_ const MAKFC_CString& str1, _In_ XCHAR ch2 ) throw()
	{
		return( (str1.GetLength() == 1) && (str1[0] == ch2) );
	}

	friend bool operator!=( _In_ XCHAR ch1, _In_ const MAKFC_CString& str2 ) throw()
	{
		return( (str2.GetLength() != 1) || (str2[0] != ch1) );
	}

	friend bool operator!=( _In_ const MAKFC_CString& str1, _In_ XCHAR ch2 ) throw()
	{
		return( (str1.GetLength() != 1) || (str1[0] != ch2) );
	}

	int Len() const throw();
	bool IsEmpty() const throw() { return !Len(); }

	const BYTE* NetA( UINT uiCodePage = CP_ACP ) const;
	const BYTE* NetW( UINT uiCodePage = CP_ACP ) const;
	const BYTE* NetW_BE( UINT uiCodePage = CP_ACP ) const;
	const BYTE* Net() const;
	const char* NetStrA( UINT uiCodePage = CP_ACP ) const;
	const wchar_t* NetStrW( UINT uiCodePage = CP_ACP ) const;
	const wchar_t* NetStrW_BE( UINT uiCodePage = CP_ACP ) const;

	int NetSizeA( UINT uiCodePage = CP_ACP ) const;
	int NetSizeW(UINT uiCodePage = CP_ACP ) const;
	int NetSize() const;

	void SetAtA( int iIndex, char c );
	void SetAtW( int iIndex, wchar_t c );
		
	BYTE* NetGetBufferA( int iSize );
	void NetReleaseBufferA( UINT uiCodePage = CP_ACP );

	BYTE * NetGetBufferW( int iSize );
	void NetReleaseBufferW( UINT uiCodePage = CP_ACP );

	const wchar_t* Duplicate() const;

	static MAKFC_CString ToString( int i, MAKFC_CString stFormat = _T( "%d" ) );
	static MAKFC_CString ToString( const MAKFC_CStringVector & Vector, MAKFC_CString stDelimeter = DefaultDelimeter );
	static MAKFC_CString ToString( const SYSTEMTIME&, BOOL bDate = TRUE, BOOL bTime = TRUE );
	static MAKFC_CString ToString( LPCTSTR szString );
	static MAKFC_CString ToString64( __int64 i, BOOL bHex = FALSE, BOOL bSigned = TRUE);
	static MAKFC_CString CreateTiledString( TCHAR c, UINT uiLength );

	template<class T>
	static void Join(const T& container, const MAKFC_CString delimiter, OUT MAKFC_CString &out);

	MAKFC_CString Trim( const MAKFC_CString& sChars = _T(" "), int dir = TrimLeftRight ) const;
	MAKFC_CString& StripWhitespace();

	void SplitByDelimeter(IN const MAKFC_CString& delim, OUT std::vector<MAKFC_CString>& result, IN bool bEmptyResultsAllowed = false) const;
	

	void LoadFromUTF8( LPCSTR szUTF8String, int nLen );
	void LoadFromA( LPCSTR szAString, int nLen, UINT uiCodePage = CP_ACP );
#ifdef _UNICODE
	void LoadFromUSC2BE( LPCWSTR szBEString, int nLen );
#endif //_UNICODE


	int FindI( MAKFC_CString sToFind, int iFrom = 0 )  const;
	unsigned int CountOf( const MAKFC_CString& s );
			
	int Serialize( MAKFC_CInputStream* is );
	int Serialize( MAKFC_COutputStream* is );

	MAKFC_CString& MakeUpper();
	MAKFC_CString& MakeLower();
	int ToNumber(BOOL bSigned = TRUE, int nBase = 10) const;
	__int64 ToNumber64(BOOL bSigned = TRUE, int nBase = 10) const;

	void ToHexArray(std::vector<unsigned char> &out);
	void FromHexArray(const std::vector<unsigned char> &inArr);

	bool ToBoolean() const throw();

	static bool IsUin( LPCWSTR lpStr );
	
	friend inline size_t hash_value( _In_z_ MAKFC_CString _Str )
	{	// hash NTBS to size_t value
		int nLength = _Str.GetLength();
		PXSTR pszBuffer = _Str.GetBuffer( nLength );

	//	if ( !_Str.m_pchData )
	//		return stdext::_Hash_value( &_Str.m_chNull, &_Str.m_chNull);

		size_t hash = _Hash_value( pszBuffer, pszBuffer + nLength );

		_Str.ReleaseBufferSetLength( nLength );

		return hash;
	}

	int AppendChar(XCHAR ch)
	{
		int l = GetLength();
		return Insert(l, ch);
	}

	bool WriteToFile(MAKFC_CString fileName);

	//ещё один способ замены для интернационализации
	//пример использования
	//MAKFC_CString a = L"try to use ###NAME1### with ###NAME2###";
	//a.Subst(_T("###NAME1###"), _T("stuff1")).Subst(_T("###NAME2###"), _T("stuff2"));
	//если заменить нужно только один параметр - желательно пользоваться только им

	MAKFC_CString& Subst(const MAKFC_CString source, const MAKFC_CString dest);
	MAKFC_CString& Subst(const MAKFC_CString& source, int dest);
	MAKFC_CString& Subst(const MAKFC_CString& source, const std::string& dest);
	MAKFC_CString& Subst(const MAKFC_CString& source, const std::wstring& dest);
	MAKFC_CString& SubstTime(const MAKFC_CString& source, const SYSTEMTIME& dest);

	bool EndsWith(const MAKFC_CString &s) const;

    bool StartsWith(const MAKFC_CString &s) const;

    void ReplaceFirst(const MAKFC_CString &oldString, const MAKFC_CString &newString);
	void ReplaceLast(const MAKFC_CString &oldString, const MAKFC_CString &newString);
	void ReplaceRegex(const wchar_t *pattern, const wchar_t *replacement);
	int FindRegex(const int startPos, const wchar_t *pattern, OUT std::wcmatch &match) const;
	int FindRegex(const int startPos, const wchar_t *pattern) const;

	MAKFC_CString GetMd5Hash() const;

	int LastIndexOf(const MAKFC_CString& s) const;

	MAKFC_CString Ellipsis(const int length, const MAKFC_CString &trail = L"...") const;
};

template<class T>
void MAKFC_CString::Join(const T& container, const MAKFC_CString delimiter, OUT MAKFC_CString &out)
{
	OUT out.Empty();

	for (const auto &str : container)
	{
		out += str;
		out += delimiter;
	}

	out.ReplaceLast(delimiter, L"");
}

#pragma pack(pop)

struct less_MAKFC_CString
{
   bool operator()(const MAKFC_CString & x, const MAKFC_CString & y) const
   {
      if ( x < y )
         return true;

      return false;
   }
};

struct less_MAKFC_LPCTSTR
{
   bool operator()(const LPCTSTR & x, const LPCTSTR & y) const
   {
      if ( _tcscmp(x, y) < 0 )
         return true;

      return false;
   }
};

//tool string functions

//type = 0 - from both sides
//type = 1 - from beginning
//type = -1 - from end
LPTSTR strtrim(LPTSTR src, LPCTSTR chars, int type);
LPSTR strtrimA(LPSTR src, LPCSTR chars, int type);
//find substring first from the end
LPTSTR strrstr(LPCTSTR str, LPCTSTR to_find);
LPTSTR strins(LPTSTR str, LPCTSTR to_ins, int pos);
int	strrep_static(const char *src, char *dst, const char *to_find, const char *to_replace, BOOL bGetSize);
int	strrep_static(LPTSTR src, LPCTSTR to_find, LPCTSTR to_replace, BOOL bGetSize);
void GetStrX(LPTSTR xorstr, char xor_char);
int safe_strlen(LPTSTR str, int max_len);
int STR_strnicmp_rus(const TCHAR *str1, const TCHAR *str2, int len);
int STR_stricmp_rus(const TCHAR *str1, const TCHAR *str2);
BOOL makfc_STR_IsConvertableTo1251(LPCTSTR szString);
BOOL makfc_STR_PutToClipboard(MAKFC_CString stBuf, HWND hwnd = NULL);
BOOL makfc_STR_GetFromClipboard(MAKFC_CString &sText, HWND hWnd = NULL);

//some path functions
LPTSTR FileNameFromPath(LPCTSTR path);
char * FileNameFromPathA(const char *path);
MAKFC_CString PATH_GetLaunchDirectory();

//some email functions
int GetLoginDomain(const MAKFC_CString& stEmail, MAKFC_CString& stLogin, MAKFC_CString& stDomain);
MAKFC_CString GetLoginFromEmail(const MAKFC_CString& stEmail);

MAKFC_CString GetEmailFromLoginDomain(const MAKFC_CString &login, const MAKFC_CString &domain);

int makfc_memfind(const void *lpSrc, int iSrcSize, const void *lpToFind, int iFindSize);
//BOOL makfc_IsRussian();

#define STR_ITEM_FIRST					0
#define STR_ITEM_LAST					-1

MAKFC_CString STR_ItemString_Insert(MAKFC_CString sString, MAKFC_CString sItem, MAKFC_CString sDelimiter, int iPos);
MAKFC_CString STR_ItemString_GetItem(MAKFC_CString sString, MAKFC_CString sDelimiter, int iPos);
int STR_ItemString_GetItemIndex(MAKFC_CString sString, MAKFC_CString sDelimiter, MAKFC_CString sItem);
MAKFC_CString STR_ItemString_Remove(MAKFC_CString sString, MAKFC_CString sDelimiter, int iPos);
int STR_ItemString_GetItemQuantity(MAKFC_CString sString, MAKFC_CString sDelimiter);

MAKFC_CString STR_GetHTTPParamValue(MAKFC_CString sHTTPResponse, MAKFC_CString sParamName);
MAKFC_CString STR_GetMailParamValue(MAKFC_CString sHTTPResponse, MAKFC_CString sParamName);

void XorData(BYTE *lpDst, const BYTE *lpSrc, int iSrcSize, const BYTE *lpXor, int iXorSize);
MAKFC_CString EscapeData(const TCHAR *lpData, int iSize);
MAKFC_CString EscapeData2(const TCHAR *lpDataW, int iSize);
MAKFC_CString EscapeData3(const TCHAR *lpDataW, int iSize);
MAKFC_CString UnEscapeData(const TCHAR *lpData, int iSize);
MAKFC_CString EscapeDataA(const char *lpData, int iSize);
MAKFC_CString UnEscapeDataA(const char *lpData, int iSize);
MAKFC_CString GetMD5Hex(const char* lpBuffer);
MAKFC_CString EscapeString(const MAKFC_CString& sString);
MAKFC_CString EscapeJsonString(const MAKFC_CString& sString);


int isenglish(TCHAR c);
int isrussian(TCHAR c);
int isrussianA(char c);
int isemail(TCHAR c);
int isemailname(TCHAR c);
int isemaildomain(TCHAR c);
BOOL makfc_IsEmail(LPCTSTR szStr);
BOOL makfc_IsPhone(MAKFC_CString sPhone);

BOOL IsEmptyGuid(GUID guid);
GUID GetEmptyGuid();

const std::wstring LoadWstringFromUtf8Buf(const char *buf, const int len);