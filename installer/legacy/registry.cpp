#include "stdafx.h"

#include "registry.h"

/**************************************************/
//  Get key value from registry
//
//  Input:
//    hTopKey        - top registry section
//    szSubKey       - full registry key path under top key
//    szValueName    - value name in specific key
//    szValueReturn  - buffer for key value returning
//  
/**************************************************/

LRESULT makfc_REG_GetValueData(HKEY hTopKey, LPCTSTR szSubKey, LPCTSTR szValueName, LPBYTE szValueReturn)
{
	DWORD dwDataSize = 1;
	BOOL  bSuccess = FALSE;
	HKEY  hKey = NULL;
	
	if (RegOpenKeyEx(hTopKey, szSubKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		RegQueryValueEx(hKey, szValueName, NULL, NULL, NULL, &dwDataSize);
		if (RegQueryValueEx(hKey, szValueName, NULL, NULL, szValueReturn, &dwDataSize) == ERROR_SUCCESS)
		{
			bSuccess = TRUE;
		}
	}
	RegCloseKey(hKey);
	return bSuccess;
}

LRESULT makfc_REG_GetNValueData(HKEY hTopKey, LPCTSTR szSubKey, LPCTSTR szValueName, LPBYTE szValueReturn, DWORD dwSize)
{
	DWORD dwDataSize = 1;
	BOOL  bSuccess = FALSE;
	HKEY  hKey = NULL;
	
	if (RegOpenKeyEx(hTopKey, szSubKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		RegQueryValueEx(hKey, szValueName, NULL, NULL, NULL, &dwDataSize);
		if (dwDataSize <= dwSize && 
			RegQueryValueEx(hKey, szValueName, NULL, NULL, szValueReturn, &dwDataSize) == ERROR_SUCCESS)
		{
			bSuccess = TRUE;
		}
	}
	RegCloseKey(hKey);
	return bSuccess;
}

LRESULT makfc_REG_GetLongData(HKEY hTopKey, LPCTSTR szSubKey, LPCTSTR szValueName, LPDWORD szValueReturn)
{
	DWORD dwDataSize = sizeof(DWORD);
	BOOL  bSuccess = FALSE;
	HKEY  hKey;
	
	if (RegOpenKeyEx(hTopKey, szSubKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		if (RegQueryValueEx(hKey, szValueName, NULL, NULL, (LPBYTE)szValueReturn, &dwDataSize) == ERROR_SUCCESS)
		{
			bSuccess = TRUE;
		}
	}
	RegCloseKey(hKey);
	return bSuccess;
}

BOOL makfc_REG_GetBoolData(HKEY hTopKey, LPCTSTR szSubKey, LPCTSTR szValueName, BOOL defaultValue)
{
	DWORD tmp;
	if (!makfc_REG_GetLongData(hTopKey, szSubKey, szValueName, &tmp))
	{
		return defaultValue;
	}
	return (tmp != 0);
}

/**************************************************/
//  Set key value into registry
//
//  Input:
//    hTopKey   - top registry section
//    szSubKey  - full registry key path under top key
//    szValueName  - value name in specific key
//    szValueData  - value data for setting
//  
/**************************************************/

LRESULT makfc_REG_SetValueData(HKEY hTopKey, LPCTSTR szSubKey, LPCTSTR szValueName, LPCTSTR szValueData)
{
	HKEY  hKey;
	LONG  lResult;
	BOOL  bSuccess  = 0;
	DWORD  disp;

	if (RegCreateKeyEx(hTopKey, szSubKey, 0, _T(""), REG_OPTION_NON_VOLATILE, 
					   KEY_ALL_ACCESS, NULL, &hKey, &disp) == ERROR_SUCCESS) 
	{
		lResult = RegSetValueEx(hKey, szValueName, 0, REG_SZ, (LPBYTE)szValueData, (_tcslen(szValueData)+1) * sizeof(TCHAR));
		bSuccess = (lResult == ERROR_SUCCESS ? TRUE : FALSE);
	}
	RegCloseKey(hKey);
	return bSuccess;
}
/*************************************************/
//Get value data from set with number uNumber

BOOL makfc_REG_GetValueNumber(HKEY hKey, LPCTSTR szSubKey, LPTSTR szValueName, LPDWORD dwNameSize,
							  LPVOID lpValueData, LPDWORD dwValueSize, DWORD uNumber)
{
	BOOL ret = 0;
	HKEY hKeysSet = NULL;	 
	TCHAR * szName = NULL;
	TCHAR * data = NULL;
	DWORD count = 0;
	DWORD keys = 0, NameLength = 0, DataLength = 0;

	szName = (TCHAR*)malloc((*dwNameSize) * sizeof(TCHAR));
	if (!szName)
		return ret;
	data = (TCHAR*)malloc((*dwValueSize));
	if (!data)
		return ret;
	if (RegOpenKeyEx(hKey, szSubKey, 0, KEY_ALL_ACCESS, &hKeysSet) == ERROR_SUCCESS)
	{
		LONG regret;
		if (RegQueryInfoKey(hKeysSet,0,0,0,0,0,0,&keys,0,0,0,0) == ERROR_SUCCESS
			&& keys && keys > uNumber)
		{
			NameLength = *dwNameSize;
			DataLength = *dwValueSize;
			while (count <= uNumber &&
				((regret = RegEnumValue(hKeysSet, count, szName, &NameLength, 0, 0,
										(BYTE*)data, &DataLength)) == ERROR_SUCCESS))
			{
				if (count == uNumber && regret == ERROR_SUCCESS)
				{
					ret = 1;
					memcpy(szValueName, szName, NameLength + sizeof(TCHAR));
					memcpy(lpValueData, data, DataLength);
					*dwValueSize = DataLength;
					*dwNameSize = NameLength;
					break;
				}
				DataLength = *dwValueSize;
				NameLength = *dwNameSize;
				count++;
			}
		}
		RegCloseKey(hKeysSet);
	}
	free(data);
	free(szName);
	if (!ret) 
	{
		*dwValueSize = 0; 
		*dwNameSize = 0;
	}
	return ret;
}

/*BOOL makfc_REG_SetFirstInValuesSet(HKEY hKey, LPCTSTR szSubKey, LPCTSTR szValueName,  LPDWORD lpdwNameSize, LPVOID  lpValueData, LPDWORD lpdwValueSize)
{
	BOOL ret = 0;
	HKEY hKeysSet = NULL;
	FILETIME ft;
	TCHAR regvalue[MAX_PATH];
	DWORD bufv = MAX_PATH;
	TCHAR data[4096];
	DWORD dataLength = 4096;
	UINT regcnt =0;
	if (RegCreateKeyEx(hKey,szSubKey,0,0,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hKeysSet, NULL) ==  ERROR_SUCCESS)
	{	
		if (RegSetValueEx(hKeysSet,szValueName,0,REG_BINARY,(BYTE*)lpValueData,*lpdwValueSize) ==ERROR_SUCCESS)
		{
			RegCloseKey(hKeysSet);
			if (RegOpenKeyEx(hKey,szSubKey,0, KEY_ALL_ACCESS,  &hKeysSet) ==  ERROR_SUCCESS)
			{	
				DWORD values = 0;
				RegQueryInfoKey(hKeysSet,0,0,0,0,0,0,&values,0,0,0,&ft);
				while (RegEnumValue(hKeysSet,0, regvalue,&bufv,NULL,NULL, (BYTE*)data, &dataLength) != ERROR_NO_MORE_ITEMS && regcnt<values)
				{
					if (_tcscmp(regvalue,szValueName)!=0 )
					{
						RegDeleteValue(hKeysSet,regvalue);
						RegSetValueEx(hKeysSet,regvalue,0,REG_BINARY,(BYTE*)data,dataLength);
						dataLength = 4096;
						bufv = MAX_PATH;
						regcnt++;
					}
					else
					{	ret = 1;
						break;
					}
				}
				RegCloseKey(hKeysSet);
			}
		}
	}
	return ret;
}  */
/*************************************************/
//Set binary data
LRESULT makfc_REG_SetNValueData(HKEY hTopKey, LPCTSTR szSubKey, LPCTSTR szValueName, 
								LPCVOID lpValueData, DWORD dwSize)
{
	HKEY  hKey = NULL;
	LONG  lResult = 0;
	BOOL  bSuccess = FALSE;
	DWORD disp = 0;

	if ((lpValueData && dwSize || !dwSize) && RegCreateKeyEx(hTopKey, szSubKey, 0, _T(""), REG_OPTION_NON_VOLATILE, 
					   KEY_ALL_ACCESS, NULL, &hKey, &disp) == ERROR_SUCCESS) 
	{
		lResult = RegSetValueEx(hKey, szValueName, 0, REG_BINARY, (BYTE*)lpValueData, dwSize);
		bSuccess = (lResult == ERROR_SUCCESS ? TRUE : FALSE);
	}
	RegCloseKey(hKey);
	return bSuccess;
}

/*************************************************/
//Set DWORD data

LRESULT makfc_REG_SetLongData(HKEY hTopKey, LPCTSTR szSubKey, LPCTSTR szValueName, DWORD dwValueData)
{
	HKEY  hKey;
	LONG  lResult;
	BOOL  bSuccess = FALSE;
	DWORD disp;

	if (RegCreateKeyEx(hTopKey, szSubKey, 0, _T(""), REG_OPTION_NON_VOLATILE, 
					   KEY_ALL_ACCESS, NULL, &hKey, &disp) == ERROR_SUCCESS) 
	{
		lResult = RegSetValueEx(hKey, szValueName, 0, REG_DWORD, (LPBYTE) &dwValueData, sizeof(DWORD) );
		bSuccess = (lResult == ERROR_SUCCESS ? TRUE : FALSE);
	}
	RegCloseKey(hKey);
	return bSuccess;
}

/**************************************************/
//  Delete specific value from registry
//
//  Input:
//    hTopKey   - top registry section
//    szSubKey  - full registry key path under top key
//    szValueName  - value name in specific key
//  
/**************************************************/

LRESULT makfc_REG_DeleteValue(HKEY hTopKey, LPCTSTR szSubKey, LPCTSTR szValueName)
{
	HKEY  hKey;
	LONG  lResult;
	BOOL  bSuccess = FALSE;

	if (RegOpenKeyEx(hTopKey, szSubKey, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
	{
		lResult = RegDeleteValue(hKey, szValueName);
		bSuccess = (lResult == ERROR_SUCCESS ? TRUE : FALSE);
	}
	RegCloseKey(hKey);

	return bSuccess;
}

LRESULT makfc_REG_DeleteKey(HKEY hTopKey, LPCTSTR szPath, LPCTSTR szKey)
{
	HKEY  hKey;
	LONG  lResult;
	BOOL  bSuccess = FALSE;

	if (RegOpenKeyEx(hTopKey, szPath, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
	{
		lResult = RegDeleteKey(hKey, szKey);
		bSuccess = (lResult == ERROR_SUCCESS ? TRUE : FALSE);
	}
	RegCloseKey(hKey);

	return bSuccess;
}

LRESULT makfc_REG_SetStringData(HKEY hTopKey, LPCTSTR szSubKey, LPCTSTR szValue, MAKFC_CString sValue)
{
	HKEY  hKey = NULL;
	LONG  lResult = 0;
	BOOL  bSuccess = FALSE;
	DWORD disp = 0;

	if (RegCreateKeyEx(hTopKey, szSubKey, 0, _T(""), REG_OPTION_NON_VOLATILE, 
		KEY_ALL_ACCESS, NULL, &hKey, &disp) == ERROR_SUCCESS) 
	{
		lResult = RegSetValueEx(hKey, szValue, 0, REG_SZ, sValue.Net(), sValue.NetSize());
		bSuccess = (lResult == ERROR_SUCCESS ? TRUE : FALSE);
	}
	RegCloseKey(hKey);
	return bSuccess;
}

MAKFC_CString makfc_REG_GetStringData(HKEY hTopKey, LPCTSTR szValue, MAKFC_CString sDefault)
{
	MAKFC_CString sResult;
	DWORD nType;
	DWORD nSize;

	if (ERROR_SUCCESS != ::RegQueryValueEx(hTopKey, szValue, 0, &nType, 
		0, &nSize)
		|| nSize <= 0 || nType != REG_SZ)
	{
		return sDefault;
	}

	if (ERROR_SUCCESS != ::RegQueryValueEx(hTopKey, szValue, 0, &nType, 
		(BYTE *)sResult.GetBuffer(nSize), &nSize) || nSize <= 0 || nType != REG_SZ)
	{
		return sDefault;
	}

	sResult.ReleaseBuffer();

	return sResult;
}

MAKFC_CString makfc_REG_GetStringData(HKEY hTopKey, LPCWSTR szSubKey, LPCWSTR szValue, MAKFC_CString sDefault)
{
	assert(hTopKey != NULL);
	assert(szSubKey != NULL);
	assert(szValue != NULL);
	assert(szSubKey[0] != L'\0');
	assert(szValue[0] != L'\0');

	HKEY hKey = NULL;
	if (RegOpenKeyEx(hTopKey, szSubKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
	{
		return sDefault;	
	}

	DWORD nType = 0;
	DWORD nSize = 0;
	if (ERROR_SUCCESS != ::RegQueryValueEx(hKey, szValue, 0, &nType, 0, &nSize) || nSize <= 0 || nType != REG_SZ)
	{
		RegCloseKey(hKey);
		return sDefault;
	}

	MAKFC_CString sResult;
	if (ERROR_SUCCESS != ::RegQueryValueEx(hKey, szValue, 0, &nType, (BYTE *)sResult.GetBuffer(nSize), &nSize) || nSize <= 0 || nType != REG_SZ)
	{
		RegCloseKey(hKey);
		return sDefault;
	}

	RegCloseKey(hKey);
	sResult.ReleaseBuffer();
	return sResult;
}

LRESULT makfc_REG_GetBinaryData(HKEY hTopKey, LPTSTR szSubKey, LPTSTR szValueName, TCHAR *data, int size)
{
	ULONG size1;
	BOOL  bSuccess = FALSE;
	HKEY  hKey;
	
	if (RegOpenKeyEx(hTopKey, szSubKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		size1 = size;
		if (RegQueryValueEx(hKey, szValueName, NULL, (ULONG*)REG_BINARY, (BYTE*)data, &size1) == ERROR_SUCCESS)
		{
			bSuccess = TRUE;
		}
	}
	RegCloseKey(hKey);
	return bSuccess;
}

LRESULT makfc_REG_SetBinaryData(HKEY hTopKey, LPCTSTR szSubKey, LPCTSTR szValueName, TCHAR *data, int size)
{
	HKEY  hKey;
	LONG  lResult;
	BOOL  bSuccess = FALSE;
	DWORD disp;

	if (RegCreateKeyEx(hTopKey, szSubKey, 0, _T(""), REG_OPTION_NON_VOLATILE, 
					   KEY_ALL_ACCESS, NULL, &hKey, &disp) == ERROR_SUCCESS) 
	{
		lResult = RegSetValueEx(hKey, szValueName, 0, REG_BINARY, (BYTE*)data, size );
		bSuccess = (lResult == ERROR_SUCCESS ? TRUE : FALSE);
	}
	RegCloseKey(hKey);
	return bSuccess;
}

BOOL makfc_REG_SetRect(HKEY hTopKey, LPCTSTR szPath, LPCTSTR szKey, RECT r)
{
	TCHAR s[1024];
	_stprintf(s, _T("%d %d %d %d"), r.left, r.top, r.right, r.bottom);
	return makfc_REG_SetValueData(hTopKey, szPath, szKey, s); 
}




// get a value in single line
DWORD makfc_REG_GetDWORDValue(HKEY hTopKey, LPCTSTR szSubKey, LPCTSTR szValueName, DWORD dwDefaultValue)
{																				
	DWORD dwResult = dwDefaultValue;
	makfc_REG_GetLongData(hTopKey, szSubKey, szValueName, &dwResult);
	return dwResult;
}

// just an alias - to make pair for getter function
LRESULT makfc_REG_SetDWORDValue(HKEY hTopKey, LPCTSTR szSubKey, LPCTSTR szValueName, DWORD dwValueData)
{	
	return makfc_REG_SetLongData(hTopKey, szSubKey, szValueName, dwValueData);	// just alias
}
