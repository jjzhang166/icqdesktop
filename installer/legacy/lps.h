/* 
* Copyright (C) 2001-2003 Mail.Ru 
* 
*/

#ifndef _MAKFC_LPS
#define _MAKFC_LPS


#define LPSLENGTH(s) (*((unsigned long *)(s)))
#define LPSSIZE(s) (LPSLENGTH(s) + sizeof(unsigned long))

char *lps2szA(char *sz, const BYTE *lps);
wchar_t *lps2szW(wchar_t *sz, const BYTE *lps);

wchar_t *lps2szAtoW(wchar_t *szw, const BYTE *lps, UINT uiCodePage = CP_ACP);
char *lps2szWtoA(char *sz, const BYTE *lps, UINT uiCodePage = CP_ACP);

TCHAR *lps2szAtoT(TCHAR *szw, const BYTE *lps, UINT uiCodePage = CP_ACP);
TCHAR *lps2szWtoT(TCHAR *sz, const BYTE *lps, UINT uiCodePage = CP_ACP);

BYTE *sz2lpsA(BYTE *lps, const char *sz);
BYTE *sz2lpsW(BYTE *lps, const wchar_t *sz);

char *mkszA(const BYTE *lps);
wchar_t *mkszW(const BYTE *lps);

wchar_t *mkszAtoW(const BYTE *lps, UINT uiCodePage = CP_ACP);
char *mkszWtoA(const BYTE *lps, UINT uiCodePage = CP_ACP);

TCHAR *mkszAtoT(const BYTE *lps, UINT uiCodePage = CP_ACP);
TCHAR *mkszWtoT(const BYTE *lps, UINT uiCodePage = CP_ACP);

BYTE *mklpsA(const char *sz);
BYTE *mklpsW(const wchar_t *sz);
BYTE *mklpsT(const TCHAR *sz);

BYTE *lpsdup(const BYTE *lps_src);

BYTE *lpscatszA(BYTE **lps_dest, const char *sz_src);
BYTE *lpscatszW(BYTE **lps_dest, const wchar_t *sz_src);

int IsValidLPS(const BYTE *lps);

class MAKFC_CInputStream;
class MAKFC_COutputStream;

class MAKFC_CString;
class MAKFC_CLPS
{
protected:
	int size;
	BYTE *buffer;
	BOOL m_bUnicode;
public:
	MAKFC_CLPS();
	virtual ~MAKFC_CLPS();
	MAKFC_CLPS(const char *str);
	MAKFC_CLPS(const wchar_t *str);
	MAKFC_CLPS(const BYTE *data, int data_size);
	MAKFC_CLPS(const int val);
	MAKFC_CLPS(const MAKFC_CLPS &lps);
//	MAKFC_CLPS(const MAKFC_CString &s);

	int getsize();
	int getDataSize() const;
	BYTE * getdata();
	const BYTE * getdatac() const;
	BYTE * getLPS();
	void clear();
	bool empty() const;

	void operator=(const MAKFC_CLPS lps2);
	int operator==(const MAKFC_CLPS lps2);

	int Serialize(const byte* lps);
	int Serialize(MAKFC_CInputStream *is);
	int Serialize(MAKFC_COutputStream *is);

	operator BYTE*()
	{
		return buffer;
	}

	MAKFC_CString ToStringA(UINT uiCodePage = CP_ACP) const;
	MAKFC_CString ToStringW(UINT uiCodePage = CP_ACP) const;
};

void GetLpsX(BYTE *lpstr, char xor_char);

#endif // _MAKFC_LPS
