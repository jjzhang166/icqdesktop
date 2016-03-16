#pragma once

#include "string.h"

class MAKFC_CBufferInputStream;
class MAKFC_CBufferOutputStream;

class MAKFC_CContactInfo
{
public:
	MAKFC_CString country;
	MAKFC_CString region;
	MAKFC_CString city;
	MAKFC_CString street;
	MAKFC_CString birth_day;
	MAKFC_CString birth_month;
	MAKFC_CString zodiac;
	MAKFC_CString domain;
	MAKFC_CString nickname;
	MAKFC_CString firstname;
	MAKFC_CString lastname;
	MAKFC_CString sex;
	MAKFC_CString phone;
	MAKFC_CString birthday; //age from
	MAKFC_CString date; //age to
	MAKFC_CString online;
	MAKFC_CString camera;
	MAKFC_CString dating;
	MAKFC_CString uafeatures;
	MAKFC_CString status_uri;
	MAKFC_CString status_title;
	MAKFC_CString status_desc;
#define RCI2_ELEMENTS 21 //21 element to save to disk
	MAKFC_CString email;
#define RCI2_ELEMENTS2 22 //22 element to save to disk
	DWORD dwInfoStatus;
#define RCI2_ELEMENTS3 24 //22 element to save to disk
	MAKFC_CString keyword;
	DWORD nToSkip;

	MAKFC_CContactInfo();
	MAKFC_CContactInfo(const MAKFC_CContactInfo &rhs);
	MAKFC_CContactInfo& operator =(const MAKFC_CContactInfo &rhs);
	virtual ~MAKFC_CContactInfo();

	virtual int unserialize(MAKFC_CBufferInputStream *bis);
	virtual int serialize(MAKFC_CBufferOutputStream *bos);
	DWORD GetDaysToBirthday() const;
	MAKFC_CString GetDisplayName() const;

	const MAKFC_CString& GetStatus() const;
	void SetStatus(const MAKFC_CString &status);

	const MAKFC_CString& GetUser() const;
	void SetUser(const MAKFC_CString &user);

private:
	MAKFC_CString _status;
	MAKFC_CString _user;

};

struct MAKFC_CContactInfoShort
{
	MAKFC_CString	m_sEmail;
	MAKFC_CString	m_sNick;
	MAKFC_CString	m_sBlogStatus;
};
