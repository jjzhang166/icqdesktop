/* 
* Copyright (C) 2001-2003 Mail.Ru 
* 
*/
#include "stdafx.h"

#include "lps.h"
#include "stream.h"
#include "string.h"

#ifndef lint
#endif

char *lps2szA(char *sz, const BYTE *lps)
{
	unsigned long len;
	
	len = *((unsigned long *)lps);
	memcpy(sz, lps + sizeof(unsigned long), len);
	*(sz + len) = 0;
	return sz;
}
wchar_t *lps2szW(wchar_t *sz, const BYTE *lps)
{
	unsigned long len;
	
	len = *((unsigned long *)lps);
	memcpy(sz, lps + sizeof(unsigned long), len);
	*(sz + len/sizeof(wchar_t)) = L'\0';
	return sz;
}
wchar_t *lps2szAtoW(wchar_t *szw, const BYTE *lps, UINT uiCodePage)
{
	unsigned long len = *((unsigned long *)lps);
	char *sz = (char *)(lps + sizeof(DWORD));
	if (len > 0)
	{
		int iBytesNeeded = MultiByteToWideChar(uiCodePage, 0, sz, len,
			NULL, 0);
		if (iBytesNeeded > 0)
		{
			iBytesNeeded = MultiByteToWideChar(uiCodePage, 0, sz, len,
				szw, iBytesNeeded);
		}
		szw[iBytesNeeded] = L'\0';
	}
	else
		szw[0] = L'\0';
	return szw;
}
char *lps2szWtoA(char *sz, const BYTE *lps, UINT uiCodePage)
{
	unsigned long len = *((unsigned long *)lps) / sizeof(wchar_t);
	wchar_t *szw = (wchar_t *)(lps + sizeof(DWORD));
	if (len > 0)
	{
		int iBytesNeeded = WideCharToMultiByte(uiCodePage, 0, szw, len,
			NULL, 0, 0, NULL);
		if (iBytesNeeded > 0)
		{
			iBytesNeeded = WideCharToMultiByte(uiCodePage, 0, szw, len,
				sz, iBytesNeeded, 0, NULL);
		}
		sz[iBytesNeeded] = '\0';
	}
	else
		sz[0] = '\0';
	return sz;
}
TCHAR *lps2szAtoT(TCHAR *szt, const BYTE *lps, UINT uiCodePage)
{
#ifndef _UNICODE
	return lps2szA(szt, lps);
#else
	return lps2szAtoW(szt, lps, uiCodePage);
#endif
}
TCHAR *lps2szWtoT(TCHAR *szt, const BYTE *lps, UINT)
{
	return lps2szW(szt, lps);
}


BYTE *sz2lpsA(BYTE *lps, const char *sz)
{
	unsigned long len;
	
	len = strlen(sz);
	*((unsigned long *)lps) = len;
	memcpy(lps + sizeof(unsigned long), sz, len);
	return lps;
}
BYTE *sz2lpsW(BYTE *lps, LPCWSTR sz)
{
	unsigned long len;
	
	len = wcslen(sz);
	*((unsigned long *)lps) = len*sizeof(wchar_t);
	memcpy(lps + sizeof(unsigned long), sz, len * sizeof(wchar_t));
	return lps;
}

char *mkszA(const BYTE *lps)
{
	int len = 1 + LPSLENGTH(lps);
	char *sz = (char *)malloc(len);
	
	return lps2szA(sz, lps);
}
wchar_t *mkszW(const BYTE *lps)
{
	int len = sizeof(wchar_t) + LPSLENGTH(lps);
	wchar_t *sz = (wchar_t *)malloc(len);
	
	return lps2szW(sz, lps);
}
wchar_t *mkszAtoW(const BYTE *lps, UINT uiCodePage)
{
	int len = 1 + LPSLENGTH(lps);
	wchar_t *szw = (wchar_t *)malloc(len * sizeof(wchar_t));
	
	return lps2szAtoW(szw, lps, uiCodePage);
}
char *mkszWtoA(const BYTE *lps, UINT uiCodePage)
{
	int len = 1 + LPSLENGTH(lps);
	char *sz = (char *)malloc(len * sizeof(char));
	
	return lps2szWtoA(sz, lps, uiCodePage);
}
TCHAR *mkszAtoT(const BYTE *lps, UINT uiCodePage)
{
#ifdef _UNICODE
	return mkszAtoW(lps, uiCodePage);
#else
	return mkszA(lps);
#endif
}
TCHAR *mkszWtoT(const BYTE *lps, UINT)
{
	return mkszW(lps);
}

BYTE *mklpsA(const char *sz)
{
	BYTE *lps = (BYTE *)malloc(sizeof(DWORD) + strlen(sz));
	
	return sz2lpsA(lps, sz);
}
BYTE *mklpsW(const wchar_t *sz)
{
	BYTE *lps = (BYTE *)malloc(sizeof(DWORD) + wcslen(sz)*sizeof(wchar_t));
	
	return sz2lpsW(lps, sz);
}

BYTE *lpsdup(const BYTE *lps_src)
{
	if (!lps_src)
		return NULL;
	BYTE *r;
	r = (BYTE *)malloc(LPSSIZE(lps_src));
	memcpy(r, lps_src, LPSSIZE(lps_src));
	return r;
}

BYTE *lpscatszA(BYTE **lps_dest, const char *sz_src)
{
	if (*lps_dest)
	{
		*lps_dest = (BYTE *)realloc(*lps_dest, LPSSIZE(*lps_dest) + strlen(sz_src));
		memcpy((*lps_dest) + LPSSIZE(*lps_dest), sz_src, strlen(sz_src));
		*((unsigned long *)(*lps_dest)) = LPSLENGTH(*lps_dest) + strlen(sz_src);
	}
	else
		*lps_dest = mklpsA(sz_src);
	return *lps_dest;
}
BYTE *lpscatszW(BYTE **lps_dest, const wchar_t *sz_src)
{
	if (*lps_dest)
	{
		int iLen = wcslen(sz_src);
		*lps_dest = (BYTE *)realloc(*lps_dest, LPSSIZE(*lps_dest) + iLen*sizeof(wchar_t));
		memcpy((*lps_dest) + LPSSIZE(*lps_dest), sz_src, iLen*sizeof(wchar_t));
		*((unsigned long *)(*lps_dest)) = LPSLENGTH(*lps_dest) + iLen*sizeof(wchar_t);
	}
	else
		*lps_dest = mklpsW(sz_src);
	return *lps_dest;
}

int IsValidLPS(const BYTE *lps)
{
	if (lps && *((unsigned long *)lps) < 0x00FFFFFF)
		return 1;
	return 0;
}

MAKFC_CLPS::MAKFC_CLPS()
{
	size = 4;
	buffer = 0;
	m_bUnicode = FALSE;
}
MAKFC_CLPS::~MAKFC_CLPS()
{
	clear();
}
MAKFC_CLPS::MAKFC_CLPS(const char *str)
{
	buffer = mklpsA(str);
	if (buffer)
		size = LPSSIZE(buffer);
	else
		size = 4;
	m_bUnicode = FALSE;
}
MAKFC_CLPS::MAKFC_CLPS(const wchar_t *str)
{
	buffer = mklpsW(str);
	if (buffer)
		size = LPSSIZE(buffer);
	else
		size = 4;
	m_bUnicode = TRUE;
}
MAKFC_CLPS::MAKFC_CLPS(const BYTE *data, int data_size)
{
	if (data_size > 0)
	{
		buffer = (BYTE *)malloc(data_size+sizeof(DWORD));
		memcpy(buffer + sizeof(DWORD), data, data_size);
		*((DWORD *)buffer) = data_size;
		this->size = data_size + sizeof(DWORD);
	}
	else
	{
		buffer = 0;
		this->size = 4;
	}
	m_bUnicode = FALSE;
}
MAKFC_CLPS::MAKFC_CLPS(const int val)
{
	size = sizeof(int) + sizeof(DWORD);
	buffer = (BYTE *)malloc(size);
	*((DWORD *)buffer + 0) = size;
	*((DWORD *)buffer + 1) = val;
	m_bUnicode = FALSE;
}
MAKFC_CLPS::MAKFC_CLPS(const MAKFC_CLPS &lps)
{
	size = lps.size;
	if (lps.buffer)
		buffer = lpsdup(lps.buffer);  
	else
		buffer = NULL;
	m_bUnicode = lps.m_bUnicode;
}

/*MAKFC_CLPS::MAKFC_CLPS(const MAKFC_CString &s)
{
#ifndef _UNICODE
	buffer = mklpsW(s.m_pchData);
	if (buffer)
		size = LPSSIZE(buffer);
	else
		size = 0;
	m_bUnicode = TRUE;
#else
	buffer = mklpsA(s.m_pchData);
	if (buffer)
		size = LPSSIZE(buffer);
	else
		size = 0;
	m_bUnicode = FALSE;
#endif
}	 */

int MAKFC_CLPS::getsize()
{
	return size;
}
BYTE * MAKFC_CLPS::getdata()
{
	if (buffer)
	{
		return buffer + sizeof(DWORD);
	}
	
	assert(size == sizeof(DWORD));
	return nullptr;
}

const BYTE* MAKFC_CLPS::getdatac() const
{
	if (buffer)
	{
		return buffer + sizeof(DWORD);
	}

	assert(size == sizeof(DWORD));
	return nullptr;
}

BYTE * MAKFC_CLPS::getLPS()
{
	assert(buffer);
	return buffer;
}

void MAKFC_CLPS::clear()
{
	if (buffer)
	{
		free(buffer);
		buffer = nullptr;
	}
	size = 4;
}

bool MAKFC_CLPS::empty() const
{
	return (size == 4);
}

void MAKFC_CLPS::operator=(const MAKFC_CLPS lps2)
{
	if (buffer)
		free(buffer);
	size = lps2.size;
	if (lps2.buffer)
		buffer = lpsdup(lps2.buffer);
	else
		buffer = NULL;
	m_bUnicode = lps2.m_bUnicode;
}
int MAKFC_CLPS::operator==(const MAKFC_CLPS lps2)
{
	if (size == lps2.size && memcmp(buffer, lps2.buffer, size) == 0)
		return 1;
	return 0;
}

int MAKFC_CLPS::Serialize(const byte* lps)
{
	assert(size >= 4);
	if(lps==NULL)
	{
		size=4;
	}
	else
	{
		size = LPSLENGTH(lps);
	}
	if (size < 0xF0000000)
	{
		if (buffer)
			free(buffer);
		buffer = (byte *)malloc(size + sizeof(DWORD));
		memcpy((byte *)buffer, lps, size + sizeof(DWORD));
		return size;
	}
	return 0;
}

int MAKFC_CLPS::Serialize(MAKFC_CInputStream *is)
{
	int bRead;
	size = is->readInt(&bRead) + sizeof(DWORD);
	assert(size >= 4);
	if (bRead && size >= sizeof(DWORD) && size < 0xF0000000
		&& (int)(size - sizeof(DWORD)) <= is->available())
	{
		if (buffer)
			free(buffer);
		buffer = (BYTE *)malloc(size);
		int iSizeOfData = 0;
		if (size > sizeof(DWORD))
			iSizeOfData = is->read((BYTE *)buffer + sizeof(DWORD), size - sizeof(DWORD));
		if (iSizeOfData != (int)(size - sizeof(DWORD)))
		{
			size = 0;
			free(buffer);
			assert(FALSE);
			return 0;
		}
		*((DWORD *)buffer) = iSizeOfData;
		return iSizeOfData;
	}
	size = 0;
	return 0;
}

int MAKFC_CLPS::Serialize(MAKFC_COutputStream *os)
{
	assert(size >= 4);
	os->writeInt(size - sizeof(DWORD));
	if (size - sizeof(DWORD) > 0 && buffer)
	{
		return os->write(buffer + sizeof(DWORD), size - sizeof(DWORD));
	}
	return sizeof(DWORD);
}

//tool lps functions

void GetLpsX(BYTE *lpstr, char xor_char)
{
	long pos = 0;
	char abc;
	if (!lpstr || !LPSLENGTH(lpstr))
		return;
	while (pos < (long)LPSLENGTH(lpstr))
	{
		abc = lpstr[sizeof(long)+pos];
		abc ^= xor_char;
		lpstr[sizeof(long)+pos] = abc;
		pos++;
	}
}

BYTE * mklpsT(const TCHAR *sz)
{
#ifdef _UNICODE
	return mklpsW(sz);
#else
	return mklpsA(sz);
#endif
}

MAKFC_CString MAKFC_CLPS::ToStringA(UINT uiCodePage) const
{

	MAKFC_CString s;
	if (buffer)
	{
		lps2szA((char *)s.NetGetBufferA(LPSLENGTH(buffer) + 1), buffer);
		s.NetReleaseBufferA(uiCodePage);
	}
	return s;
}

MAKFC_CString MAKFC_CLPS::ToStringW(UINT uiCodePage) const
{
	MAKFC_CString s;
	if (buffer)
	{
		lps2szW((wchar_t *)s.NetGetBufferW((LPSLENGTH(buffer) + 1) * sizeof(wchar_t)), buffer);
		s.NetReleaseBufferW(uiCodePage);
	}
	return s;
}

int MAKFC_CLPS::getDataSize() const
{
	return size-sizeof(DWORD);
}