#include "stdafx.h"

#include "http_login_session.h"
#include "gbase.h"
#include <atltime.h>
#include "login_data.h"

#include "gbase.h"

#define ICQ_DEVICE_ID L"ICQ_DEVICE_ID"
#define ICQ_AOL_DEVICE L"ICQ_A_DEVICE"
#define ICQ_SESSION_KEY_DATA L"ICQ_SESSION_KEY_DATA"
#define ICQ_AOL_TOKEN_EXP_DATE L"ICQ_TOKEN_EXP_DATE"
#define ICQ_PARTNERS_LOGIN_DATA L"ICQ_LOGIN_DATA_PARTNERS"
#define ICQ_HTTP_TIME_OFFSET L"ICQ_HTTP_TIME_OFFSET"
#define WIM_DEV_ID L"ic1nmMjqg7Yu-0hL"

static MAKFC_CString gigabaseByteArrayToString(const gigabase::dbArray<gigabase::byte>& param)
{
    wchar_t tmp[1024];
    ::memset(tmp, 0, sizeof(tmp));
    ::memcpy(tmp, param.get(), param.length());
    MAKFC_CString result(tmp);
    return result;
}

bool LoadExported(
    IN MRABase& _db,
    IN MAKFC_CString sKey, 
    OUT MAKFC_CString& devID, 
    OUT MAKFC_CString& aolToken, 
    OUT MAKFC_CString& sessionKey, 
    OUT time_t& expirationDate, 
    OUT __int64& time_offset)
{
    gigabase::dbArray<gigabase::byte> data1, data2, data3;

    if ( !_db.ReadRawData( (LPCTSTR) sKey, L"", ICQ_DEVICE_ID, data1) || !data1.length() )
        return false;

    if ( !_db.ReadRawData( (LPCTSTR) sKey, L"", ICQ_AOL_DEVICE, data2) || !data2.length() )
        return false;

    if ( !_db.ReadRawData( (LPCTSTR) sKey, L"", ICQ_SESSION_KEY_DATA, data3) || !data3.length())
        return false;

    CString a1, a2, a3;
    a1 = gigabaseByteArrayToString(data1);
    a2 = gigabaseByteArrayToString(data2);
    a3 = gigabaseByteArrayToString(data3);

    // session key may be empty, do not check it for emptiness please
    if (a1.IsEmpty() || a2.IsEmpty() || a3.IsEmpty())
        return false;

    time_t defaultExpDate = ::time(0) + 365 * 24 * 60 * 60;

    DWORD tmpExpDate = defaultExpDate;
    if (!_db.ReadDWORD( (LPCTSTR) sKey, L"", ICQ_AOL_TOKEN_EXP_DATE, tmpExpDate ))
    {
        expirationDate = defaultExpDate;
    }
    else
    {
        expirationDate = static_cast<time_t>(tmpExpDate);
    }

    devID = a1;
    aolToken = a2;
    sessionKey = a3;


    time_offset = 0;
    gigabase::dbArray<gigabase::byte> time_data;
    if (_db.ReadRawData( (LPCTSTR) sKey, L"", ICQ_HTTP_TIME_OFFSET, time_data ) )
    {
        if ( time_data.length() != sizeof( __int64 ) )
            memcpy( &time_offset, time_data.get(), sizeof( __int64 ) );
    }

    return true;
}

#define UNIXTIME_MINUS_FILETIME_DIFF_IN100NS 116444736000000000

void FileTimeToUnixTime(OUT time_t *pt, IN FILETIME ft)
{
    LONGLONG ll;

    ll = (LONGLONG) ft.dwHighDateTime;
    ll <<= 32;
    ll +=  ft.dwLowDateTime;
    ll = (ll  - UNIXTIME_MINUS_FILETIME_DIFF_IN100NS);
    ll /= 10000000;
    *pt = (DWORD) ll;
}


bool LoadSessionData(
    IN MRABase& _db,
    IN MAKFC_CString databaseKey, 
    OUT MAKFC_CString& sDevId, 
    OUT MAKFC_CString& sA, 
    OUT MAKFC_CString& sSessionKey, 
    OUT MAKFC_CString& ts)
{
    gigabase::dbArray<gigabase::byte> data;
    if ( !_db.ReadRawData( tstring(databaseKey.NetStrW()), L"", ICQ_PARTNERS_LOGIN_DATA, data ) )
    {
        return false;
    }

    MAKFC_CString sCookies = (LPCWSTR) data.get();
    if ( !sCookies.Len() )
        return false;

    int iStart1 = 0;
    MAKFC_CString sToken1;

    while ( ( sToken1 = sCookies.Tokenize( L";", iStart1 ) ).GetLength() )
    {
        int iStart2 = 0;
        MAKFC_CString sLeft = sToken1.Tokenize( L"=", iStart2 );
        MAKFC_CString sRight = sToken1.Mid( iStart2 );

        if ( sLeft == L"k" )
            sDevId = sRight;
        else if ( sLeft == L"a" )
            sA = sRight;
        else if ( sLeft == L"session_key" )
            sSessionKey = sRight;
        else if ( sLeft == L"time_offset" )
        {
            __int64 offset = _wtoi64( sRight );
            time_t tt = 0;
            FileTimeToUnixTime( &tt, CFileTime( CFileTime::GetCurrentTime().GetTime() - offset ) );
            ts.Format( L"%d", (DWORD) tt );
        }
    }

    return true;
}

MAKFC_CString GetProtocolName()
{
    return L"ICQ";
}


MAKFC_CString GenerateKeyName(const MAKFC_CString &sLogin)
{
    if ( MAKFC_CString::IsUin( sLogin ) )
    {
        return sLogin;
    }

    auto sResult = (GetProtocolName() + L"_" + sLogin);
    
    return sResult;
}


MAKFC_CString GenerateKeyName(const MAKFC_CLoginData::TLoginType loginType, const MAKFC_CString &sLogin)
{
    MAKFC_CString sResult;

    sResult = GenerateKeyName( MAKFC_CLoginData::GetLoginTypeName(loginType) + L"_" + sLogin );

    return sResult;
}


MAKFC_CString GetDatabaseKey(const MAKFC_CLoginData& _ld)
{
    return GenerateKeyName(_ld.GetLoginType(), _ld.GetLogin() );
}

std::shared_ptr<wim_auth_parameters> load_auth_params_from_db(IN MRABase& _db, const MAKFC_CLoginData& _ld, const MAKFC_CString& _database_key)
{
    MAKFC_CString uin = _ld.GetLogin();

    auto auth_params = std::make_shared<wim_auth_parameters>();

    auth_params->m_dev_id = WIM_DEV_ID;

    time_t currentTime = ::time(0);
    const time_t nDefaultExpiresTime = currentTime + 24 * 60 * 60;
    auth_params->m_exipired_in = nDefaultExpiresTime;

    bool readSessionOk = LoadExported( 
        _db,
        _database_key, 
        auth_params->m_dev_id, 
        auth_params->m_a_token, 
        auth_params->m_session_key, 
        auth_params->m_exipired_in,
        auth_params->m_time_offset );

    if ( !readSessionOk )
    {
        MAKFC_CString key_old = _database_key;
        key_old.Replace( L"phone_", L"" );

        readSessionOk = LoadExported(
            _db,
            key_old, 
            auth_params->m_dev_id, 
            auth_params->m_a_token, 
            auth_params->m_session_key, 
            auth_params->m_exipired_in,
            auth_params->m_time_offset );

        if ( !readSessionOk )
        {
            readSessionOk = LoadExported( 
                _db,
                uin, 
                auth_params->m_dev_id, 
                auth_params->m_a_token, 
                auth_params->m_session_key, 
                auth_params->m_exipired_in,
                auth_params->m_time_offset );
        }
    }

    if (!readSessionOk)
        auth_params.reset();

    return auth_params;
}


std::shared_ptr<wim_auth_parameters> load_auth_params_from_db(IN MRABase& _db, const MAKFC_CLoginData& _ld)
{
    MAKFC_CString database_key = GetDatabaseKey(_ld);
    MAKFC_CString database_key2 = database_key;
    
    if (_ld.GetLoginType() == MAKFC_CLoginData::TLoginType::LT_Phone)
        database_key.Replace(L"+", L"");

    auto result = load_auth_params_from_db(_db, _ld, database_key);
    if (!result && _ld.GetLoginType() == MAKFC_CLoginData::TLoginType::LT_Phone)
        result = load_auth_params_from_db(_db, _ld, database_key2);
    
    return result;    
}
