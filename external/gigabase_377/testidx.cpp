//-< PAGEPOOL.CPP >--------------------------------------------------*--------*
// GigaBASE                  Version 1.0         (c) 1999  GARRET    *     ?  *
// (Post Relational Database Management System)                      *   /\|  *
//                                                                   *  /  \  *
//                          Created:     10-Feb-98    K.A. Knizhnik  * / [] \ *
//                          Last update: 10-Feb-99    K.A. Knizhnik  * GARRET *
//-------------------------------------------------------------------*--------*
// Test for index insert/search/delete operations
//-------------------------------------------------------------------*--------*

#include "gigabase.h"
#include <stdio.h>

USE_GIGABASE_NAMESPACE

const int nInsertedRecords = 10000000;
const int nRecords = 100000;
const int maxDuplicates = 256;
const int delta = 100000;

class Record {
  public:
    int4  key;
    char* strKey;
    TYPE_DESCRIPTOR((KEY(key, INDEXED|OPTIMIZE_DUPLICATES), KEY(strKey, INDEXED|OPTIMIZE_DUPLICATES)));
};

REGISTER(Record);


int __cdecl main(int argc, char* argv[])
{
    bool incremental = argc > 1 && strncmp(argv[1], "inc", 3) == 0;

    dbDatabase db(dbDatabase::dbAllAccess, 4096); // 32Mb page pool
    if (db.open("testidx.dbs")) {
        nat8 insKey = 1999;
        nat8 delKey = 1999;
        int4 key, minKey, maxKey;
        char strKey[32], minStrKey[32], maxStrKey[32];
        dbQuery q, q2, q3, q4, q5;
        Record  rec;
        dbCursor<Record> cursor(dbCursorForUpdate);
        dbCursorType cursorType = incremental ? dbCursorIncremental : dbCursorViewOnly;
        dbCursor<Record> cursor2(cursorType), cursor3(cursorType);
        q = "key=",key;
        q2 = "key between",minKey,"and",maxKey;
        q3 = "key between",minKey,"and",maxKey," order by key desc";
        q4 = "strKey between",minStrKey,"and",maxStrKey;
        q5 = "strKey between",minStrKey,"and",maxStrKey," order by strKey desc";
        time_t start = time(NULL);
        for (int i = 0, j = 0, n = 0; i < nInsertedRecords;) {
            if (n >= nRecords) {                
                delKey = (3141592621u*delKey + 2718281829u) % 1000000007u;
                minKey = (int4)(delKey-delta);
                maxKey = (int4)(delKey+delta);
                int n1 = cursor2.select(q2);
                int n2 = cursor3.select(q3);                
                assert(n1 == n2);
                if (incremental) { 
                    assert(cursor2.isIncremental());
                    assert(cursor3.isIncremental());
                }
                cursor2.last();
                bool hasPrev, hasNext;
                do { 
                    assert(cursor2->key == cursor3->key);
                    hasPrev = cursor2.prev();
                    hasNext = cursor3.next();
                    assert(hasPrev == hasNext);
                } while (hasNext);

                sprintf(minStrKey, INT8_FORMAT, delKey-delta);
                sprintf(maxStrKey, INT8_FORMAT, delKey+delta);                
                n1 = cursor2.select(q4);
                n2 = cursor3.select(q5);
                assert(n1 == n2);
                if (incremental) { 
                    assert(cursor2.isIncremental());
                    assert(cursor3.isIncremental());
                }
                if (n1 > 0) { 
                    cursor2.last();
                    do { 
                        assert(strcmp(cursor2->strKey, cursor3->strKey) == 0);
                        hasPrev = cursor2.prev();
                        hasNext = cursor3.next();
                        assert(hasPrev == hasNext);
                    } while (hasNext);
                }
                key = (int4)delKey;                
                unsigned r = cursor.select(q);
                assert(r == ((unsigned)delKey % maxDuplicates) + 1);
                n -= r;
                cursor.removeAllSelected();
            }
            insKey = (3141592621u*nat8(insKey) + 2718281829u) % 1000000007;
            unsigned r = ((unsigned)insKey % maxDuplicates) + 1;
            rec.key = (int4)insKey;
            sprintf(strKey, INT8_FORMAT, insKey);
            rec.strKey = strKey;
            n += r;
            i += r;
            do {
                insert(rec);
            } while (--r != 0);
            if (i > j) {
                printf("Insert %d objects...\r", i);
                fflush(stdout);
                j = i + 1000;
            }
        }
        printf("Elapsed time for %d record: %d seconds\n",
               nInsertedRecords, int(time(NULL) - start));
        db.close();
        return EXIT_SUCCESS;
    } else {
        printf("Failed to open database\n");
        return EXIT_FAILURE;
    }
}




