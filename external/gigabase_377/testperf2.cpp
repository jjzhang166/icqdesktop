//-< TESTPERF.CPP >--------------------------------------------------*--------*
// FastDB                    Version 1.0         (c) 1999  GARRET    *     ?  *
// (Main Memory Database Management System)                          *   /\|  *
//                                                                   *  /  \  *
//                          Created:     10-Dec-98    K.A. Knizhnik  * / [] \ *
//                          Last update: 20-Jan-99    K.A. Knizhnik  * GARRET *
//-------------------------------------------------------------------*--------*
// Performance test for index and sequential searches
//-------------------------------------------------------------------*--------*

#include "gigabase.h"
#include <stdio.h>

USE_GIGABASE_NAMESPACE

const int nRecords = 100000;
const int maxKeys = 512;
const int nSequentialSearches = 10;

class Record {
  public:
    int key1;
    char const* key2;

    TYPE_DESCRIPTOR((KEY(key1, INDEXED), KEY(key2, INDEXED)));
    //TYPE_DESCRIPTOR((KEY(key1, INDEXED), FIELD(key2)));
};

REGISTER(Record);


int __cdecl main(int argc, char* argv[])
{
    int i, n;
    int nThreads = 1;
    char buf[32];

    if (argc > 1) {
        nThreads = atoi(argv[1]);
    }
    dbDatabase db;
    if (db.open("testperf2.dbs")) {
        db.setConcurrency(nThreads);
        nat8 key = 1999;
        time_t start = time(NULL);
        int* counters = new int[maxKeys*maxKeys];
        memset(counters, 0, sizeof(int)*maxKeys*maxKeys);
        for (i = 0; i < nRecords; i++) {
            Record rec;
            key = (3141592621u*key + 2718281829u) % 1000000007u;
            rec.key1 = (int)(key % maxKeys);
            key = (3141592621u*key + 2718281829u) % 1000000007u;
            counters[rec.key1*maxKeys + (int)(key % maxKeys)] += 1;
            sprintf(buf, "%d", (int)(key % maxKeys));
            rec.key2 = buf;
            insert(rec);
        }
        printf("Elapsed time for inserting %d record: %d seconds\n",
               nRecords, int(time(NULL) - start));
        start = time(NULL);
        db.commit();
        printf("Commit time: %d\n", int(time(NULL) - start));

        dbQuery q;
        dbCursor<Record> cursor;

        int   id;
        q = "key1=",id,"and key2=",buf;
        key = 1999;
        for (i = 0; i < nRecords; i++) {
            key = (3141592621u*key + 2718281829u) % 1000000007u;
            id = (int)(key % maxKeys);
            key = (3141592621u*key + 2718281829u) % 1000000007u;
            sprintf(buf, "%d", (int)(key % maxKeys));
            n = cursor.select(q);
            if (n != counters[id*maxKeys + (int)(key % maxKeys)]) printf("n=%d, should be %d\n", n, counters[id*maxKeys + (int)(key % maxKeys)]);
            assert(n == counters[id*maxKeys + (int)(key % maxKeys)]);
        }
        printf("Elapsed time for %d index searches: %d seconds\n",
               nRecords, int(time(NULL) - start));
        cursor.select(dbCursorForUpdate);
        cursor.removeAllSelected();
        db.close();
        return EXIT_SUCCESS;
    } else {
        printf("Failed to open database\n");
        return EXIT_FAILURE;
    }
}




