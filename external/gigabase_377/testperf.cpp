//-< TESTPERF.CPP >--------------------------------------------------*--------*
// GigaBASE                  Version 1.0         (c) 1999  GARRET    *     ?  *
// (Post Relational Database Management System)                      *   /\|  *
//                                                                   *  /  \  *
//                          Created:     10-Dec-98    K.A. Knizhnik  * / [] \ *
//                          Last update: 20-Jan-99    K.A. Knizhnik  * GARRET *
//-------------------------------------------------------------------*--------*
// Performance test for index and sequential searches
//-------------------------------------------------------------------*--------*

#include "gigabase.h"
#include <stdio.h>

#ifdef SUPPORT_DATA_ENCRYPTION
#include "crypt/crypt_file.h"
#endif

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
    int nThreads = 1;
    char_t buf[32];

    if (argc > 1) {
        nThreads = atoi(argv[1]);
    }
    dbDatabase db(dbDatabase::dbAllAccess, 5*1024); // 40Mb page pool
#ifdef SUPPORT_DATA_ENCRYPTION 
    dbCryptFile cf("KEY");
    int rc = cf.open(_T("testperf.dbs"), dbFile::no_buffering);
    if (rc != dbFile::ok) { 
        char_t buf[256];
        cf.errorText(rc, buf, itemsof(buf));
        FPRINTF(stderr, _T("Failed to open encrupted file: %s\n"), buf);
        return EXIT_FAILURE;
    }
    if (db.open(&cf)) {
#else
    if (db.open(_T("testperf.dbs"))) {
#endif
        db.setConcurrency(nThreads);
        dbQuery q;
        dbCursor<Record> cursor;
        q = _T("key="),buf;
        nat8 key = 1999;
        nat8 sum = 0;
        time_t start = time(NULL);
        for (i = 0; i < nRecords; i++) {
            Record rec;
            key = (3141592621u*key + 2718281829u) % 1000000007u;
            sum += key;
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
            n = cursor.select(q);
            assert(n == 1);
            assert(STRCMP(cursor->key, buf) == 0);
        }
        printf("Elapsed time for %d index searches: %d seconds\n",
               nRecords, int(time(NULL) - start));
        q = _T("value=key");
        start = time(NULL);
        for (i = 0; i < nSequentialSearches; i++) {
            n = cursor.select(q);
            assert(n == nRecords);
            do {
                n -= 1;
            } while (cursor.next());
            assert(n == 0);
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
#if 0 // it takes too much time
        strcpy(buf, cursor->key);
        for (i = nRecords; --i != 0;) {
            char const* curr = cursor.next()->key;
            assert(strcmp(curr, buf) > 0);
            strcpy(buf, curr);
        }
#endif
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




