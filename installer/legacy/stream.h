#pragma once

#ifndef FALSE
#define FALSE		0
#endif

#ifndef TRUE
#define TRUE		1
#endif

class MAKFC_CString;

class MAKFC_CInputStream 
{
public:
	MAKFC_CInputStream()
	{
	}
	virtual ~MAKFC_CInputStream()
	{
	}
	virtual int read(BYTE *buf, int size) = 0;
	virtual int readInt(int *bRead = 0) = 0;
	virtual double readDouble(int *bRead = 0) = 0;
	virtual __int64 readInt64(int *bRead = 0) = 0;
	virtual short readShort(int *bRead = 0) = 0;
	virtual BYTE readByte(int *bRead = 0) = 0;
	virtual void close() = 0;
	virtual int available() const = 0;
};

class MAKFC_CBufferInputStream : public MAKFC_CInputStream
{
protected:
	BYTE *m_lpBuffer;
	int m_iSize;
	int m_iPos;
public:
	MAKFC_CBufferInputStream(BYTE *buffer, int size);
	virtual ~MAKFC_CBufferInputStream();

	virtual int read(BYTE *buf, int size);

	template<typename __Type>
	void read_item(__Type &item, BOOL *was_read = NULL)
	{
		const size_t need_to_read = sizeof(item);
		::memset(&item, 0, need_to_read);

		int read = read((BYTE*)&item, need_to_read);
		if (read != need_to_read && was_read)
		{
			*was_read = FALSE;		 
		}
		else if (was_read)
		{
			*was_read = TRUE;
		}
	}

	virtual int readInt(int *bRead = 0);
	virtual double readDouble(int *bRead = 0);
	virtual DWORD readDWORD(int *bRead = 0);
	virtual __int64 readInt64(int *bRead = 0);
	virtual unsigned __int64 readUInt64(int *bRead = 0);
	virtual short readShort(int *bRead = 0);
	virtual BYTE readByte(int *bRead = 0);
	virtual int readUntil(BYTE *buf, int size, const BYTE *toFind, int findSize);
	virtual int readLpsAsStringA(MAKFC_CString &stBuf, UINT uiCodePage = CP_ACP);
	virtual int readLpsAsStringA(MAKFC_CString &stBuf, BYTE nBytesPerLen, 
		UINT uiCodePage, bool bNetworkByteOrder);
	virtual int readLpsAsStringW(MAKFC_CString &stBuf, UINT uiCodePage = CP_ACP);
	virtual int readLpsAsStringW(MAKFC_CString &stBuf, BYTE nBytesPerLen, 
		UINT uiCodePage, bool bNetworkByteOrder);
	virtual int readLpsAsSzA(char **buf);
	virtual int readLpsAsSzW(wchar_t **buf);
//	virtual int readLpsAsSz(LPTSTR *buf);
	virtual char * readLpsAsSzA();
	virtual wchar_t * readLpsAsSzW();
	virtual wchar_t * readLpsAsSzAtoW(UINT uiCodePage = CP_ACP);
	virtual char * readLpsAsSzWtoA(UINT uiCodePage = CP_ACP);
	virtual TCHAR * readLpsAsSzAtoT(UINT uiCodePage = CP_ACP);
	virtual TCHAR * readLpsAsSzWtoT(UINT uiCodePage = CP_ACP);
//	virtual LPTSTR readLpsAsSz();
	virtual int readLps(BYTE **buf);
	virtual int readLps(BYTE **buf, BYTE nBytesPerLen, bool bNetworkByteOrder);
	virtual BYTE * readLps();
	virtual BYTE * readLps(BYTE nBytesPerLen, bool bNetworkByteOrder);
	virtual void close();
	virtual int available() const;

	virtual BYTE * getBuffer();
};

class MAKFC_COutputStream 
{
public:
	MAKFC_COutputStream()
	{
	}
	virtual ~MAKFC_COutputStream()
	{
	}
	virtual int write(const BYTE *buf, int size) = 0;
	virtual void writeInt(int val) = 0;
	virtual void writeDouble(double val) = 0;
	virtual void writeDWORD( DWORD dwVal ) = 0;
	virtual void writeShort(short val) = 0;
	virtual void writeByte(BYTE val) = 0;
	virtual void writeSzAsLpsA(const char *sz) = 0;
	virtual void writeSzAsLpsW(const wchar_t *sz) = 0;
	virtual void writeSzAsLps(LPCTSTR sz) = 0;
	virtual void writeDataAsLPS(const BYTE *lpData, int size) = 0;
	virtual void close() = 0;
	virtual int size() const = 0;
	virtual BYTE *appendBufferForWrite(int iSize) = 0;
};

class MAKFC_CBufferOutputStream : public MAKFC_COutputStream
{
protected:
	BYTE *m_lpBuffer;
	int m_iSize;
	int m_iPos;
	int m_iGrowby;
public:
	MAKFC_CBufferOutputStream(int size = 0, int growby = 0);
	virtual ~MAKFC_CBufferOutputStream();

	virtual int write(const BYTE *buf, int size);

	template<typename __Type>
	void write_item(__Type &item)
	{
		write((BYTE*)&item, sizeof(__Type));
	}

	virtual void writeInt(int val);
	virtual void writeDouble(double val);
	virtual void writeDWORD( DWORD dwVal );
	virtual void writeInt64( __int64 val );
	virtual void writeShort(short val);
	virtual void writeUshort(unsigned short val);
	virtual void writeByte(BYTE val);
	virtual void writeSzAsLpsA(const char *sz);
	virtual void writeSzAsLpsW(const wchar_t *sz);
	virtual void writeSzAsLps(LPCTSTR sz);
	virtual void writeDataAsLPS(const BYTE *lpData, int size);
	virtual void writeLPS(const BYTE *lps);
	virtual void close();
	virtual int size() const;
	virtual BYTE *appendBufferForWrite(int iSize);

	virtual BYTE * getBuffer();
};

class MAKFC_CLPS;

class MAKFC_CReaderWriter : public MAKFC_CBufferInputStream, public MAKFC_CBufferOutputStream
{
protected:
public:
	MAKFC_CReaderWriter();
	MAKFC_CReaderWriter(MAKFC_CLPS &lps);
	MAKFC_CReaderWriter(const MAKFC_CReaderWriter &rw);
	virtual ~MAKFC_CReaderWriter();

	virtual void close();
	virtual int size() const;
	virtual int available() const;
	virtual int seek_read(int pos, int offset);
	virtual int seek_write(int pos, int offset);
	virtual int write(const BYTE *buf, int size);
	virtual int minimize();
	virtual int truncate(int size);
	virtual BYTE * pop(int iSize);
	virtual BYTE * push(int iSize);
	virtual BYTE * getBuffer() const;
	virtual BYTE * appendToBuffer(int iBytesToAppend);
	virtual BYTE * appendToBufferMemset(int iBytesToAppend, int c);

	virtual int readLpsTo(MAKFC_CReaderWriter *lpRW);

	operator MAKFC_CLPS();
	const MAKFC_CReaderWriter& operator =(const MAKFC_CReaderWriter &rw);
	bool operator==(const MAKFC_CReaderWriter& rw) const;
	bool operator!=(const MAKFC_CReaderWriter& rw) const { return !(*this == rw); }
	BYTE& operator [](int iIndex);

	static MAKFC_CReaderWriter SZ_TO_LPS_A(LPCSTR sz);
	static MAKFC_CReaderWriter SZ_TO_LPS_W(LPCWSTR sz);
	static MAKFC_CReaderWriter SZ_TO_LPS_T(LPCTSTR sz);
	static MAKFC_CReaderWriter DWORD_TO_LPS(DWORD dwValue);
	static MAKFC_CReaderWriter DWORDLPS_TO_LPS(DWORD dwValue);
};