#include "stdafx.h"
#include "stream.h"
#include "string.h"
#include "lps.h"

MAKFC_CBufferInputStream::MAKFC_CBufferInputStream(BYTE *buffer, int size)
{
	m_lpBuffer = buffer;
	m_iSize = size;
	m_iPos = 0;
}
MAKFC_CBufferInputStream::~MAKFC_CBufferInputStream()
{
}

int MAKFC_CBufferInputStream::read(BYTE *buf, int size)
{
	int available = m_iSize - m_iPos;
	if (size > available)
	{
		memcpy(buf, m_lpBuffer + m_iPos, available);
		m_iPos += available;
		return available;
	}
	else
	{
		memcpy(buf, m_lpBuffer + m_iPos, size);
		m_iPos += size;
		return size;
	}
}
int MAKFC_CBufferInputStream::readUntil(BYTE *buf, int size, const BYTE *toFind, int findSize)
{
	BYTE *p = m_lpBuffer + m_iPos;
	int pos = makfc_memfind(p, available(), toFind, findSize);
	if (pos >= 0 && size >= pos + findSize)
	{
		return read(buf, pos + findSize);
	}
	return 0;
}
int MAKFC_CBufferInputStream::readInt(int *bRead)
{
	int ret;
	int val = 0;
	ret = read((BYTE*)&val, sizeof(int));
	if (ret != sizeof(int) && bRead)
		*bRead = FALSE;		 
	else if (bRead)
		*bRead = TRUE;
	return val;
}

double MAKFC_CBufferInputStream::readDouble(int *bRead)
{
	int ret;
	double val = 0;
	ret = read((BYTE*)&val, sizeof(double));
	if (ret != sizeof(double) && bRead)
		*bRead = FALSE;		 
	else if (bRead)
		*bRead = TRUE;
	return val;
}


DWORD MAKFC_CBufferInputStream::readDWORD(int *bRead)
{
	int ret;
	DWORD val = 0;
	ret = read((BYTE*)&val, sizeof(DWORD));
	if (ret != sizeof(DWORD) && bRead)
		*bRead = FALSE;		 
	else if (bRead)
		*bRead = TRUE;
	return val;
}

__int64 MAKFC_CBufferInputStream::readInt64(int *bRead)
{
	int ret;
	__int64 val = 0;
	ret = read((BYTE*)&val, sizeof(__int64));
	if (ret != sizeof(__int64) && bRead)
		*bRead = FALSE;		 
	else if (bRead)
		*bRead = TRUE;
	return val;
}

unsigned __int64 MAKFC_CBufferInputStream::readUInt64(int *bRead)
{
	int ret;
	unsigned __int64 val = 0;
	ret = read((BYTE*)&val, sizeof(unsigned __int64));
	if (ret != sizeof(unsigned __int64) && bRead)
		*bRead = FALSE;		 
	else if (bRead)
		*bRead = TRUE;
	return val;
}

short MAKFC_CBufferInputStream::readShort(int *bRead)
{
	int ret;
	short val = 0;
	ret = this->read((BYTE*)&val, sizeof(short));
	if (ret != sizeof(short) && bRead)
		*bRead = FALSE;
	else if (bRead)
		*bRead = TRUE;
	return val;
}
BYTE MAKFC_CBufferInputStream::readByte(int *bRead)
{
	int ret;
	BYTE val = 0;
	ret = this->read((BYTE*)&val, sizeof(BYTE));
	if (ret != sizeof(BYTE) && bRead)
		*bRead = FALSE;
	else if (bRead)
		*bRead = TRUE;
	return val;
}

BYTE * MAKFC_CBufferInputStream::getBuffer()
{
	return m_lpBuffer;
}

void MAKFC_CBufferInputStream::close()
{
} 
int MAKFC_CBufferInputStream::available() const
{
	return (m_iSize - m_iPos >= 0) ? m_iSize - m_iPos : 0; 
}
int MAKFC_CBufferInputStream::readLpsAsSzA(char **buf)
{
	BYTE *lpsBuf = readLps();
	if (lpsBuf)
	{
		*buf = mkszA(lpsBuf);
		free(lpsBuf);
		return strlen(*buf);
	}
	return 0;
}
/*int MAKFC_CBufferInputStream::readLpsAsSz(LPTSTR *buf)
{
#ifdef _UNICODE
	return readLpsAsSzW(buf);
#else
	return readLpsAsSzA(buf);
#endif
} */

int MAKFC_CBufferInputStream::readLpsAsSzW(wchar_t **buf)
{
	BYTE *lpsBuf = readLps();
	if (lpsBuf)
	{
		*buf = mkszW(lpsBuf);
		free(lpsBuf);
		return wcslen(*buf);
	}
	return 0;
}
char * MAKFC_CBufferInputStream::readLpsAsSzA()
{
	char *szBuf = NULL;
	readLpsAsSzA(&szBuf);
	if (szBuf)
		return szBuf;
	return NULL;
}
wchar_t * MAKFC_CBufferInputStream::readLpsAsSzW()
{
	wchar_t *szBuf = NULL;
	readLpsAsSzW(&szBuf);
	if (szBuf)
		return szBuf;
	return NULL;
}
/*LPTSTR MAKFC_CBufferInputStream::readLpsAsSz()
{
#ifdef _UNICODE
	return readLpsAsSzW();
#else
	return readLpsAsSzA();
#endif
} */
int MAKFC_CBufferInputStream::readLps(BYTE **buf)
{
	return readLps(buf, 4, false);
}

int MAKFC_CBufferInputStream::readLps(BYTE **buf, BYTE nBytesPerLen, bool bNetworkByteOrder)
{
	BOOL bRead = FALSE;
	int iLength = 0;

	switch (nBytesPerLen)
	{
	case 1:
		iLength = readByte(&bRead);
		break;
	case 2:
		{
			short tmp;
			tmp = readShort(&bRead);
			iLength = (bNetworkByteOrder) ? ntohs(tmp) : tmp;
		}
		break;
	case 4:
		{
			iLength = readInt(&bRead);
			iLength = (bNetworkByteOrder) ? ntohl(iLength) : iLength;
		}
		break;
	default:
		assert (false);
		break;
	}
	
	if (!bRead)
		return 0;

	if (iLength > available())
		return 0;
	*buf = (BYTE *)malloc(iLength + sizeof(int));
	*LPINT(*buf) = iLength;
	int iRead = read(((BYTE *)(*buf)) + sizeof(int), iLength);
	if (iRead != iLength)
	{
		free(*buf);
		*buf = NULL;
		return 0;
	}
	return iLength + sizeof(int);
}

int MAKFC_CBufferInputStream::readLpsAsStringA(MAKFC_CString &stBuf, UINT uiCodePage)
{
	BYTE *lpsBuf = readLps();
	if (lpsBuf)
	{
		BYTE *lpPtr = stBuf.NetGetBufferA(LPSLENGTH(lpsBuf) + sizeof(char));
		lps2szA((char *)lpPtr, lpsBuf);
		stBuf.NetReleaseBufferA(uiCodePage);
		free(lpsBuf);
		return stBuf.GetLength();
	}
	stBuf = _T("");
	return 0;
}

int MAKFC_CBufferInputStream::readLpsAsStringA(MAKFC_CString &stBuf, BYTE nBytesPerLen, 
	UINT uiCodePage, bool bNetworkByteOrder)
{
	BYTE *lpsBuf = readLps(nBytesPerLen, bNetworkByteOrder);
	if (lpsBuf)
	{
		BYTE *lpPtr = stBuf.NetGetBufferA(LPSLENGTH(lpsBuf) + sizeof(char));
		lps2szA((char *)lpPtr, lpsBuf);
		stBuf.NetReleaseBufferA(uiCodePage);
		free(lpsBuf);
		return stBuf.GetLength();
	}
	stBuf = _T("");
	return 0;
}

int MAKFC_CBufferInputStream::readLpsAsStringW(MAKFC_CString &stBuf, UINT uiCodePage)
{
	BYTE *lpsBuf = readLps();
	if (lpsBuf)
	{
		BYTE *lpPtr = stBuf.NetGetBufferW((LPSLENGTH(lpsBuf) + 1) * sizeof(wchar_t));
		lps2szW((wchar_t *)lpPtr, lpsBuf);
		stBuf.NetReleaseBufferW(uiCodePage);
		free(lpsBuf);
		return stBuf.GetLength();
	}
	stBuf = _T("");
	return 0;
}

int MAKFC_CBufferInputStream::readLpsAsStringW(MAKFC_CString &stBuf, BYTE nBytesPerLen, 
	UINT uiCodePage, bool bNetworkByteOrder)
{
	BYTE *lpsBuf = readLps(nBytesPerLen, bNetworkByteOrder);
	if (lpsBuf)
	{
		BYTE *lpPtr = stBuf.NetGetBufferW((LPSLENGTH(lpsBuf) + 1) * sizeof(wchar_t));
		lps2szW((wchar_t *)lpPtr, lpsBuf);
		stBuf.NetReleaseBufferW(uiCodePage);
		free(lpsBuf);
		return stBuf.GetLength();
	}
	stBuf = _T("");
	return 0;
}

BYTE * MAKFC_CBufferInputStream::readLps()
{
	BYTE *buf;
	if (readLps(&buf) > 0)
		return buf;
	return NULL;
}

BYTE * MAKFC_CBufferInputStream::readLps(BYTE nBytesPerLen, bool bNetworkByteOrder)
{
	BYTE *buf;
	if (readLps(&buf, nBytesPerLen, bNetworkByteOrder) > 0)
		return buf;
	return NULL;
}

wchar_t * MAKFC_CBufferInputStream::readLpsAsSzAtoW(UINT uiCodePage)
{
	char *sz = readLpsAsSzA();
	wchar_t *szw = NULL;
	if (sz)
	{
		int iLen = strlen(sz);
		int iBytesNeeded = MultiByteToWideChar(uiCodePage, 0, sz, iLen+1,
			NULL, 0);
		if (iBytesNeeded > 0)
		{
			szw = (wchar_t *)malloc(sizeof(wchar_t) * (iBytesNeeded + 1));
			iBytesNeeded = MultiByteToWideChar(uiCodePage, 0, sz, iLen+1,
				szw, iBytesNeeded);
		}
		free(sz);
	}
	return szw;
}

char * MAKFC_CBufferInputStream::readLpsAsSzWtoA(UINT uiCodePage)
{
	wchar_t *szw = readLpsAsSzW();
	char *sz = NULL;
	if (szw)
	{
		int iLen = wcslen(szw);
		int iBytesNeeded = WideCharToMultiByte(uiCodePage, 0, szw, iLen+1,
			NULL, 0, 0, NULL);
		if (iBytesNeeded > 0)
		{
			sz = (char *)malloc(sizeof(char) * (iBytesNeeded + 1));
			iBytesNeeded = WideCharToMultiByte(uiCodePage, 0, szw, iLen+1,
				sz, iBytesNeeded, 0, NULL);
		}
		free(szw);
	}
	return sz;
	
}
TCHAR * MAKFC_CBufferInputStream::readLpsAsSzAtoT(UINT uiCodePage)
{
#ifdef _UNICODE
	return readLpsAsSzAtoW(uiCodePage);
#else
	return readLpsAsSzA();
#endif
}
TCHAR * MAKFC_CBufferInputStream::readLpsAsSzWtoT(UINT)
{
	return readLpsAsSzW();
}
MAKFC_CBufferOutputStream::MAKFC_CBufferOutputStream(int iSize, int iGrowby)
{
	m_lpBuffer = 0;
	m_iPos = 0;
	m_iGrowby = iGrowby;
	m_iSize = iSize;
	
	if (m_iSize > 0)
		m_lpBuffer = (BYTE*)malloc(m_iSize);
}
MAKFC_CBufferOutputStream::~MAKFC_CBufferOutputStream()
{
	free(m_lpBuffer);
	m_iSize = 0;
	m_iPos = 0;
}

int MAKFC_CBufferOutputStream::write(const BYTE *buf, int iSize)
{
	if (iSize > 0)
	{
		if (m_iPos + iSize > m_iSize)
		{
			m_lpBuffer = (BYTE*)realloc(m_lpBuffer, m_iPos + iSize);
			m_iSize = m_iPos + iSize;
		}
		memcpy(m_lpBuffer + m_iPos, buf, iSize);
		m_iPos += iSize;
		return iSize;
	}
	return 0;
}

void MAKFC_CBufferOutputStream::writeInt(int val)
{
	write((BYTE*)&val, sizeof(val));
}

void MAKFC_CBufferOutputStream::writeDouble(double val)
{
	write((BYTE*)&val, sizeof(val));
}


void MAKFC_CBufferOutputStream::writeDWORD( DWORD dwVal )
{
	write((BYTE*)&dwVal, sizeof(dwVal));
}

void MAKFC_CBufferOutputStream::writeInt64( __int64 val )
{
	write((BYTE*)&val, sizeof(val));
}

void MAKFC_CBufferOutputStream::writeShort(short val)
{
	write((BYTE*)&val, sizeof(val));
}

void MAKFC_CBufferOutputStream::writeUshort(unsigned short val)
{
	write((BYTE*)&val, sizeof(val));
}

void MAKFC_CBufferOutputStream::writeByte(BYTE val)
{
	write((BYTE*)&val, sizeof(val));
}
void MAKFC_CBufferOutputStream::writeSzAsLpsA(const char *sz)
{
	int iLen = strlen(sz);
	writeInt(iLen);
	write((BYTE *)sz, iLen);
}
void MAKFC_CBufferOutputStream::writeSzAsLpsW(const wchar_t *sz)
{
	int iLen = wcslen(sz) * sizeof(wchar_t);
	writeInt(iLen);
	write((BYTE *)sz, iLen);
}
void MAKFC_CBufferOutputStream::writeSzAsLps(LPCTSTR sz)
{
#ifdef _UNICODE
	writeSzAsLpsW(sz);
#else
	writeSzAsLpsA(sz);
#endif
}

void MAKFC_CBufferOutputStream::writeDataAsLPS(const BYTE *lpData, int size)
{
	writeInt(size);
	write(lpData, size);
}
void MAKFC_CBufferOutputStream::writeLPS(const BYTE *lps)
{
	write(lps, LPSSIZE(lps));
}


void MAKFC_CBufferOutputStream::close()
{
}

int MAKFC_CBufferOutputStream::size() const
{
	return m_iSize;
}

BYTE * MAKFC_CBufferOutputStream::getBuffer()
{
	return m_lpBuffer;
}

BYTE * MAKFC_CBufferOutputStream::appendBufferForWrite(int iSize)
{
	if (iSize > 0)
	{
		if (m_iPos + iSize > m_iSize)
		{
			m_lpBuffer = (BYTE*)realloc(m_lpBuffer, m_iPos + iSize);
			m_iSize = m_iPos + iSize;
		}
		m_iPos += iSize;
		return m_lpBuffer + (m_iPos - iSize);
	}
	return NULL;
}
	   
MAKFC_CReaderWriter::MAKFC_CReaderWriter() : MAKFC_CBufferInputStream(0, 0), MAKFC_CBufferOutputStream()
{
}
MAKFC_CReaderWriter::MAKFC_CReaderWriter(MAKFC_CLPS &lps) : MAKFC_CBufferInputStream(0, 0), MAKFC_CBufferOutputStream()
{
	write(lps.getdata(), lps.getsize());
}
MAKFC_CReaderWriter::MAKFC_CReaderWriter(const MAKFC_CReaderWriter &rw) : MAKFC_CBufferInputStream(0, 0), MAKFC_CBufferOutputStream()
{
	appendToBuffer(rw.MAKFC_CBufferOutputStream::m_iSize);
	write(rw.MAKFC_CBufferOutputStream::m_lpBuffer, rw.MAKFC_CBufferOutputStream::m_iSize);
	MAKFC_CBufferOutputStream::m_iGrowby = rw.MAKFC_CBufferOutputStream::m_iGrowby;
	MAKFC_CBufferOutputStream::m_iPos = rw.MAKFC_CBufferOutputStream::m_iPos;
	MAKFC_CBufferInputStream::m_iPos = rw.MAKFC_CBufferInputStream::m_iPos;
	MAKFC_CBufferInputStream::m_iSize = rw.MAKFC_CBufferInputStream::m_iSize;
}

MAKFC_CReaderWriter::~MAKFC_CReaderWriter()
{
}
const MAKFC_CReaderWriter& MAKFC_CReaderWriter::operator =(const MAKFC_CReaderWriter &rw)
{
	if (&rw == this)
		return *this;
	pop(-1);
	minimize();
	appendToBuffer(rw.MAKFC_CBufferOutputStream::m_iSize);
	write(rw.MAKFC_CBufferOutputStream::m_lpBuffer, rw.MAKFC_CBufferOutputStream::m_iSize);
	MAKFC_CBufferOutputStream::m_iGrowby = rw.MAKFC_CBufferOutputStream::m_iGrowby;
	MAKFC_CBufferOutputStream::m_iPos = rw.MAKFC_CBufferOutputStream::m_iPos;
	MAKFC_CBufferInputStream::m_iPos = rw.MAKFC_CBufferInputStream::m_iPos;
	MAKFC_CBufferInputStream::m_iSize = rw.MAKFC_CBufferInputStream::m_iSize;
	return *this;
}


void MAKFC_CReaderWriter::close()
{

}
int MAKFC_CReaderWriter::size() const
{
	return available();
}
int MAKFC_CReaderWriter::available() const
{
	return (MAKFC_CBufferOutputStream::m_iSize) - (MAKFC_CBufferInputStream::m_iPos);
}
int MAKFC_CReaderWriter::seek_read(int iPos, int iOffset)
{
	int iNewPos;
	if (iOffset == 0) //current pos
	{
		iNewPos = MAKFC_CBufferInputStream::m_iPos + iPos;
	}
	else if (iOffset == 1) //end
	{
		iNewPos = MAKFC_CBufferInputStream::m_iSize + iPos;
	}
	else //beginning
	{	 
		iNewPos = iPos;
	}
	if (iNewPos < 0 || iNewPos > MAKFC_CBufferInputStream::m_iSize)
		return FALSE;
	MAKFC_CBufferInputStream::m_iPos = iNewPos;
	return TRUE;
}
int MAKFC_CReaderWriter::seek_write(int iPos, int iOffset)
{
	int iNewPos;
	if (iOffset == 0) //current pos
	{
		iNewPos = MAKFC_CBufferOutputStream::m_iPos + iPos;
	}
	else if (iOffset == 1) //end
	{
		iNewPos = MAKFC_CBufferOutputStream::m_iSize + iPos;
	}
	else //beginning
	{	 
		iNewPos = iPos;
	}
	if (iNewPos < 0 || iNewPos > MAKFC_CBufferOutputStream::m_iSize)
		return FALSE;
	MAKFC_CBufferOutputStream::m_iPos = iNewPos;
	return TRUE;
}
int MAKFC_CReaderWriter::write(const BYTE *buf, int size)
{
	int ret = MAKFC_CBufferOutputStream::write(buf, size);
	MAKFC_CBufferInputStream::m_lpBuffer = MAKFC_CBufferOutputStream::m_lpBuffer;
	MAKFC_CBufferInputStream::m_iSize = MAKFC_CBufferOutputStream::m_iSize;
	return ret;
}
BYTE * MAKFC_CReaderWriter::appendToBuffer(int iBytesToAppend)
{
	if (iBytesToAppend < 0)
		return NULL;
	if (iBytesToAppend == 0)
		return getBuffer();
	MAKFC_CBufferOutputStream::m_lpBuffer = (BYTE *)realloc(MAKFC_CBufferOutputStream::m_lpBuffer, MAKFC_CBufferOutputStream::m_iSize + iBytesToAppend);
	MAKFC_CBufferOutputStream::m_iSize += iBytesToAppend;
	MAKFC_CBufferInputStream::m_lpBuffer = MAKFC_CBufferOutputStream::m_lpBuffer;
	MAKFC_CBufferInputStream::m_iSize = MAKFC_CBufferOutputStream::m_iSize;
	return MAKFC_CBufferOutputStream::m_lpBuffer + MAKFC_CBufferOutputStream::m_iSize - iBytesToAppend;
}
BYTE * MAKFC_CReaderWriter::appendToBufferMemset(int iBytesToAppend, int c)
{
	BYTE *lpBuf = appendToBuffer(iBytesToAppend);
	if (!lpBuf)
		return NULL;
	memset(lpBuf, c, iBytesToAppend);
	return lpBuf;
}
int MAKFC_CReaderWriter::minimize()
{
	int delta = MAKFC_CBufferInputStream::m_iSize - MAKFC_CBufferInputStream::m_iPos;

	memmove(MAKFC_CBufferOutputStream::m_lpBuffer,
		MAKFC_CBufferOutputStream::m_lpBuffer + MAKFC_CBufferInputStream::m_iPos,
		delta);

	if (delta > 0)
	{
		_expand(MAKFC_CBufferOutputStream::m_lpBuffer,
			delta);
	}
	else
	{
		free(MAKFC_CBufferOutputStream::m_lpBuffer);
		MAKFC_CBufferOutputStream::m_lpBuffer = NULL;
	}


	MAKFC_CBufferOutputStream::m_iSize -= MAKFC_CBufferInputStream::m_iPos;
	assert(MAKFC_CBufferOutputStream::m_iSize >= 0);
	MAKFC_CBufferOutputStream::m_iPos -= MAKFC_CBufferInputStream::m_iPos;
	if (MAKFC_CBufferOutputStream::m_iPos < 0)
		MAKFC_CBufferOutputStream::m_iPos = 0;
	MAKFC_CBufferInputStream::m_iSize = MAKFC_CBufferOutputStream::m_iSize;
	MAKFC_CBufferInputStream::m_iPos = 0;
	return MAKFC_CBufferOutputStream::m_iSize;
}

BYTE * MAKFC_CReaderWriter::pop(int iSize)
{
	//returns the pointer set to the current READ position
	//and moves the READ pointer 'iSize' bytes right
	if (iSize == -1)
		iSize = available();
	BYTE *p = MAKFC_CBufferInputStream::m_lpBuffer + MAKFC_CBufferInputStream::m_iPos;
	seek_read(iSize, 0);
	return p;
}
BYTE * MAKFC_CReaderWriter::push(int iSize)
{
	if (iSize == -1)
		iSize = MAKFC_CBufferInputStream::m_iPos;
	seek_read(-iSize, 0);
	BYTE *p = MAKFC_CBufferInputStream::m_lpBuffer + MAKFC_CBufferInputStream::m_iPos;
	return p;
}
BYTE * MAKFC_CReaderWriter::getBuffer() const
{
	return MAKFC_CBufferInputStream::m_lpBuffer;
}

MAKFC_CReaderWriter::operator MAKFC_CLPS()
{
	int avl = available();
	MAKFC_CLPS lps(pop(avl), avl);
	push(avl);
	return lps;
}

int MAKFC_CReaderWriter::readLpsTo(MAKFC_CReaderWriter *lpRW)
{
	BYTE *buf = readLps();
	if (buf)
	{
		int iSize = LPSSIZE(buf);
		lpRW->writeLPS(buf);
		free(buf);
		return iSize;
	}
	return 0;
}

BYTE& MAKFC_CReaderWriter::operator[](int iIndex)
{
	assert(iIndex < size());
	return MAKFC_CBufferInputStream::m_lpBuffer[iIndex];
}

int MAKFC_CReaderWriter::truncate(int size)
{
	assert(size <= MAKFC_CBufferInputStream::m_iSize);
	if (size >= MAKFC_CBufferInputStream::m_iSize)
		return MAKFC_CBufferInputStream::m_iSize;
	MAKFC_CBufferInputStream::m_iSize = size;
	MAKFC_CBufferOutputStream::m_iSize = size;
	if (MAKFC_CBufferInputStream::m_iPos > MAKFC_CBufferInputStream::m_iSize)
		MAKFC_CBufferInputStream::m_iPos = MAKFC_CBufferInputStream::m_iSize;
	if (MAKFC_CBufferOutputStream::m_iPos > MAKFC_CBufferOutputStream::m_iSize)
		MAKFC_CBufferOutputStream::m_iPos = MAKFC_CBufferOutputStream::m_iSize;
	return size;
}

MAKFC_CReaderWriter MAKFC_CReaderWriter::SZ_TO_LPS_A(LPCSTR sz)
{
	MAKFC_CReaderWriter rw;
	rw.writeSzAsLpsA(sz);
	return rw;
}

MAKFC_CReaderWriter MAKFC_CReaderWriter::SZ_TO_LPS_W(LPCWSTR sz)
{
	MAKFC_CReaderWriter rw;
	rw.writeSzAsLpsW(sz);
	return rw;
}

MAKFC_CReaderWriter MAKFC_CReaderWriter::SZ_TO_LPS_T(LPCTSTR sz)
{
	MAKFC_CReaderWriter rw;
	rw.writeSzAsLps(sz);
	return rw;	
}

MAKFC_CReaderWriter MAKFC_CReaderWriter::DWORD_TO_LPS(DWORD dwValue)
{
	MAKFC_CReaderWriter rw;
	rw.writeInt(dwValue);
	return rw;
}

MAKFC_CReaderWriter MAKFC_CReaderWriter::DWORDLPS_TO_LPS(DWORD dwValue)
{
	MAKFC_CReaderWriter rw;
	rw.writeInt(sizeof(DWORD));
	rw.writeInt(dwValue);
	return rw;
}

bool MAKFC_CReaderWriter::operator==(const MAKFC_CReaderWriter& rw) const
{
	return size() == rw.size() && memcmp(getBuffer(), rw.getBuffer(), size()) == 0;
}
