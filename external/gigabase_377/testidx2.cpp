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

USE_GIGABASE_NAMESPACE

const int nIterations = 1000000;
const int nRecords = 1000;

#define MAX_KEY_LEN (4000/sizeof(char_t))

class Record {
  public:
    char_t const* key;

    TYPE_DESCRIPTOR((KEY(key, INDEXED)));
};

REGISTER(Record);


int __cdecl main(int argc, char* argv[])
{
    char_t buf[MAX_KEY_LEN+1];

    dbDatabase db(dbDatabase::dbAllAccess, 5*1024); // 40Mb page pool
    if (db.open(_T("testidx2.dbs"))) {
        dbQuery q;
        dbCursor<Record> cursor(dbCursorForUpdate);
        q = _T("key="),buf;
        nat8 insKey = 1999;
        nat8 remKey = insKey;
        time_t start = time(NULL);
        for (int i = 0; i < nIterations; i++) {
            Record rec;
            insKey = (3141592621u*insKey + 2718281829u) % 1000000007u;
            int n = SPRINTF(SPRINTF_BUFFER(buf), T_INT8_FORMAT _T("."), insKey);
            int len = (unsigned)insKey % MAX_KEY_LEN;
            while (n < len) { 
                buf[n++] = ' ';
            }
            buf[n] = '\0';
            rec.key = buf;
            insert(rec);
            if (i > nRecords) { 
                remKey = (3141592621u*remKey + 2718281829u) % 1000000007u;
                int n = SPRINTF(SPRINTF_BUFFER(buf), T_INT8_FORMAT _T("."), remKey);
                int len = (unsigned)remKey % MAX_KEY_LEN;
                while (n < len) { 
                    buf[n++] = ' ';
                }
                buf[n] = '\0';
                n = cursor.select(q);
                assert(n == 1);
                cursor.remove();
            }
            if (i % 100 == 0) { 
                printf("%d interations\r", i);
            }
        }
        printf("Elapsed for %d interations: %d seconds\n",
               nIterations, int(time(NULL) - start));
        db.close();
        return EXIT_SUCCESS;
    } else {
        printf("Failed to open database\n");
        return EXIT_FAILURE;
    }
}




