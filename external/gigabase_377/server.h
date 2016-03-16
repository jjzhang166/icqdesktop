//-< SERVER.CPP >----------------------------------------------------*--------*
// GigaBASE                  Version 1.0         (c) 1999  GARRET    *     ?  *
// (Post Relational Database Management System)                      *   /\|  *
//                                                                   *  /  \  *
//                          Created:     13-Jan-2000  K.A. Knizhnik  * / [] \ *
//                          Last update: 13-Jan-2000  K.A. Knizhnik  * GARRET *
//-------------------------------------------------------------------*--------*
// CLI multithreaded server class
//-------------------------------------------------------------------*--------*

#ifndef __SERVER_H__
#define __SERVER_H__

#include "sockio.h"

BEGIN_GIGABASE_NAMESPACE

class dbColumnBinding {
  public:
    dbColumnBinding*   next;
    dbFieldDescriptor* fd;
    int                cliType;
    int                len;
    char*              ptr;

    int  unpackArray(char* dst, size_t& offs);
    void unpackScalar(char* dst, bool insert);

    dbColumnBinding(dbFieldDescriptor* field, int type) {
        fd = field;
        cliType = type;
        next = NULL;
    }
};

struct dbParameterBinding {
    union {
        int1       i1;
        int2       i2;
        int4       i4;
        db_int8    i8;
        real4      r4;
        real8      r8;
        oid_t      oid;
        bool       b;
        char_t*    str;
        rectangle  rect;
    } u;
    int type;
};

const int dbQueryMaxIdLength = 256;

class dbQueryScanner {
  public:
    char*    p;
    db_int8  ival;
    real8    fval;
    char_t   buf[dbQueryMaxIdLength];
    char_t*  ident;

    int  get();

    void reset(char* sql) {
        p = sql;
    }
};

class dbStatement {
  public:
    int                 id;
    bool                firstFetch;
    dbStatement*        next;
    dbAnyCursor*        cursor;
    dbQuery             query;
    dbColumnBinding*    columns;
    char*               buf;
    int                 buf_size;
    int                 n_params;
    int                 n_columns;
    dbParameterBinding* params;
    dbTableDescriptor*  table;

    void reset();

    dbStatement(int stmt_id) {
        id = stmt_id;
        columns = NULL;
        params = NULL;
        buf = NULL;
        buf_size = 0;
        table = NULL;
        cursor = NULL; 
    }
    ~dbStatement() {
        reset();
        delete[] buf;
    }
};

struct UserInfo {
    char_t const* user;
    char_t const* password;

    TYPE_DESCRIPTOR((KEY(user, INDEXED), FIELD(password)));
};


class dbClientSession {
  public:
    dbClientSession*   next;
    dbStatement*       stmts;
    dbQueryScanner     scanner;
    socket_t*          sock;
    bool               in_transaction;
    dbTableDescriptor* dropped_tables;
    dbTableDescriptor* existed_tables;
};
class GIGABASE_DLL_ENTRY dbServer {
  protected:
    static dbServer* chain;
    dbServer*        next;
    char_t*          URL;
    char*            address;
    dbClientSession* freeList;
    dbClientSession* waitList;
    dbClientSession* activeList;
    int              optimalNumberOfThreads;
    int              connectionQueueLen;
    int              nActiveThreads;
    int              nIdleThreads;
    int              waitListLength;
    bool             cancelWait;
    bool             cancelAccept;
    bool             cancelSession;
    dbMutex          mutex;
    dbSemaphore      go;
    dbSemaphore      done;
    socket_t*        globalAcceptSock;
    socket_t*        localAcceptSock;
    dbThread         localAcceptThread;
    dbThread         globalAcceptThread;
    dbDatabase*      db;

    static void thread_proc serverThread(void* arg);
    static void thread_proc acceptLocalThread(void* arg);
    static void thread_proc acceptGlobalThread(void* arg);

    void serveClient();
    void acceptConnection(socket_t* sock);

    bool freeze(dbClientSession* session, int stmt_id);
    bool unfreeze(dbClientSession* session, int stmt_id);
    bool get_first(dbClientSession* session, int stmt_id);
    bool get_last(dbClientSession* session, int stmt_id);
    bool get_next(dbClientSession* session, int stmt_id);
    bool get_prev(dbClientSession* session, int stmt_id);
    bool seek(dbClientSession* session, int stmt_id, char* buf);
    bool skip(dbClientSession* session, int stmt_id, char* buf);
    bool fetch(dbClientSession* session, dbStatement* stmt, oid_t result);
    bool fetch(dbClientSession* session, dbStatement* stmt) { 
        return fetch(session, stmt, stmt->cursor->currId);
    }
    bool insert_cpp(dbClientSession* session, char* data);
    bool update_cpp(dbClientSession* session, char* data);
    bool remove_cpp(dbClientSession* session, char* data);
    bool select_cpp(dbClientSession* session, char* data);
    bool reload_schema(dbClientSession* session, char* data);
    bool remove_cond(dbClientSession* session, char* data);
    int  execute_query(char* data, dbQuery& q, dbAnyCursor& cursor);

    bool remove(dbClientSession* session, int stmt_id);
    bool remove_current(dbClientSession* session, int stmt_id);
    bool update(dbClientSession* session, int stmt_id, char* new_data);
    bool insert(dbClientSession* session, int stmt_id, char* data, bool prepare);
    bool select(dbClientSession* session, int stmt_id, char* data, bool prepare);
    bool show_tables(dbClientSession* session); 
    bool describe_table(dbClientSession* session, char* data);
    bool update_table(dbClientSession* session, char* data, bool create);
    bool drop_table(dbClientSession* session, char* data);
    bool alter_index(dbClientSession* session, char* data);
    bool authenticate(char* buf);

    char* checkColumns(dbStatement* stmt, int n_columns,
                       dbTableDescriptor* desc, char* data,
                       int4& reponse, bool select);

    dbStatement* findStatement(dbClientSession* stmt, int stmt_id);

  public:

    static dbServer* find(char_t const* serverURL);
    static void      cleanup();

    void stop();
    void start();

    dbServer(dbDatabase* db,
             char_t const* serverURL,
             int optimalNumberOfThreads = 8,
             int connectionQueueLen = 64);
    ~dbServer();
};

END_GIGABASE_NAMESPACE

#endif
