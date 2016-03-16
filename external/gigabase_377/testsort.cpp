//-< TESTSORT.CPP >--------------------------------------------------*--------*
// GigaBASE                  Version 1.0         (c) 1999  GARRET    *     ?  *
// (Post Relational Database Management System)                      *   /\|  *
//                                                                   *  /  \  *
//                          Created:     10-Dec-98    K.A. Knizhnik  * / [] \ *
//                          Last update: 19-Dec-98    K.A. Knizhnik  * GARRET *
//-------------------------------------------------------------------*--------*
// Test of records sorting
//-------------------------------------------------------------------*--------*

#include "gigabase.h"

USE_GIGABASE_NAMESPACE

class Address { 
  public:
    char_t const* city;
    char_t const* street;
    int4          building;
    
    TYPE_DESCRIPTOR((FIELD(city), FIELD(street), FIELD(building)));
};

class Person { 
  public:
    char_t const* name;
    dbReference<Address> address;

    TYPE_DESCRIPTOR((FIELD(name), FIELD(address)));
};
    
REGISTER(Address);
REGISTER(Person);

void print(dbCursor<Person>& persons, char_t* title)
{
    dbCursor<Address> addresses;
    PRINTF(_T("%s\n"), title);
    do { 
        addresses.at(persons->address);
        PRINTF(_T("Name=%s\tcity=%s\tstreet=%s\tbuilding=%d\n"), 
               persons->name, addresses->city, addresses->street, addresses->building);
    } while (persons.next()); 
    PRINTF(_T("----------------------------------------------\n"));
}

void print(dbCursor<Address>& addresses, char_t* title)
{
    PRINTF(_T("%s\n"), title);
    do { 
        PRINTF(_T("City=%s\tstreet=%s\tbuilding=%d\n"), 
               addresses->city, addresses->street, addresses->building);
    } while (addresses.next());
    PRINTF(_T("----------------------------------------------\n"));
}

int main() 
{
    dbDatabase db;
    if (db.open(_T("testsort.dbs"))) {
        Address a;
        Person  p;
        a.city =_T("Moscow");
        a.street =_T("Komsomolkyj pr.");
        a.building = 42;
        p.address = insert(a);
        p.name =_T("Andrey");
        insert(p);

        a.city =_T("Istra");
        a.street =_T("Lenina");
        a.building = 17;
        p.address = insert(a);
        p.name =_T("Sergey");
        insert(p);

        a.city =_T("Moscow");
        a.street =_T("Shipilovkyj pr.");
        a.building = 57;
        p.address = insert(a);
        p.name =_T("Konstantin");
        insert(p);

        a.city =_T("Moscow");
        a.street =_T("Shipilovkyj pr.");
        a.building = 57;
        p.address = insert(a);
        p.name =_T("Anna");
        insert(p);

        a.city =_T("Moscow");
        a.street =_T("Vostochnaya");
        a.building = 2;
        p.address = insert(a);
        p.name =_T("Alexandr");
        insert(p);

        a.city =_T("Krasnogorsk");
        a.street =_T("Zelenaya");
        a.building = 11;
        p.address = insert(a);
        p.name =_T("Elena");
        insert(p);

        a.city =_T("Moscow");
        a.street =_T("Vavilova");
        a.building = 101;
        p.address = insert(a);
        p.name =_T("Natasha");
        insert(p);

        dbCursor<Address> addresses;
        dbCursor<Person>  persons;

        persons.select(_T("order by address.city"));
        print(persons, _T("order by address.city"));
        persons.select(_T("order by address.city,address.street"));
        print(persons, _T("order by address.city,address.street"));
        persons.select(_T("order by address.city desc, name asc"));
        print(persons, _T("order by address.city desc, name asc"));
        persons.select(_T("order by name"));
        print(persons, _T("order by name"));
        persons.select(_T("order by name desc"));
        print(persons, _T("order by name desc"));
        addresses.select(_T("order by city,street,building"));
        print(addresses, _T("order by city,street,building"));
        db.close();
    }
    return 0;
}
