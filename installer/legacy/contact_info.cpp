#include "stdafx.h"
#include "contact_info.h"
#include "stream.h"


MAKFC_CContactInfo::MAKFC_CContactInfo(void)
{
	dwInfoStatus = 0;
	nToSkip = 0;
}

MAKFC_CContactInfo::MAKFC_CContactInfo(const MAKFC_CContactInfo &rhs)
{
	*this = rhs;
}

MAKFC_CContactInfo& MAKFC_CContactInfo::operator =(const MAKFC_CContactInfo &rhs)
{
	country = rhs.country;
	region = rhs.region;
	city = rhs.city;
	street = rhs.street;
	birth_day = rhs.birth_day;
	birth_month = rhs.birth_month;
	zodiac = rhs.zodiac;
	domain = rhs.domain;
	nickname = rhs.nickname;
	firstname = rhs.firstname;
	lastname = rhs.lastname;
	sex = rhs.sex;
	phone = rhs.phone;
	birthday = rhs.birthday;
	date = rhs.date;
	online = rhs.online;
	camera = rhs.camera;
	dating = rhs.dating;
	uafeatures = rhs.uafeatures;
	status_uri = rhs.status_uri;
	status_title = rhs.status_title;
	status_desc = rhs.status_desc;
	email = rhs.email;
	dwInfoStatus = rhs.dwInfoStatus;
	keyword = rhs.keyword;
	nToSkip = rhs.nToSkip;
	_user = rhs._user;
	SetStatus(rhs.GetStatus());

	return *this;
}

MAKFC_CContactInfo::~MAKFC_CContactInfo(void)
{
}

int MAKFC_CContactInfo::unserialize(MAKFC_CBufferInputStream *bis)
{
	BOOL bRead = FALSE;
	int iElements = bis->readInt(&bRead);
	if (iElements < RCI2_ELEMENTS)
		return FALSE;

	MAKFC_CString status;
	bis->readLpsAsStringW(status);
	SetStatus(status.Trim());

	bis->readLpsAsStringW(country);
	bis->readLpsAsStringW(region);
	bis->readLpsAsStringW(city);
	bis->readLpsAsStringW(street);
	bis->readLpsAsStringW(birth_day);
	bis->readLpsAsStringW(birth_month);
	bis->readLpsAsStringW(zodiac);

	MAKFC_CString user;
	bis->readLpsAsStringW(user);
	SetUser(user);

	bis->readLpsAsStringW(domain);
	bis->readLpsAsStringW(nickname);
	bis->readLpsAsStringW(firstname);
	bis->readLpsAsStringW(lastname);
	bis->readLpsAsStringW(sex);
	bis->readLpsAsStringW(phone);
	bis->readLpsAsStringW(birthday);
	bis->readLpsAsStringW(date);
	bis->readLpsAsStringW(online);
	bis->readLpsAsStringW(camera);
	bis->readLpsAsStringW(dating);
	bis->readLpsAsStringW(uafeatures);
	bis->readLpsAsStringW(status_uri);
	bis->readLpsAsStringW(status_title);
	bis->readLpsAsStringW(status_desc);
	if (iElements < RCI2_ELEMENTS2)
		return TRUE;
	bis->readLpsAsStringW(email);

	if (iElements < RCI2_ELEMENTS3) 
		return TRUE;

	bis->readLpsAsStringW(keyword);
	nToSkip = bis->readDWORD();

	return TRUE;
}
int MAKFC_CContactInfo::serialize(MAKFC_CBufferOutputStream *bos)
{
	bos->writeInt(RCI2_ELEMENTS3);

	bos->writeSzAsLpsW(GetStatus().NetStrW());
	bos->writeSzAsLpsW(country.NetStrW());
	bos->writeSzAsLpsW(region.NetStrW());
	bos->writeSzAsLpsW(city.NetStrW());
	bos->writeSzAsLpsW(street.NetStrW());
	bos->writeSzAsLpsW(birth_day.NetStrW());
	bos->writeSzAsLpsW(birth_month.NetStrW());
	bos->writeSzAsLpsW(zodiac.NetStrW());
	bos->writeSzAsLpsW(GetUser().NetStrW());
	bos->writeSzAsLpsW(domain.NetStrW());
	bos->writeSzAsLpsW(nickname.NetStrW());
	bos->writeSzAsLpsW(firstname.NetStrW());
	bos->writeSzAsLpsW(lastname.NetStrW());
	bos->writeSzAsLpsW(sex.NetStrW());
	bos->writeSzAsLpsW(phone.NetStrW());
	bos->writeSzAsLpsW(birthday.NetStrW()); //age from
	bos->writeSzAsLpsW(date.NetStrW()); //age to
	bos->writeSzAsLpsW(online.NetStrW());
	bos->writeSzAsLpsW(camera.NetStrW());
	bos->writeSzAsLpsW(dating.NetStrW());
	bos->writeSzAsLpsW(uafeatures.NetStrW());
	bos->writeSzAsLpsW(status_uri.NetStrW());
	bos->writeSzAsLpsW(status_title.NetStrW());
	bos->writeSzAsLpsW(status_desc.NetStrW());
	bos->writeSzAsLpsW(email.NetStrW());
	bos->writeSzAsLpsW(keyword.NetStrW());
	bos->writeDWORD(nToSkip);
	return TRUE;
}

DWORD MAKFC_CContactInfo::GetDaysToBirthday() const
{
//	const int daysToBirthday = Date_DaysToBirthday(birthday);
//	assert(daysToBirthday >= -1);

//	return (DWORD)daysToBirthday;

    return 0;
}

MAKFC_CString MAKFC_CContactInfo::GetDisplayName() const
{
	if ( !nickname.IsEmpty() )
	{
		return nickname;
	}

	if ( !firstname.IsEmpty() )
	{
		if ( !lastname.IsEmpty() )
		{
			return (firstname + L" " + lastname);
		}

		return firstname;
	}

	if (!GetUser().IsEmpty())
	{
		if (!domain.IsEmpty())
		{
			return (GetUser() + L"@" + domain);
		}

		return GetUser();
	}

	return L"";
}

const MAKFC_CString& MAKFC_CContactInfo::GetStatus() const
{
	return _status;
}

void MAKFC_CContactInfo::SetStatus(const MAKFC_CString &status)
{
	assert(status.Find(L" ") == -1);
	_status = status;
}

const MAKFC_CString& MAKFC_CContactInfo::GetUser() const
{
	return _user;
}

void MAKFC_CContactInfo::SetUser(const MAKFC_CString &user)
{
	_user = user;
}

