#include "stdafx.h"
#include "string.h"
#include "md5.h"
#include "stream.h"

const MAKFC_CString MAKFC_CString::EmptyString;
const MAKFC_CString MAKFC_CString::DefaultDelimeter = L",";

MAKFC_CString::MAKFC_CString()
	: m_pNetData(NULL)
{
}

MAKFC_CString::MAKFC_CString( _In_opt_z_ const XCHAR* pszSrc )
	: CAtlString( pszSrc ),
	  m_pNetData( NULL )
{
}

MAKFC_CString::MAKFC_CString( IAtlStringMgr* pStringMgr ) throw()
	: CAtlString( pStringMgr ),
	  m_pNetData( NULL )
{
}

MAKFC_CString::MAKFC_CString( _In_opt_z_ LPCSTR pszSrc, _In_ IAtlStringMgr* pStringMgr )
	: CAtlString( pszSrc, pStringMgr ),
	  m_pNetData( NULL )
{
}

MAKFC_CString::MAKFC_CString( _In_ const CAtlString& strSrc )
	: CAtlString( strSrc ),
	  m_pNetData( NULL )
{
}

MAKFC_CString::MAKFC_CString( const MAKFC_CString& strSrc )
	: CAtlString( strSrc ),
	  m_pNetData( NULL )
{
}

CSTRING_EXPLICIT MAKFC_CString::MAKFC_CString( _In_opt_z_ const YCHAR* pszSrc )
	: CAtlString( pszSrc ),
	  m_pNetData( NULL )
{
}

CSTRING_EXPLICIT MAKFC_CString::MAKFC_CString( std::wstring &s ) throw()
	:	CAtlString( s.c_str() ),
		m_pNetData( nullptr )
{
}

MAKFC_CString::~MAKFC_CString()
{
	free( m_pNetData );
}


MAKFC_CString& MAKFC_CString::operator+=( _In_ const CAtlString& str )
{
	CAtlString::operator+=( str );

	return( *this );
}

MAKFC_CString& MAKFC_CString::operator+=( _In_ const MAKFC_CString& str )
{
	CAtlString::operator+=( str );

	return( *this );
}


MAKFC_CString& MAKFC_CString::operator+=( _In_opt_z_ PCXSTR pszSrc )
{
	CAtlString::operator+=( pszSrc );

	return( *this );
}

MAKFC_CString& MAKFC_CString::operator+=( _In_opt_z_ PCYSTR pszSrc )
{
	MAKFC_CString str( pszSrc, GetManager() );

	return( operator+=( str ) );
}

MAKFC_CString& MAKFC_CString::operator+=( _In_ char ch )
{
	CAtlString::operator+=( ch );

	return( *this );
}

MAKFC_CString& MAKFC_CString::operator+=( _In_ unsigned char ch )
{
	CAtlString::operator+=( ch );

	return( *this );
}

MAKFC_CString& MAKFC_CString::operator+=( _In_ wchar_t ch )
{
	CAtlString::operator+=( ch );

	return( *this );
}


MAKFC_CString& MAKFC_CString::operator=( _In_ const MAKFC_CString& strSrc )
{
	CAtlString::operator=( strSrc );
	free(m_pNetData);
	m_pNetData = NULL;

	return( *this );
}

MAKFC_CString& MAKFC_CString::operator=( _In_ const CAtlString& strSrc )
{
	CAtlString::operator=( strSrc );

	return( *this );
}

MAKFC_CString& MAKFC_CString::operator=( _In_opt_z_ PCXSTR pszSrc )
{
	CAtlString::operator=( pszSrc );

	return( *this );
}

MAKFC_CString& MAKFC_CString::operator=( _In_opt_z_ PCYSTR pszSrc )
{
	CAtlString::operator=( pszSrc );
	
	return( *this );
}

MAKFC_CString& MAKFC_CString::operator=( _In_ const std::wstring& strSrc )
{
	CAtlString::operator=(strSrc.c_str());

	return( *this );
}
















int MAKFC_CString::Len() const throw()
{
	return GetLength();
}

const BYTE* MAKFC_CString::NetA( UINT uiCodePage ) const
{
	int nLength = GetLength();
	PXSTR pszBuffer = const_cast<MAKFC_CString *>(this)->GetBuffer( nLength );

	int iBytesNeeded = WideCharToMultiByte( uiCodePage, 0, pszBuffer, nLength + 1, NULL, 0, 0, NULL );

	free( m_pNetData );
	m_pNetData = NULL;

	if ( iBytesNeeded > 0 )
	{
		m_pNetData = (BYTE *) malloc( sizeof( BYTE ) * ( iBytesNeeded + 1 ) );
		
		iBytesNeeded = WideCharToMultiByte( uiCodePage, 0, pszBuffer, nLength + 1,
			(LPSTR) m_pNetData, iBytesNeeded, 0, NULL );
	}
	else
	{
		m_pNetData = (BYTE *) malloc( sizeof( BYTE ) );
		m_pNetData[0] = 0;
	}

	const_cast<MAKFC_CString *>(this)->ReleaseBufferSetLength( nLength );

	return m_pNetData;
}

const char* MAKFC_CString::NetStrA( UINT uiCodePage ) const
{
	return (const char*) NetA( uiCodePage );
}

const BYTE * MAKFC_CString::NetW(UINT) const
{
	return (const BYTE *) this->operator LPCWSTR();
}

const BYTE * MAKFC_CString::Net() const
{
#ifdef _NET_ANSI
	return NetA();
#else
	return NetW();
#endif //_NET_ANSI
}

const BYTE * MAKFC_CString::NetW_BE(UINT) const
{
	free( m_pNetData );
	m_pNetData = NULL;

	int nLength = GetLength();
	PXSTR pszBuffer = const_cast<MAKFC_CString *>(this)->GetBuffer( nLength );

	m_pNetData = (BYTE *) malloc( 2*( nLength + 1 ) );

	for ( int i = 0; i < ( nLength*2 ); i+=2 )
	{
		( (char*) m_pNetData )[i] = ( (char*) pszBuffer )[i+1];
		( (char*) m_pNetData )[i+1] = ( (char*) pszBuffer )[i];
	}

	( ( wchar_t* ) m_pNetData )[nLength] = L'\0';

	const_cast<MAKFC_CString *>(this)->ReleaseBufferSetLength( nLength );

	return m_pNetData;
}



const wchar_t* MAKFC_CString::NetStrW( UINT uiCodePage ) const
{
	return ( const wchar_t * ) NetW( uiCodePage );
}

const wchar_t* MAKFC_CString::NetStrW_BE( UINT uiCodePage ) const
{
	return ( const wchar_t* ) NetW_BE( uiCodePage );
}



int MAKFC_CString::NetSizeA( UINT uiCodePage ) const
{
	int nLength = GetLength();
	PXSTR pszBuffer = const_cast<MAKFC_CString *>(this)->GetBuffer( nLength );

	int iBytesNeeded = WideCharToMultiByte( uiCodePage, 0, pszBuffer, nLength, NULL, 0, 0, NULL );

	const_cast<MAKFC_CString *>(this)->ReleaseBufferSetLength( nLength );

	return iBytesNeeded;
}

int MAKFC_CString::NetSizeW(UINT) const
{
	return ( Len() ) * sizeof( wchar_t );
}


int MAKFC_CString::NetSize() const
{
#ifdef _NET_ANSI
	return NetSizeA();
#else
	return NetSizeW();
#endif //_NET_ANSI
}

void MAKFC_CString::SetAtA( int iIndex, char c )
{
	assert( m_pNetData );
	if (m_pNetData)
		*( (LPSTR) m_pNetData + iIndex ) = c;
}

void MAKFC_CString::SetAtW(int iIndex, wchar_t c)
{
	
	assert(m_pNetData);
	if (m_pNetData)
		*( (LPWSTR) m_pNetData + iIndex ) = c;
}


BYTE* MAKFC_CString::NetGetBufferA( int iSize )
{
	assert( iSize > 0 );
	if ( iSize <= 0 )
		return NULL;

	free( m_pNetData );

	m_pNetData = (BYTE *)calloc(1, iSize * sizeof(BYTE));

	//m_bGotBufferNet = TRUE;
	return m_pNetData;
}

void MAKFC_CString::NetReleaseBufferA( UINT uiCodePage )
{
	assert( m_pNetData );
	if ( !m_pNetData )
		return;

#ifdef _UNICODE
	
	int iNetLen = strlen( (LPCSTR) m_pNetData );

	int iBytesNeeded = MultiByteToWideChar( uiCodePage, 0, (LPCSTR) m_pNetData, iNetLen+1, NULL, 0 );

	if ( iBytesNeeded > 0 )
	{
		LPTSTR pchData = (wchar_t *) malloc( sizeof( TCHAR ) * ( iBytesNeeded + 1 ) );
		iBytesNeeded = MultiByteToWideChar(uiCodePage, 0, (LPCSTR) m_pNetData, iNetLen+1, pchData, iBytesNeeded );
		
		(*this) = pchData;

		free( pchData );
	}
	else
	{
		(*this) = _T( "" );
	}

#else
	(*this) = (LPCSTR) m_pNetData;
#endif //_UNICODE

	free( m_pNetData );
	m_pNetData = NULL;
}


BYTE * MAKFC_CString::NetGetBufferW( int iSize )
{
	assert( iSize > 0 );
	if ( iSize <= 0 )
		return NULL;

	if ( m_pNetData )
		free( m_pNetData );

	m_pNetData = (BYTE *)calloc(1, iSize * sizeof(BYTE));
	
	return m_pNetData;
}

const wchar_t* MAKFC_CString::Duplicate() const
{
	return _wcsdup(*this);
}

void MAKFC_CString::NetReleaseBufferW(UINT)
{
	assert( m_pNetData );
	if ( !m_pNetData )
		return;

	*this = (const wchar_t*) m_pNetData;


	free( m_pNetData );
	m_pNetData = NULL;

}



MAKFC_CString MAKFC_CString::ToString( int i, MAKFC_CString stFormat )
{
	MAKFC_CString str;
	str.Format( stFormat, i );
	
	return str;
}

MAKFC_CString MAKFC_CString::ToString64( __int64 i, BOOL bHex, BOOL bSigned )
{
	MAKFC_CString str;
	if ( bHex )
	{
		str.Format( _T( "%I64x" ), i );
	}
	else if(bSigned)
	{
		str.Format( _T( "%I64d" ), i );
	}
	else
	{
		str.Format( L"%I64u", i );
	}

	return str;
}

MAKFC_CString MAKFC_CString::ToString( const SYSTEMTIME& st, BOOL bDate, BOOL bTime )
{
	MAKFC_CString str;

	if ( bDate && bTime )
	{
		str.Format( _T( "%.2d.%.2d.%.2d %d:%.2d" ), st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute );
	}
	else if ( bDate )
	{
		str.Format( _T( "%.2d.%.2d.%.2d" ), st.wDay, st.wMonth, st.wYear );
	}
	else if ( bTime )
	{
		str.Format( _T( "%d:%.2d" ), st.wHour, st.wMinute );
	}

	return str;
}


MAKFC_CString MAKFC_CString::ToString( LPCTSTR szString )
{
	if ( !szString )
	{
		return _T( "" );
	}

	return MAKFC_CString( szString );
}

MAKFC_CString MAKFC_CString::ToString( const MAKFC_CStringVector & Vector, MAKFC_CString sDelimeter )
{
	MAKFC_CString sResult;
	Join(Vector, sDelimeter, sResult);
	return sResult;
}


MAKFC_CString MAKFC_CString::Trim( const MAKFC_CString& sChars, int dir ) const
{
	MAKFC_CString s = *this;

	switch ( dir )
	{
		case TrimFromLeft:
		{
			s.TrimLeft( sChars );
			break;
		}
		case TrimFromRight:
		{
			s.TrimRight( sChars );
			break;
		}
		case TrimLeftRight:
		{
			s.__super::Trim( sChars );
			break;
		}
	}

	return s;
}


void MAKFC_CString::LoadFromA( LPCSTR szAString, int nLen, UINT uiCodePage )
{
	assert( szAString );
	assert( nLen >= 0 );
	if ( !szAString || (nLen <= 0))
	{
		Empty();
		return;
	}

	const auto convertedCharsCount = ::MultiByteToWideChar( uiCodePage, 0, szAString, nLen, nullptr, 0 );
	assert(convertedCharsCount > 0);

	if (convertedCharsCount <= 0)
	{
		Empty();
		return;
	}

	std::vector<wchar_t> buf(convertedCharsCount, L'\0');
	::MultiByteToWideChar( uiCodePage, 0, szAString, nLen, &buf[0], convertedCharsCount );
	SetString(&buf[0], convertedCharsCount);
}

void MAKFC_CString::SplitByDelimeter(IN const MAKFC_CString& delim, OUT std::vector<MAKFC_CString>& result, IN bool bEmptyResultsAllowed) const
{
	assert(delim.Len() > 0);
	assert(result.empty());

	if (delim.Len() <= 0)
		return;

	if (Len() <= 0)
		return;

	int oldPos = 0;
	int pos = Find(delim, oldPos);

	while (pos != -1)
	{
		MAKFC_CString tmp = Mid(oldPos, pos - oldPos);
		if (tmp.Len() > 0 || bEmptyResultsAllowed)
		{
			result.push_back(tmp);
		}
		oldPos = pos + delim.Len();
		pos = Find(delim, oldPos);
	}
	MAKFC_CString tmp = Mid(oldPos);
	if (tmp.Len() > 0 || bEmptyResultsAllowed)
	{
		result.push_back(tmp);
	}

}



void MAKFC_CString::LoadFromUTF8( LPCSTR szUTF8String, int nLen )
{
	assert(szUTF8String);
	if ( !szUTF8String )
	{
		Empty();
		return;
	}

	LoadFromA( szUTF8String, nLen, CP_UTF8 );
}


#ifdef _UNICODE
void MAKFC_CString::LoadFromUSC2BE( LPCWSTR szBEString, int nLen )
{
	assert( szBEString );
	if ( !szBEString )
	{
		*this = _T( "" );
		return;
	}
	else
	{
		int iSize = ( nLen + 1 ) * sizeof(TCHAR);

		std::vector<char> buf(iSize);
		TCHAR* lpBuf = (TCHAR*) &buf[0];

		if ( iSize == sizeof( TCHAR ) )
		{
			lpBuf[0] = _T('\0');
		}
		else
		{
			for ( int i = 0; i < (nLen*2); i+=2 )
			{
				( (char*) lpBuf )[i] = ( (char*) szBEString )[i+1];
				( (char*) lpBuf )[i+1] = ( (char*) szBEString )[i];
			}

			lpBuf[nLen] = _T('\0');
		}

		*this = lpBuf;
	}
}
#endif _UNICODE



int MAKFC_CString::FindI( MAKFC_CString sToFind, int iFrom ) const
{
	if ( iFrom >= GetLength() )
		return -1;
	
	MAKFC_CString s1(*this);
	MAKFC_CString s2(sToFind);

	s1.MakeUpper();
	s2.MakeUpper();
	
	return s1.Find( s2, iFrom );
}

unsigned int MAKFC_CString::CountOf( const MAKFC_CString& s )
{
	unsigned int cnt = 0;
	int from = 0;

	while ( ( from = Find( s, from ) ) >= 0 )
	{
		from += s.Len();
		cnt++;
	}

	return cnt;
}


int MAKFC_CString::Serialize( MAKFC_CInputStream *is )
{
	int bRead = FALSE;
	TCHAR c;

	int iSize = is->available() + sizeof( TCHAR );

	LPTSTR pchData = (LPTSTR) malloc( iSize );

	pchData[0] = _T('\0');
	int iPos = 0;
#ifdef _UNICODE
	while ( ( c = is->readShort( &bRead ) ) != 0 && bRead )
#else
	while ( ( c = is->readByte( &bRead ) ) != 0 && bRead )
#endif
	{
		if ( (int)(iPos*sizeof( TCHAR ) + sizeof( TCHAR ) * 2) > iSize )
		{
			iSize = 128 * sizeof( TCHAR ) + ( iPos * sizeof(TCHAR) + sizeof(TCHAR)*2);
			pchData = (LPTSTR)realloc( pchData, iSize );
		}

		pchData[iPos] = c;
		iPos++;
	}

	pchData[iPos] = _T('\0');

	*this = pchData;

	return iPos * sizeof(TCHAR);
}

int MAKFC_CString::Serialize( MAKFC_COutputStream *is )
{
	return is->write( (BYTE*)(LPCTSTR)(*this), Len() * sizeof(TCHAR));
}


LPTSTR strtrim(LPTSTR src, LPCTSTR chars, int type)
{
	LPTSTR p;
	if (!src || !chars || type < -1 || type > 1 || _tcslen(src) <= 0 || _tcslen(chars) <= 0)
		return src;
	if (type == 1 || type == 0)
	{
		p = src;
		while (*p && _tcschr(chars, *p))
			p++;
		memmove(src, p, sizeof(TCHAR) * (_tcslen(p)+1));
	}
	if (type == -1 || type == 0)
	{
		int iLen = _tcslen(src);
		if (iLen > 0)
		{
			p = src + iLen - 1;
			while (p >= src && *p && _tcschr(chars, *p))
				p--;
			*(p+1) = _T('\0');
		}
	}
	return src;
}

LPSTR strtrimA(LPSTR src, LPCSTR chars, int type)
{
	LPSTR p;
	if (!src || !chars || type < -1 || type > 1 || strlen(src) <= 0 || strlen(chars) <= 0)
		return src;
	if (type == 1 || type == 0)
	{
		p = src;
		while (*p && strchr(chars, *p))
			p++;
		memmove(src, p, strlen(p)+1);
	}
	if (type == -1 || type == 0)
	{
		int iLen = strlen(src);
		if (iLen > 0)
		{
			p = src + iLen - 1;
			while (p >= src && *p && strchr(chars, *p))
				p--;
			*(p+1) = _T('\0');
		}
	}
	return src;
}

LPTSTR strrstr(LPCTSTR str, LPCTSTR to_find)
{
	LPTSTR p1 = NULL, p = (LPTSTR)str;
	BOOL bFound = FALSE;
	while ((p1 = _tcsstr(p, to_find)) != 0)
	{
		p = p1+1;
		bFound = TRUE;
	}
	return bFound ? p-1 : NULL;
}

LPTSTR strins(LPTSTR str, LPCTSTR to_ins, int pos)
{
	int len_str, len_ins;
	if (pos >= (int)_tcslen(str))
		return NULL;
	len_str = _tcslen(str);
	len_ins = _tcslen(to_ins);
	memmove(str+pos+len_ins, str+pos, (len_str - pos + 1) * sizeof(TCHAR));
	len_str = _tcslen(str);
	memmove(str+pos, to_ins, len_ins * sizeof(TCHAR));
	return str;
}

int strrep_static(LPTSTR src, LPCTSTR to_find, LPCTSTR to_replace, BOOL bGetSize)
{
	LPTSTR p = src;
	int count = 0;
	int find_len = _tcslen(to_find);
	int replace_len = _tcslen(to_replace);
	int dif_len = replace_len - find_len;

	p = _tcsstr(src, to_find);
	while (p)
	{
		p = _tcsstr(p + find_len, to_find);
		count++;
	}
	if (bGetSize)
		return _tcslen(src) + count * dif_len;

	p = _tcsstr(src, to_find);
	while (p)
	{
		memmove(p + replace_len, p + find_len, (_tcslen(p + find_len)+1) * sizeof(TCHAR));
		memmove(p, to_replace, replace_len * sizeof(TCHAR));
		p = _tcsstr(p + replace_len, to_find);
	}
	return _tcslen(src);
}

void GetStrX(LPTSTR xorstr, char xor_char)
{
	int i = 0;
	TCHAR abc;
	while (1)
	{
		if (xorstr[i] == _T('\0'))
		{
			return;
		}
		abc = xorstr[i];
		abc ^= xor_char;
		xorstr[i] = abc;
		i++;
	}
}

int safe_strlen(LPTSTR str, int max_len)
{
	LPTSTR p;
	int len = 0;
	if (!str || max_len < 0)
		return -1;
	p = str;
	while (*p != _T('\0') && p - str < max_len)
		p++;
	len = (*p == _T('\0')) ? p - str : -1;
	return len;
}

//some path functions

LPTSTR FileNameFromPath(LPCTSTR path)
{
	LPCTSTR p;
	p = _tcsrchr(path, _T('\\'));
	if (p == NULL)
		return (LPTSTR)path;
	return (LPTSTR)p+1;
}
LPSTR FileNameFromPathA(LPCSTR path)
{
	LPCSTR p;
	p = strrchr(path, '\\');
	if (p == NULL)
		return (LPSTR)path;
	return (LPSTR)p+1;
}

MAKFC_CString PATH_GetLaunchDirectory()
{
	MAKFC_CString s;
	int size = 1024;
	GetModuleFileName(NULL, s.GetBuffer(size), size);
	s.ReleaseBuffer();
	while (s.Len() == size && size < 1000000)
	{
		size *= 2;
		GetModuleFileName(NULL, s.GetBuffer(size), size);
		s.ReleaseBuffer();
	}

	LPTSTR p = FileNameFromPath(s);
	if (p)
		*p = 0;
	return s.Trim(_T("\\"), MAKFC_CString::TrimFromRight);
}
//some email functions

int GetLoginDomain(const MAKFC_CString& stEmail, MAKFC_CString& stLogin, MAKFC_CString& stDomain)
{
	int nAt=stEmail.Find(_T('@'));
	if(nAt!=-1)
	{
		stLogin=stEmail.Mid(0,nAt);
		stDomain=stEmail.Mid(nAt+1);
	}
	else
	{
		stLogin=stEmail;
		stDomain=_T("");
	}

	return TRUE;
}

MAKFC_CString GetLoginFromEmail(const MAKFC_CString& stEmail)
{
	MAKFC_CString sLogin, sDomain;
	GetLoginDomain(stEmail, sLogin, sDomain);
	return sLogin;
}

MAKFC_CString GetEmailFromLoginDomain(const MAKFC_CString &login, const MAKFC_CString &domain)
{
	if (domain.IsEmpty())
		return login;

	MAKFC_CString sLogin, sDomain;
	GetLoginDomain(login, sLogin, sDomain);
	if (!sDomain.IsEmpty())
		return login; // login already included some domain
	
	return login + L"@" + domain;
}

int makfc_memfind(const void *lpSrc, int iSrcSize, const void *lpToFind, int iFindSize)
{
	if (iSrcSize <= 0 || iFindSize <= 0 || !lpSrc || !lpToFind)
		return -1;
	for (int i = 0; i < iSrcSize - iFindSize+1; i++)
	{
		if (memcmp((BYTE *)lpSrc + i, lpToFind, iFindSize) == 0)
			return i;
	}
	return -1;
}

bool MAKFC_CString::IsUin( LPCWSTR lpStr )
{
	while ( *lpStr != L'\0' )
	{
		if ( !_istdigit( *lpStr ) )
			return false;

		lpStr++;
	}

	return true;
}


MAKFC_CString STR_ItemString_GetItem(MAKFC_CString sString, MAKFC_CString sDelimiter, int iPos)
{
	MAKFC_CString sResult, sResultPrev;
	int iFrom, iTo, iNumber;
	iFrom = iTo = 0;
	iNumber = 0;
	do
	{
		iTo = sString.Find(sDelimiter, iFrom);
		if (iTo == -1)
			break;
		sResult = sString.Mid(iFrom, iTo - iFrom);
		if (iNumber == iPos)
			break;
		iFrom = iTo + sDelimiter.Len();
		iNumber++;
		sResultPrev = sResult;
	} while (1);
	if (iTo == -1 && (iPos == STR_ITEM_LAST || iPos == iNumber))
	{
		sResult = sString.Mid(iFrom);
//		sResult = sResultPrev;		
		if (iPos == 0 && sResult.Len() == 0)
			sResult = sString;
	}
	else if (iTo == -1)
	{
		sResult = _T("");
		if (iPos == 0 && sResult.Len() == 0)
			sResult = sString;
	}
	return sResult;
}
int STR_ItemString_GetItemIndex(MAKFC_CString sString, MAKFC_CString sDelimiter, MAKFC_CString sItem)
{
	MAKFC_CString sResult;
	int iIndex = -1;
	do
	{
		iIndex++;
		sResult = STR_ItemString_GetItem(sString, sDelimiter, iIndex);
	} while (sResult.Len() > 0 && sResult != sItem);
	if (sResult.Len() == 0)
		iIndex = -1;
	return iIndex;
}
MAKFC_CString STR_ItemString_Remove(MAKFC_CString sString, MAKFC_CString sDelimiter, int iPos)
{
	MAKFC_CString sResult, sResultPrev;
	int iFrom, iTo, iNumber, iFromPrev;
	iFrom = iTo = iFromPrev = 0;
	iNumber = 0;
	do
	{
		iTo = sString.Find(sDelimiter, iFrom);
		if (iTo == -1)
			break;
		sResult = sString.Mid(iFrom, iTo - iFrom);
		if (iNumber == iPos)
			break;
		iFromPrev = iFrom;
		iFrom = iTo + sDelimiter.Len();
		iNumber++;
		sResultPrev = sResult;
	} while (1);
	if (iTo > 0)
	{
		sResult = sString.Mid(0, iFrom);
		sResult += sString.Mid(iTo + sDelimiter.Len());
	}
	else
	{
		if (iPos == STR_ITEM_LAST || iPos == iNumber)
			sResult = sString.Mid(0, iFrom > 0 ? iFrom - sDelimiter.Len() : 0);
		else
			sResult = sString.Mid(0, iFromPrev > 0 ? iFromPrev - sDelimiter.Len() : 0);
	}
	return sResult;
}

int STR_ItemString_GetItemQuantity(MAKFC_CString sString, MAKFC_CString sDelimiter)
{
	int i = 0;
	while (STR_ItemString_GetItem(sString, sDelimiter, i).Len() > 0)
		i++;
	return i;
}

MAKFC_CString STR_GetHTTPParamValue(MAKFC_CString sHTTPResponse, MAKFC_CString sParamName)
{
	if (sHTTPResponse.Find(_T("\r\n\r\n")) != sHTTPResponse.Len() - 4)
		return _T("");
	int iColon, iLine = 0;
	MAKFC_CString sLine, sParam, sValue;
	do
	{
		sLine = STR_ItemString_GetItem(sHTTPResponse, _T("\r\n"), iLine);
		iLine++;
		iColon = sLine.Find(_T(":"));
		if (iColon != -1)
		{
			sParam = sLine.Mid(0, iColon);
			sParam = sParam.Trim( _T(" "));
			if (sParam == sParamName)
			{
				sValue = sLine.Mid(iColon+1);
				sValue = sValue.Trim(_T(" "));
				return sValue;
			}
		}
	} while (sLine.Len() > 0 && iLine < 1000);
	return _T("");
}

MAKFC_CString STR_GetMailParamValue(MAKFC_CString sHTTPResponse, MAKFC_CString sParamName)
{
	if (sHTTPResponse.Find(_T("\n\n")) == -1)
		return _T("");
	int iColon, iLine = 0;
	MAKFC_CString sLine, sParam, sValue;
	do
	{
		sLine = STR_ItemString_GetItem(sHTTPResponse, _T("\n"), iLine);
		if (sLine == _T(""))
			break;
		iLine++;
		iColon = sLine.Find(_T(":"));
		if (iColon != -1)
		{
			sParam = sLine.Mid(0, iColon);
			sParam = sParam.Trim(_T(" "));
			if (sParam == sParamName)
			{
				sValue = sLine.Mid(iColon+1);
				sValue = sValue.Trim(_T(" "));
				return sValue;
			}
		}
	} while (sLine.Len() > 0 && iLine < 1000);
	return _T("");
}


int STR_strnicmp_rus(const TCHAR *str1, const TCHAR *str2, int len)
{
#ifdef _UNICODE
	int iRet = CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE, str1, len, str2, len);
	if (iRet == CSTR_EQUAL)
		return 0;
	else if (iRet == CSTR_LESS_THAN)
		return -1;
	else
		return 1;
#endif
/*	int i, delta, delta_rus;
	TCHAR c1, c2;

	delta = (int)_T('a') - (int)_T('A');
	delta_rus = (int)_T('à') - (int)_T('À');

	for (i = 0; i < len; i++)
	{
		c1 = str1[i];
		c2 = str2[i];

		if (c1 >= _T('A') && c1 <= _T('Z'))
			c1 += (TCHAR)delta;
		else if (c1 >= _T('À') && c1 <= _T('ß'))
			c1 += (TCHAR)delta_rus;

		if (c2 >= _T('A') && c2 <= _T('Z'))
			c2 += (TCHAR)delta;
		else if (c2 >= _T('À') && c2 <= _T('ß'))
			c2 += (TCHAR)delta_rus;

		if (c1 < c2)
			return -1;
		else if (c1 > c2)
			return 1;
	}
	return 0;*/
}

int STR_stricmp_rus(const TCHAR *str1, const TCHAR *str2)
{
	int iRet = ::CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE, str1, _tcslen(str1), str2, _tcslen(str2));

	if (iRet == CSTR_EQUAL)
	{
		return 0;
	}

	if (iRet == CSTR_LESS_THAN)
	{
		return -1;
	}

	return 1;
}

BOOL makfc_STR_IsConvertableTo1251(LPCTSTR szString)
{
	MAKFC_CString s = szString;
	char *sz = _strdup(s.NetStrA(1251));
	strcpy((char *)s.NetGetBufferA(strlen(sz) + 1), sz);
	s.NetReleaseBufferA(1251);
	if (s == szString)
	{
		free(sz);
		return TRUE;
	}
	else
	{
		free(sz);
		return FALSE;
	}
}

BOOL makfc_STR_PutToClipboard(MAKFC_CString stBuf, HWND hwnd)
{
	if (!OpenClipboard(hwnd))
		return FALSE;
	EmptyClipboard();
	HGLOBAL hData = NULL;

	hData = GlobalAlloc(GMEM_MOVEABLE, (stBuf.GetLength()+1)*sizeof(wchar_t));
	if (hData)
	{
		LPWSTR szW = (LPWSTR)GlobalLock(hData);
		if (szW)
		{
			wcscpy(szW, stBuf.NetStrW());
			GlobalUnlock(hData);
			SetClipboardData(CF_UNICODETEXT, hData);
		}	  
	} 

	hData = GlobalAlloc(GMEM_MOVEABLE, (stBuf.GetLength()+1)*sizeof(char));
	if (hData)
	{
		char *szA = (char *)GlobalLock(hData);
		if (szA)
		{
			strcpy(szA, stBuf.NetStrA());
			GlobalUnlock(hData);
			SetClipboardData(CF_TEXT, hData);
		}
	} 
	
	hData = GlobalAlloc(GMEM_MOVEABLE, (stBuf.GetLength()+1)*sizeof(char));
	if (hData)
	{
		LPSTR szOEM = (LPSTR)GlobalLock(hData); 
		if (szOEM)
		{
			CharToOem(stBuf, szOEM);
			GlobalUnlock(hData);
			SetClipboardData(CF_OEMTEXT, hData);
		}
	}  
	CloseClipboard();
	return TRUE;
}

BOOL makfc_STR_GetFromClipboard(MAKFC_CString &sText, HWND hWnd)
{
	if (!OpenClipboard(hWnd))
		return FALSE;

	if ( HANDLE data = GetClipboardData( CF_UNICODETEXT ) )
	{
		wchar_t *szText = (wchar_t *)GlobalLock(data);
		if (szText)
		{
			sText = szText;
			GlobalUnlock(data);
		}
		CloseClipboard();
		return TRUE;	 
	}
	else if ( HANDLE data = GetClipboardData( CF_TEXT ) )
	{
		char *szText = (char *)GlobalLock(data);
		if (szText)
		{
			sText = szText;
			GlobalUnlock(data);
		}
		CloseClipboard();
		return TRUE;	 
	}
	else
	{
		CloseClipboard();
		return FALSE;	 
	}
}

MAKFC_CString& MAKFC_CString::MakeUpper()
{
	int nLength = GetLength();
	PXSTR pszBuffer = GetBuffer( nLength );
	CharUpper( pszBuffer );
	ReleaseBufferSetLength( nLength );

	return( *this );
}

MAKFC_CString& MAKFC_CString::MakeLower()
{
	int nLength = GetLength();
	PXSTR pszBuffer = GetBuffer( nLength );
	CharLower( pszBuffer );
	ReleaseBufferSetLength( nLength );

	return( *this );
}

int MAKFC_CString::ToNumber( BOOL bSigned, int nBase ) const
{
	if(bSigned)
	{
		return _ttoi((LPCTSTR)*this);
	}
	return _tcstoul((LPCTSTR)*this, NULL, nBase);
}

__int64 MAKFC_CString::ToNumber64( BOOL bSigned, int nBase ) const
{
	if(bSigned)
	{
		return _tcstoi64((LPCTSTR)*this, NULL, nBase);
	}
	return _tcstoui64((LPCTSTR)*this, NULL, nBase);
}

bool MAKFC_CString::ToBoolean() const throw()
{
	if (Len() == 0)
		return false;
	if (*this == L"1")
		return true;
	if (*this == L"0")
		return false;
	if (CompareNoCase(L"true") == 0)
		return true;
	if (CompareNoCase(L"false") == 0)
		return false;
	if (CompareNoCase(L"yes") == 0)
		return true;
	if (CompareNoCase(L"no") == 0)
		return false;
	return true;
}

MAKFC_CString& MAKFC_CString::StripWhitespace()
{
	__super::Trim(_T("\r\n"));			// cause overrided Trim() has "const" modifier. Fuck.
	Replace(_T("\r\n"), _T(" "));
	Replace(_T('\t'), _T(' '));
	Replace(_T('\r'), _T(' '));
	Replace(_T('\n'), _T(' '));
	__super::Trim(_T(" "));				// cause overrided Trim() has "const" modifier. Fuck.
	while(Find(L"  ")!=-1)
	{
		Replace(L"  ", L" ");
	}
	return *this;
}

void MAKFC_CString::ToHexArray(std::vector<unsigned char> &out)
{
	if (Len() % 2 != 0)
		return;

	int len = NetSizeA(CP_UTF8);
	const char *str = NetStrA(CP_UTF8);

	for (int i = 0; i < len; i += 2)
	{
		unsigned char ch1 = str[i], ch2 = str[i+1];
		int dig1 = 0, dig2 = 0;
		if(isdigit(ch1)) dig1 = ch1 - '0';
		else if(ch1>='A' && ch1<='F') dig1 = ch1 - 'A' + 10;
		else if(ch1>='a' && ch1<='f') dig1 = ch1 - 'a' + 10;
		if(isdigit(ch2)) dig2 = ch2 - '0';
		else if(ch2>='A' && ch2<='F') dig2 = ch2 - 'A' + 10;
		else if(ch2>='a' && ch2<='f') dig2 = ch2 - 'a' + 10;
		out.push_back((unsigned char)((dig1 << 4) | dig2));
	}
}

void MAKFC_CString::FromHexArray(const std::vector<unsigned char> &inArr)
{
	for (std::vector<unsigned char>::const_iterator it = inArr.begin(); it != inArr.end(); ++it)
	{
		char buf[16];
		sprintf(buf, "%.2x", *it);
		*this += buf;
	}
}

bool MAKFC_CString::WriteToFile(MAKFC_CString fileName)
{
	HANDLE hFile = ::CreateFile(fileName.NetStrW(), GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_READ, 0, 
		TRUNCATE_EXISTING, 
		FILE_ATTRIBUTE_NORMAL, 0);

	if ((hFile == INVALID_HANDLE_VALUE) && (GetLastError() == ERROR_FILE_NOT_FOUND))
	{
		hFile = ::CreateFile(fileName.NetStrW(), GENERIC_READ | GENERIC_WRITE, 
			FILE_SHARE_READ, 0, 
			CREATE_NEW, 
			FILE_ATTRIBUTE_NORMAL, 0);
	}

	if (hFile == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	DWORD written = 0;
	DWORD totalWritten = 0;
	DWORD totalLen = 2 * Len();

	bool result = !!::WriteFile(hFile, NetStrW(), totalLen, &written, 0);
	totalWritten += written;

	while (result && totalWritten < (DWORD)Len() )
	{
		result = !!::WriteFile(hFile, NetStrW() + totalWritten, 
			totalLen - totalWritten, &written, 0);
		totalWritten += written;
	}

	::CloseHandle(hFile);

	return result;
}

MAKFC_CString& MAKFC_CString::Subst(const MAKFC_CString source, const MAKFC_CString dest)
{
#ifndef _FINAL
	assert(!source.IsEmpty());
	assert(source.Find(L"####") == -1);
	assert(source.Find(L"###") != -1);
	assert(source.Find(L"_") == -1);
#endif

	Replace(source, dest);
	return (*this);
}

MAKFC_CString& MAKFC_CString::Subst(const MAKFC_CString& source, int dest)
{
#ifdef _DEBUG
	assert(!source.IsEmpty());
	assert(source.Find(L"####") == -1);
	assert(source.Find(L"###") != -1);
#endif

	Replace(source, MAKFC_CString::ToString(dest));
	return (*this);
}

MAKFC_CString& MAKFC_CString::Subst(const MAKFC_CString& source, const std::string& dest)
{
	return Subst(source, MAKFC_CString(dest.c_str()));
}

MAKFC_CString& MAKFC_CString::Subst(const MAKFC_CString& source, const std::wstring& dest)
{
	return Subst(source, MAKFC_CString(dest.c_str()));
}

MAKFC_CString& MAKFC_CString::SubstTime(const MAKFC_CString& source, const SYSTEMTIME& dest)
{
	MAKFC_CString destStr = MAKFC_CString::ToString(dest.wHour) + MAKFC_CString(_T(":"))
		+ MAKFC_CString::ToString(dest.wMinute, _T("%.2d"));
	return Subst(source, destStr);
}

bool MAKFC_CString::EndsWith(const MAKFC_CString &s) const
{
	if (IsEmpty() || s.IsEmpty())
	{
		return false;
	}

	const int pos = Find(s);
	const bool found = (pos != -1);
	const bool atEnd = (pos == (GetLength() - s.GetLength()));

	return found && atEnd;
}

bool MAKFC_CString::StartsWith(const MAKFC_CString &s) const
{
    if (IsEmpty() || s.IsEmpty())
    {
        return false;
    }

    return Find(s) == 0;
}

void MAKFC_CString::ReplaceFirst(const MAKFC_CString &oldString, const MAKFC_CString &newString)
{
	assert(!oldString.IsEmpty());
    if (oldString.IsEmpty())
    {
        return;
    }

    const int oldPos = Find(oldString);
    if (oldPos == -1)
    {
        return;
    }

    auto result = Mid(0, oldPos);
    result += newString;

    const int oldEnd = oldPos + oldString.Len();
    result += Mid(oldEnd);

    *this = result;
}

void MAKFC_CString::ReplaceLast(const MAKFC_CString &oldString, const MAKFC_CString &newString)
{
	assert(!oldString.IsEmpty());
	if (oldString.IsEmpty())
	{
		return;
	}

	const int oldPos = LastIndexOf(oldString);
	if (oldPos == -1)
	{
		return;
	}

	auto result = Mid(0, oldPos);
	result += newString;

	const int oldEnd = oldPos + oldString.Len();
	result += Mid(oldEnd);

	*this = result;
}

void MAKFC_CString::ReplaceRegex(const wchar_t *pattern, const wchar_t *replacement)
{
	assert(pattern);
	assert(::wcslen(pattern) > 0);
	assert(replacement);
	
	std::wstring thisStr = NetStrW();

	std::wregex regex(pattern);
	thisStr = std::regex_replace(thisStr, regex, replacement);

	SetString(thisStr.c_str());
}

int MAKFC_CString::FindRegex(const int startPos, const wchar_t *pattern, OUT std::wcmatch &match) const
{
	assert(pattern);
	assert(::wcslen(pattern) > 0);
	assert(match.empty());
	
	if (startPos >= Len())
	{
		return -1;
	}

	std::wregex regex(pattern);
	
	const auto from = (LPCWSTR)*this + startPos;
	const auto to = from + Len();

	std::regex_search(from, to, OUT match, regex, std::regex_constants::match_default);

	return match.empty() ? -1 : match.position();
}

int MAKFC_CString::FindRegex(const int startPos, const wchar_t *pattern) const
{
	std::wcmatch match;
	return FindRegex(startPos, pattern, OUT match);
}

MAKFC_CString MAKFC_CString::GetMd5Hash() const
{
	char md5[16] = { 0 };
	MAKFC_md5( NetStrA( 1251 ), NetSizeA( 1251 ), md5 );
	return GetMD5Hex(md5);
}

int MAKFC_CString::LastIndexOf(const MAKFC_CString& s) const
{
	int found = -1;
	int nextPos = 0;
	for (;;)
	{
		nextPos = Find(s, nextPos);
		if (nextPos == -1)
		{
			return found;
		}

		assert(found != nextPos);
		found = nextPos;
		++nextPos;
	}
}

MAKFC_CString MAKFC_CString::Ellipsis(const int length, const MAKFC_CString &trail) const
{
	const auto fitLength = (Len() < length);
	if (fitLength)
	{
		return *this;
	}

	const auto trailLength = trail.Len();
	assert(length > trailLength);
	if (length <= trailLength)
	{
		return EmptyString;
	}

	return (Left(length - trailLength) + trail);
}

void XorData(BYTE *lpDst, const BYTE *lpSrc, int iSrcSize, const BYTE *lpXor, int iXorSize)
{
	const BYTE *psrc = lpSrc;
	const BYTE *pxor = lpXor;
	BYTE *pdst = lpDst;
	for (int i = 0; i < iSrcSize; i++)
	{
		*pdst++ = *psrc++ ^ *pxor++;
		if (pxor - lpXor == iXorSize)
			pxor = lpXor;
	}
}

MAKFC_CString EscapeData(const TCHAR *lpDataW, int iSize)
{
	MAKFC_CString sRet;
	char szSymbol[1024];

	MAKFC_CString s = lpDataW;
	const char *lpData = s.NetStrA(1251);
	const char *p = lpData;

	while (p - lpData < iSize)
	{
#ifdef _UNICODE
#pragma message ("************************EscapeData: escapement still in ANSI 1251****************")
		if (isrussianA(*p) || *p < 0 || !isalpha(*p))
			sprintf(szSymbol, "%%%.2X", (BYTE)*p); //%uXXXX for unicode
		else
		{
			szSymbol[0] = *p;
			szSymbol[1] = _T('\0');
		}
#else
		_stprintf(szSymbol, _T("%%%.2X"), (TBYTE)*p);
#endif
		sRet += szSymbol;
		p++;
	}
	return sRet;
}

MAKFC_CString EscapeData2(const TCHAR *lpDataW, int iSize)
{
	MAKFC_CString sRet;
	char szSymbol[1024];

	MAKFC_CString s = lpDataW;
	const char *lpData = s.NetStrA(1251);
	const char *p = lpData;

	while (p - lpData < iSize)
	{
#ifdef _UNICODE
#pragma message ("************************EscapeData2: escapement still in ANSI 1251****************")
		if ( ( isrussianA(*p) || !isalpha(*p) ) && !isdigit(*p) && !strchr( "-.", *p ) )
			sprintf(szSymbol, "%%%.2X", (BYTE)*p); //%uXXXX for unicode
		else
		{
			szSymbol[0] = *p;
			szSymbol[1] = _T('\0');
		}
#else
		_stprintf(szSymbol, _T("%%%.2X"), (TBYTE)*p);
#endif
		sRet += szSymbol;
		p++;
	}
	return sRet;
}

MAKFC_CString EscapeData3(const TCHAR *lpDataW, int/* iSize*/)
{
	MAKFC_CString sRet;
	char szSymbol[1024];

	MAKFC_CString s = lpDataW;
	const char *lpData = s.NetStrA(CP_UTF8);
	const char *p = lpData;

	while (*p != '\0')
	{
#ifdef _UNICODE
#pragma message ("************************EscapeData2: escapement still in ANSI 1251****************")
		if (*p<0 || (!isalpha(*p) && !isdigit(*p) && !strchr("-.", *p)))
			sprintf(szSymbol, "%%%.2X", (BYTE)*p); //%uXXXX for unicode
		else
		{
			szSymbol[0] = *p;
			szSymbol[1] = _T('\0');
		}
#else
		_stprintf(szSymbol, _T("%%%.2X"), (TBYTE)*p);
#endif
		sRet += szSymbol;
		p++;
	}
	return sRet;
}

MAKFC_CString EscapeString(const MAKFC_CString& sString)
{
	return EscapeData3(sString, sString.GetLength());
}

MAKFC_CString EscapeJsonString(const MAKFC_CString& sString)
{
	MAKFC_CString result;
	
	auto isallowed = [](const wchar_t ch) {
		return iswalnum(ch) || (ch == L' ') || (ch == L':') || (ch == L',') || (ch == L'.') || (ch == L'#') || (ch == L'/') || (ch == L'-') || (ch == L'_');
	};

	const wchar_t *ch = sString;
	for (; *ch != L'\0'; ++ch)
	{
		if (*ch == L'\\')
		{
			result += L"\\\\";
			continue;
		}

		if (isallowed(*ch))
		{
			result += *ch;
			continue;
		}
		
		result.AppendFormat(L"\\u%04X", static_cast<int>(*ch));
	}

	return result;
}

static inline bool isHexDigit(TCHAR c)
{
	if (_istdigit(c))
		return true;
	if (c >= _T('a') && c <= _T('f'))
		return true;
	if (c >= _T('A') && c <= _T('F'))
		return true;

	return false;
}

MAKFC_CString UnEscapeData(const TCHAR *lpData, int iSize)
{
	MAKFC_CString sRet;
	const TCHAR *p = lpData;
	while (p - lpData < iSize)
	{
		if(*p==_T('%') && p-lpData<iSize-2 && isHexDigit(*(p+1)) && isHexDigit(*(p+2)))
		{
			sRet+=(TCHAR)((*(p+1)-'0')*0x010 + (*(p+2)-'0'));
			p+=3;
		}
		else
		{
			sRet+=(TCHAR)*p;
			p++;
		}
	}
	return sRet;
}
MAKFC_CString EscapeDataA(const char *lpData, int iSize)
{
	char szSymbol[1024];
	MAKFC_CString sRet;
	const char *p = lpData;
	while (p - lpData < iSize)
	{
		sprintf(szSymbol, "%%%.2X", (BYTE)*p);
		sRet += szSymbol;
		p++;
	}
	return sRet;
}

MAKFC_CString UnEscapeDataA(const char *lpData, int iSize)
{
	MAKFC_CString sRet;
	const char *p = lpData;
	while (p - lpData < iSize)
	{
		if(*p==_T('%') && p-lpData<iSize-2 && isdigit(*(p+1)) && isdigit(*(p+2)))
		{
			sRet+=(char)((*(p+1)-'0')*0x010 + (*(p+2)-'0'));
			p+=3;
		}
		else
		{
			sRet+=(char)*p;
			p++;
		}
	}
	return sRet;
}

int isrussian(TCHAR c)
{
	if ((c >= _T('à') && c <= _T('ÿ')) || (c >= _T('À') && c <= _T('ß')) || c == _T('¸') || c == _T('¨') || c == _T('¹'))
		return TRUE;
	return FALSE;
}
int isrussianA(char c)
{
	if ((c >= 'à' && c <= 'ÿ') || (c >= 'À' && c <= 'ß') || c == '¸' || c == '¨' || c == '¹')
		return TRUE;
	return FALSE;
}
int isemail(TCHAR c)
{
	if (!isemailname(c) && c!=_T('@'))
		return FALSE;
	return TRUE;
}
int isemailname(TCHAR c)
{
	if (c>0x7F || (!_istalpha(c) && !isdigit(c) && c != _T('_') && c != _T('.') && c != _T('-')))
		return FALSE;
	return TRUE;
}
int isemaildomain(TCHAR c)
{
	return isemailname(c);
}
int isenglish(TCHAR c)
{
	if ((c >= _T('a') && c <= _T('z')) || (c >= _T('A') && c <= _T('Z')))
		return TRUE;
	return FALSE;
}

BOOL makfc_IsEmail(LPCTSTR szStr)
{
	LPCTSTR p = szStr;
	int iAlpha = 0;
	int iName = 0, iDomain = 0;
	int iDots = 0;
	while (*p != 0)
	{
		if (*p == _T('@'))
		{
			iAlpha++;
			if (iAlpha > 1)
				return FALSE;
		}
		else if (iAlpha == 0)
		{
			if (!isemailname(*p))
				return FALSE;
			iName++;
		}
		else
		{
			if (!isemaildomain(*p))
				return FALSE;
			if (*p == _T('.'))
				iDots++;
			iDomain++;
		}
		p++;
	}
	if (iAlpha == 1 && iName > 0 && iDomain - iDots > 0 && iDots > 0)
		return TRUE;
	return FALSE;
}

BOOL IsEmptyGuid(GUID guid)
{
	if (guid.Data1 == 0 && guid.Data2 == 0 && guid.Data3 == 0
		&& guid.Data4[0] == 0
		&& guid.Data4[1] == 0
		&& guid.Data4[2] == 0
		&& guid.Data4[3] == 0
		&& guid.Data4[4] == 0
		&& guid.Data4[5] == 0
		&& guid.Data4[6] == 0
		&& guid.Data4[7] == 0
		)
		return TRUE;
	return FALSE;
}

GUID GetEmptyGuid()
{
	GUID guid = {0};
	return guid;
}

const std::wstring LoadWstringFromUtf8Buf(const char *buf, const int len)
{
	assert(buf);
	assert(len >= 0);
	assert(len < 1024 * 1024);

	if (!buf || (len == 0))
	{
		return L"";
	}

	MAKFC_CString wide;
	wide.LoadFromUTF8(buf, len);

	return wide.NetStrW();
}

MAKFC_CString GetMD5Hex(const char* lpBuffer)
{
	LPDWORD lpHash = (LPDWORD) lpBuffer;

	MAKFC_CString sHex;
	sHex.Format( L"%08x%08x%08x%08x", htonl(lpHash[0]), htonl(lpHash[1]), htonl(lpHash[2]), htonl(lpHash[3] ) );

	return sHex;
}

BOOL makfc_IsPhone(MAKFC_CString sPhone)
{
	int iLen = sPhone.Len();
	if(iLen<1)
	{
		return FALSE;
	}
	TCHAR *p = sPhone.GetBuffer();
	BOOL bHasDigit=FALSE;
	for (int i = 0; i < iLen; i++)
	{
		if (*(p+i) == _T('+'))
		{
		}
		else if (!isrussian(*(p+i)) && (_istdigit(*(p+i))))
		{
			bHasDigit=TRUE;
		}
		else if (*(p+i) != _T('-') && *(p+i) != _T(':')
			&& *(p+i) != _T(' ')
			&& *(p+i) != _T('.')
			&& *(p+i) != _T('#')
			&& *(p+i) != _T('*')
			&& *(p+i) != _T('^')
			&& *(p+i) != _T('\'')
			&& *(p+i) != _T('(')
			&& *(p+i) != _T(')')
			//				&& !_istalpha(*(p+i))
			)
		{
			return FALSE;
		}
	}
	return bHasDigit;
}

MAKFC_CString MAKFC_CString::CreateTiledString( TCHAR c, UINT uiLength )
{
	MAKFC_CString resultString;
	for (UINT i = 0; i < uiLength; ++i)
	{
		resultString += c;
	}
	
	return resultString;
}