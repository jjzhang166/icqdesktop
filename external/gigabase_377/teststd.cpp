//-< TESTSTD.CPP >---------------------------------------------------*--------*
// GigaBASE                  Version 1.0         (c) 1999  GARRET    *     ?  *
// (Post Relational Database Management System)                      *   /\|  *
//                                                                   *  /  \  *
//                          Created:     25-Sep-2001  K.A. Knizhnik  * / [] \ *
//                          Last update: 25-Sep-2001  K.A. Knizhnik  * GARRET *
//-------------------------------------------------------------------*--------*
// Test std::string support
//-------------------------------------------------------------------*--------*

#include "gigabase.h"
#include <stdio.h>
#include <string>

#ifndef USE_STD_STRING
#error GigaBASE should be build with -DUSE_STD_STRING option
#endif

#ifndef NO_NAMESPACES
using namespace std;
#endif

USE_GIGABASE_NAMESPACE

const int nRecords = 100000;
const int nSequentialSearches = 10;

class Record {
  public:
    STD_STRING key;
    STD_STRING value;

    TYPE_DESCRIPTOR((KEY(key, INDEXED), FIELD(value)));
};

REGISTER(Record);


int __cdecl main(int argc, char* argv[])
{
    int i, n;
    int nThreads = 1;
    char_t buf[64];
    STD_STRING val;

    if (argc > 1) {
        nThreads = atoi(argv[1]);
    }
    dbDatabase db(dbDatabase::dbAllAccess, 5*1024); // 40Mb page pool
    if (db.open(_T("teststd.dbs"))) {
        db.setConcurrency(nThreads);
        dbQuery q;
        dbCursor<Record> cursor;
        q = _T("key="),val;
        nat8 key = 1999;
        time_t start = time(NULL);
        for (i = 0; i < nRecords; i++) {
            Record rec;
            key = (3141592621u*key + 2718281829u) % 1000000007u;
            SPRINTF(SPRINTF_BUFFER(buf), T_INT8_FORMAT _T("."), key);
            rec.key = buf;
            rec.value = buf;
            insert(rec);
        }
        printf("Elapsed time for inserting %d record: %d seconds\n",
               nRecords, int(time(NULL) - start));
        start = time(NULL);
        db.commit();
        printf("Commit time: %d\n", int(time(NULL) - start));

        start = time(NULL);
        key = 1999;
        for (i = 0; i < nRecords; i++) {
            key = (3141592621u*key + 2718281829u) % 1000000007u;
            SPRINTF(SPRINTF_BUFFER(buf), T_INT8_FORMAT _T("."), key);
            val = buf;
            n = cursor.select(q);
            assert(n == 1);
        }
        printf("Elapsed time for %d index searches: %d seconds\n",
               nRecords, int(time(NULL) - start));
        q = _T("value=key");
        start = time(NULL);
        for (i = 0; i < nSequentialSearches; i++) {
            n = cursor.select(q);
            assert(n == nRecords);
        }
        printf("Elapsed time for %d sequential search through %d records: "
               "%d seconds\n", nSequentialSearches, nRecords,
               int(time(NULL) - start));

        q = _T("value=key order by value");
        start = time(NULL);
        n = cursor.select(q, dbCursorForUpdate);
        assert(n == nRecords);
        printf("Elapsed time for search with sorting %d records: %d seconds\n",
               nRecords, int(time(NULL)-start));
        start = time(NULL);
        cursor.removeAll();
        printf("Elapsed time for deleting all %d records: %d seconds\n",
               nRecords, int(time(NULL) - start));
        db.close();
        return EXIT_SUCCESS;
    } else {
        printf("Failed to open database\n");
        return EXIT_FAILURE;
    }
}




