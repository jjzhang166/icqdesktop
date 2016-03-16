//-< TESTINC.CPP >---------------------------------------------------*--------*
// GigaBASE                  Version 1.0         (c) 1999  GARRET    *     ?  *
// (Post Relational Database Management System)                      *   /\|  *
//                                                                   *  /  \  *
//                          Created:     25-Sep-2001  K.A. Knizhnik  * / [] \ *
//                          Last update: 25-Sep-2001  K.A. Knizhnik  * GARRET *
//-------------------------------------------------------------------*--------*
// Test of autoincrement fields support
//-------------------------------------------------------------------*--------*

#include "gigabase.h"
#include <stdio.h>

#ifndef AUTOINCREMENT_SUPPORT
#error To use autoincrement feature you should rebuild GigaBASE with -DAUTOINCREMENT_SUPPORT option
#endif

USE_GIGABASE_NAMESPACE

const int nRecords = 100000;
const int nSequentialSearches = 10;

class Record {
  public:
    int4 rid;
    db_int8 key;

    TYPE_DESCRIPTOR((KEY(rid, AUTOINCREMENT|INDEXED), FIELD(key)));
};

REGISTER(Record);


int main(int argc, char* argv[])
{
    int  i, n;
    int4 rid;

    dbDatabase db(dbDatabase::dbAllAccess, 5*1024); // 40Mb page pool
    if (db.open(_T("testinc.dbs"))) {
        dbQuery q;
        dbCursor<Record> cursor;
        q = _T("rid="),rid;
        nat8 key = 1999;
        time_t start = time(NULL);
        for (i = 0; i < nRecords; i++) {
            Record rec;
            key = (3141592621u*key + 2718281829u) % 1000000007u;
            rec.key = key;
            insert(rec);
            assert(rec.rid == i+1);
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
            rid = i+1;
            n = cursor.select(q);
            assert(n == 1 && cursor->key == key);
        }
        printf("Elapsed time for %d index searches: %d seconds\n",
               nRecords, int(time(NULL) - start));
        start = time(NULL);
        n = cursor.select(dbCursorForUpdate);
        assert(n == nRecords);
        for (i = 0; i < nRecords; i++) {
            assert(cursor->rid == i+1);
            cursor.next();
        }
        printf("Elapsed time for table traversal through %d records: "
               "%d seconds\n", nRecords, int(time(NULL) - start));
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




