//-< TESTSESSION.CPP >-----------------------------------------------*--------*
// GigaBASE                  Version 1.0         (c) 1999  GARRET    *     ?  *
// (Post Relational Database Management System)                      *   /\|  *
//                                                                   *  /  \  *
//                          Created:     10-Dec-98    K.A. Knizhnik  * / [] \ *
//                          Last update: 20-Jan-99    K.A. Knizhnik  * GARRET *
//-------------------------------------------------------------------*--------*
// Example of remote database access using C++ interface
//-------------------------------------------------------------------*--------*

#include "session.h"
#include <stdio.h>

USE_GIGABASE_NAMESPACE

const int nRecords = 1000000;
const int nSequentialSearches = 10;

class Record {
  public:
    char_t const* key;
    char_t const* value;

    TYPE_DESCRIPTOR((KEY(key, INDEXED), FIELD(value)));
};

REGISTER(Record);


int __cdecl main(int argc, char* argv[])
{
    int i, n;
    char_t buf[32];

    dbSession session("localhost:6101");
    dbReference<Record> ref;
    dbQuery q;
    q = _T("key="),buf;
    nat8 key = 1999;
    nat8 sum = 0;
    time_t start = time(NULL);
    Record rec;
    for (i = 0; i < nRecords; i++) {
        key = (3141592621u*key + 2718281829u) % 1000000007u;
        sum += key;
        SPRINTF(SPRINTF_BUFFER(buf), T_INT8_FORMAT _T("."), key);
        rec.key = buf;
        rec.value = buf;
        session.insert(rec);
    }
    printf("Elapsed time for inserting %d record: %d seconds\n",
           nRecords, int(time(NULL) - start));
    start = time(NULL);
    session.commit();
    printf("Commit time: %d\n", int(time(NULL) - start));

    start = time(NULL);
    key = 1999;
    for (i = 0; i < nRecords; i++) {
        key = (3141592621u*key + 2718281829u) % 1000000007u;
        SPRINTF(SPRINTF_BUFFER(buf), T_INT8_FORMAT _T("."), key);
        session.select<Record>(q);
        ref = session.next(rec);
        assert(!ref.isNull());
        assert(STRCMP(rec.key, buf) == 0);
        assert(session.next(rec).isNull());
    }
    printf("Elapsed time for %d index searches: %d seconds\n",
           nRecords, int(time(NULL) - start));
    q = _T("value=key order by value");
    start = time(NULL);
    for (i = 0; i < nSequentialSearches; i++) {
        session.select<Record>(q);
        for (n = 0; !session.next(rec).isNull(); n++);
        assert(n == nRecords);
    }
    printf("Elapsed time for %d sequential search through %d records: "
           "%d seconds\n", nSequentialSearches, nRecords,
           int(time(NULL) - start));
    
    start = time(NULL);
    dbQuery all;
    session.select<Record>(all);
    for (n = 0; !session.next(rec).isNull(); n++);
    assert(n == nRecords);
    printf("Elapsed time for iteration through %d records: %d seconds\n",
           nRecords, int(time(NULL) - start));

    q = _T("value=key");
    start = time(NULL);
    n = (int)session.remove<Record>(q);
    assert(n == nRecords);
    printf("Elapsed time for deleting all %d records: %d seconds\n",
           nRecords, int(time(NULL) - start));
    return EXIT_SUCCESS;    
}




