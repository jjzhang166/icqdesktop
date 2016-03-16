//-< TESTREPLIC.CPP >------------------------------------------------*--------*
// GigaBASE                  Version 1.0         (c) 1999  GARRET    *     ?  *
// (Post Relational Database Management System)                      *   /\|  *
//                                                                   *  /  \  *
//                          Created:     13-Mar-2002  K.A. Knizhnik  * / [] \ *
//                          Last update: 13-Mar-2002  K.A. Knizhnik  * GARRET *
//-------------------------------------------------------------------*--------*
// Test of replicated database
//-------------------------------------------------------------------*--------*

#include "gigabase.h"
#include "replicator.h"

USE_GIGABASE_NAMESPACE

class Account { 
  public:
    char const* name;
    int         amount;

    TYPE_DESCRIPTOR((KEY(name, INDEXED), FIELD(amount)));
};
  
REGISTER(Account);
 
class dbMyReplicationManager : public dbReplicationManager { 
  public:
    virtual bool connectionBroken(char* hostName) { 
        printf("Connection with host %s is broken\n", hostName);
        terminated = true;
        recoveryNeeded = true;
        return false;
    }

    virtual void transactionCommitted() {
        nCommittedTransactions += 1;
    }
        
    virtual void replicationEnd() { 
        terminated = true;
    }

    virtual bool preserveSlaveConsistency() { 
        return false;
    }

    dbMyReplicationManager() { 
        terminated = false;
        recoveryNeeded = false;
        nCommittedTransactions = 0;
    }

    bool terminated;
    bool recoveryNeeded;
    int  nCommittedTransactions;
};
    

#define INITIAL_AMOUNT         1000
#define DEFAULT_N_ITERATIONS   INITIAL_AMOUNT
#define DEFAULT_MASTER_ADDRESS "127.0.0.1:5100"

#define NAME1 "John Smith"
#define NAME2 "Robert Mayer"

int __cdecl main(int argc, char* argv[])
{
    if (argc < 2) { 
        fprintf(stderr, "Usage: testreplic (masterN|slave) [master-host-address] [number-of-iterations]\n");
        return 1;
    }
    char const* masterAddress = argc < 3 ? DEFAULT_MASTER_ADDRESS : argv[2];
    int nIterations = argc < 4 ? DEFAULT_N_ITERATIONS : atoi(argv[3]);
    int i = 0, am1 = 0, am2 = 0, rc;

    dbMyReplicationManager mng;
    dbReplicatedDatabase db(&mng);

    dbCursor<Account> cursor;
    dbQuery q;
    char* name;
    q = "name=",&name;

    if (strcmp(argv[1], "slave") == 0) { 
        if (!db.open(masterAddress, 0, "slave.dbs")) { 
            fprintf(stderr, "Failed to start slave\n");
            return 1;
        }
        for (i = 0; !mng.terminated; i++) { 
            name = NAME1;
            if (cursor.select(q)) {
                am1 = cursor->amount;
                name = NAME2;
                rc = cursor.select(q);
                assert(rc == 1);
                am2 = cursor->amount;
                assert(am1 + am2 == INITIAL_AMOUNT);
            }
            db.commit();
        } 
        if (!mng.recoveryNeeded) { 
            printf("Slave database closed\n");
            db.close();
        }
        printf("Number of received transactions: %d\n", mng.nCommittedTransactions);
    } else { 
        int nReplicas = 0;
        sscanf(argv[1], "master%d", &nReplicas);
        if (nReplicas <= 0) { 
            fprintf(stderr, "Usage: testreplic masterN, when N is poistive number\n");
            return 1;
        }
        if (!db.open(masterAddress, nReplicas, "master.dbs")) { 
            fprintf(stderr, "Failed to start master\n");
            return 1;
        }
        if (cursor.select() == 0) { 
            Account acc;
            acc.name = NAME1;
            acc.amount = INITIAL_AMOUNT;
            insert(acc);
            acc.name = NAME2;
            acc.amount = 0;
            insert(acc);
        }
        dbCursor<Account> cursor1(dbCursorForUpdate);
        dbCursor<Account> cursor2(dbCursorForUpdate);
        for (i = 0; i < nIterations && !mng.terminated; i++) { 
            name = NAME1;
            rc = cursor1.select(q);
            assert(rc == 1);
            name = NAME2;
            rc = cursor2.select(q);
            assert(rc == 1);
            am1 = cursor1->amount -= 1;
            am2 = cursor2->amount += 1;
            cursor1.update();
            cursor2.update();
            db.commit();
        }

        if ((i & 1) == 0) { 
            printf("Master database closed\n");
            db.close();
        }
    }
    printf("Number of iterations %d, amount1=%d, amount2=%d\n", i, am1, am2);
    return 0;
}











