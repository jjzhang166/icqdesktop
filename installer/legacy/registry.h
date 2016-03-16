#ifndef _MAKFC_REGKEY
#define _MAKFC_REGKEY

#include "string.h"

LRESULT makfc_REG_GetValueData(HKEY hTopKey, LPCTSTR szSubKey, LPCTSTR szValueName, LPBYTE szValueReturn);
LRESULT makfc_REG_GetNValueData(HKEY hTopKey, LPCTSTR szSubKey, LPCTSTR szValueName, LPBYTE szValueReturn, DWORD dwSize);
LRESULT makfc_REG_GetLongData(HKEY hTopKey, LPCTSTR szSubKey, LPCTSTR szValueName, LPDWORD szValueReturn);
DWORD makfc_REG_GetDWORDValue(HKEY hTopKey, LPCTSTR szSubKey, LPCTSTR szValueName, DWORD dwDefaultValue);
BOOL makfc_REG_GetBoolData(HKEY hTopKey, LPCTSTR szSubKey, LPCTSTR szValueName, BOOL defaultValue = FALSE);
LRESULT makfc_REG_SetValueData(HKEY hTopKey, LPCTSTR szSubKey, LPCTSTR szValueName,LPCTSTR szValueData);
BOOL makfc_REG_GetValueNumber(HKEY hKey, LPCTSTR szSubKey, LPTSTR szValueName, LPDWORD dwNameSize, LPVOID lpValueData, LPDWORD dwValueSize, DWORD uNumber);
BOOL makfc_REG_SetFirstInValuesSet(HKEY hKey, LPCTSTR szSubKey, LPCTSTR szValueName, LPDWORD lpdwNameSize, LPVOID lpValueData, LPDWORD lpdwValueSize);
LRESULT makfc_REG_SetNValueData(HKEY hTopKey, LPCTSTR szSubKey, LPCTSTR szValueName,LPCVOID lpValueData, DWORD dwSize);
LRESULT makfc_REG_SetLongData(HKEY hTopKey, LPCTSTR szSubKey, LPCTSTR szValueName, DWORD dwValueData);
LRESULT makfc_REG_SetDWORDValue(HKEY hTopKey, LPCTSTR szSubKey, LPCTSTR szValueName, DWORD dwValueData);
LRESULT makfc_REG_DeleteValue(HKEY hTopKey, LPCTSTR szSubKey, LPCTSTR szValueName);
LRESULT makfc_REG_DeleteKey(HKEY hTopKey, LPCTSTR szPath, LPCTSTR szKey);
LRESULT makfc_REG_SetStringData(HKEY hTopKey, LPCTSTR szPath, LPCTSTR szValue, MAKFC_CString sValue);
MAKFC_CString makfc_REG_GetStringData(HKEY hTopKey, LPCTSTR szValue, MAKFC_CString sDefault = L"");
MAKFC_CString makfc_REG_GetStringData(HKEY hTopKey, LPCWSTR szSubKey, LPCWSTR szValue, MAKFC_CString sDefault);

BOOL makfc_REG_SetRect(HKEY hTopKey, LPCTSTR szPath, LPCTSTR szKey, RECT r);

#endif 