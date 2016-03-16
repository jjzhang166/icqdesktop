//-< TESTLEAK.CPP >--------------------------------------------------*--------*
// GigaBASE                  Version 1.0         (c) 1999  GARRET    *     ?  *
// (Post Relational Database Management System)                      *   /\|  *
//                                                                   *  /  \  *
//                          Created:     26-Jul-2001  K.A. Knizhnik  * / [] \ *
//                          Last update: 26-Jul-2001  K.A. Knizhnik  * GARRET *
//-------------------------------------------------------------------*--------*
// Memory allocator test
//-------------------------------------------------------------------*--------*

#include "gigabase.h"
#include <stdio.h>

USE_GIGABASE_NAMESPACE

#define N_ITERATIONS  2000
#define N_KEYS        1000
#define VALUE_LENGTH  10000


class Test {
  public:
    char const* key;
    char const* value;

    TYPE_DESCRIPTOR((KEY(key, INDEXED), FIELD(value)));
};
   
REGISTER(Test);

int __cdecl main(int argc, char* argv[])
{
    dbDatabase db;
    char buf[64];
    int nIterations = N_ITERATIONS;
    if (argc > 1) { 
        nIterations = atoi(argv[1]);
    }
    if (db.open("testleak.dbs")) {
        Test t;
        sprintf(buf, "KEY-%08d", 0);    
        char* p = new char[VALUE_LENGTH];       
        memset(p, ' ', VALUE_LENGTH);
        p[VALUE_LENGTH-1] = '\0';
        t.value = p;
        dbCursor<Test> cursor(dbCursorForUpdate);
        cursor.select();
        cursor.removeAll();
        dbQuery q;
        q = "key = ", buf;
        for (int i = 0; i < nIterations; i++) {             
            if (i > N_KEYS) { 
                sprintf(buf, "KEY-%08d", i - N_KEYS);
                int rc = cursor.select(q);
                assert(rc == 1);
                cursor.remove();
            }
            sprintf(buf, "KEY-%08d", i);
            t.key = buf;
            insert(t);
            db.commit();
            if (i % 100 == 0) { 
                printf("%d records\r", i);
            }
        }
        db.close();
        printf("\nDone\n");
    } else { 
        fprintf(stderr, "Failed to open database\n");
    }
    return 0;
}











