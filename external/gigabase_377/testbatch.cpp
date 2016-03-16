//-< TESTBULK.CPP >--------------------------------------------------*--------*
// GigaBASE                  Version 1.0         (c) 1999  GARRET    *     ?  *
// (Post Relational Database Management System)                      *   /\|  *
//                                                                   *  /  \  *
//                          Created:     23-Oct-2002  K.A. Knizhnik  * / [] \ *
//                          Last update: 23-Oct-99    K.A. Knizhnik  * GARRET *
//-------------------------------------------------------------------*--------*
// Test batch loading
//-------------------------------------------------------------------*--------*

#include "gigabase.h"
#include <stdio.h>

USE_GIGABASE_NAMESPACE

const int nRecords = 1000000;

class Record {
  public:
    db_int8       int_key;
    char_t const* str_key;

    TYPE_DESCRIPTOR((KEY(int_key, INDEXED), KEY(str_key, INDEXED)));
};

REGISTER(Record);


int __cdecl main(int argc, char* argv[])
{
    int i;
    char_t buf[32];
    int batchSize = nRecords+1;

    if (argc > 1) {
        batchSize = atoi(argv[1]);
    }
    dbDatabase db(dbDatabase::dbAllAccess, 5*1024); // 40Mb page pool
    if (db.open(_T("testbatch.dbs"))) {
        nat8 key = 2002;
        time_t start = time(NULL);
        for (i = 0; i < nRecords; i++) {
            Record rec;
            key = (3141592621u*key + 2718281829u) % 1000000007u;
            SPRINTF(SPRINTF_BUFFER(buf), T_INT8_FORMAT, key);
            rec.int_key = key;
            rec.str_key = buf;
            if (batchSize > 1) { 
                batchInsert(rec);
                if ((i + 1) % batchSize == 0) { 
                    db.executeBatch();
                }            
            } else { 
                insert(rec);
            }
        }
        printf("Elapsed time for inserting %d record: %d seconds\n",
               nRecords, int(time(NULL) - start));
        start = time(NULL);
        db.commit();
        printf("Commit time: %d seconds\n", int(time(NULL) - start));

#if 0  // takes a lot of time
        dbQuery q1, q2;
        db_int8 int_key;
        dbCursor<Record> cursor;
        q1 = _T("str_key="), buf;
        q2 = _T("int_key="), int_key;
        start = time(NULL);
        key = 2002;
        for (i = 0; i < nRecords; i++) {
            int n;
            key = (3141592621u*key + 2718281829u) % 1000000007u;
            SPRINTF(SPRINTF_BUFFER(buf), T_INT8_FORMAT, key);
            int_key = key;
            n = cursor.select(q1);
            assert(n == 1);
            n = cursor.select(q2);
            assert(n == 1);
        }
        printf("Elapsed time for %d index searches: %d seconds\n",
               nRecords*2, int(time(NULL) - start));
#endif
        db.close();
        return EXIT_SUCCESS;
    } else {
        printf("Failed to open database\n");
        return EXIT_FAILURE;
    }
}




