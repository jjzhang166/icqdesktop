//-< SESSION.H >-----------------------------------------------------*--------*
// GigaBASE                  Version 1.0         (c) 1999  GARRET    *     ?  *
// (Post Relational Database Management System)                      *   /\|  *
//                                                                   *  /  \  *
//                          Created:     28-Jul-2009  K.A. Knizhnik  * / [] \ *
//                          Last update: 28-Jul-2009  K.A. Knizhnik  * GARRET *
//-------------------------------------------------------------------*--------*
// Remote C++ interface to database 
//-------------------------------------------------------------------*--------*

#ifndef __SESSION_H__
#define __SESSION_H__

#include "class.h"
#include "reference.h"
#include "array.h"
#include "query.h"
#include "sockio.h"
#include "exception.h"

BEGIN_GIGABASE_NAMESPACE

/**
 * Exception thrown in case of connection failure
 */
class ConnectionException : public dbException {
  public:
    ConnectionException(char const* op, socket_t* s);
};

/**
 * Exception thrown when next method is called for closed cursor
 */
class CursorException : public dbException {
  public:
    CursorException(char const* msg);
};


const size_t SOCKET_BUFFER_SIZE = 64*1024;    

/**
 * Class representing remote connection to GiggaBASE server.
 */
class GIGABASE_DLL_ENTRY dbSession 
{
  public:
    /**
     * Create new session
     * @param host server host address and poty, for example "192.168.1.100:1234"
     * @param max_connect_attempts  - number of attempts to establish connection
     * @param reconnect_timeout_sec - timeput in seconds between connection attempts
     * @param user_name - user name for login
     * @param password  - password for login
     */
    dbSession(char const* address, 
              int max_connect_attempts = 10,
              int reconnect_timeout_sec = 1, 
              char_t const* user_name = _T(""),
              char_t const* password = _T(""));

    /**
     * Cleanup session data
     */
    ~dbSession();

    /**
     * Execute query. You shoul duse next method to iterate through query results.
     * @param query constructed query
     * @param limit maximal number of selected records (0 means no limit)
     */
    template <class T>
    void select(dbQuery const& query, size_t limit = 0) {
        select(&T::dbDescriptor, query, limit);
    }
    
    /**
     * Reset cursor: release all resource hold be opened cursor
     */
    void reset();

    /**
     * Move to the next selewcted record. Thisd method should be used after select to iterate through query results.
     * @param record destination where current record will be extracred
     * @return reference to the current object or null reference when there are no more records (end of selcetion is reached)
     */
    template <class T>
    dbReference<T> next(T& record) {
        return dbReference<T>(next(&T::dbDescriptor, &record));
    }

    /**
     * Insert new record in the database
     * @param record inserted record data
     * @param batch if <code>true</code> then record will be inserted in the batch mode:
     * it will be included in indices at the trasnaction commit time
     * @return reference assigned to the inserted object
     */
    template <class T>
    dbReference<T> insert(T const& record, bool batch = false) {
        return dbReference<T>(insert(&T::dbDescriptor, &record, batch));
    }

    /**
     * Update referenced record 
     * @param record updated record data 
     * @param ref reference to the updated record
     */
    template <class T>
    bool update(T const& record, dbReference<T> ref)
    {
        return update(&T::dbDescriptor, &record, ref.getOid());
    }

    /**
     * Remove referenced record
     * @param ref reference to the record to be removed
     */
    template <class T>
    void remove(dbReference<T> ref)
    {
        remove(&T::dbDescriptor, ref.getOid());
    }

    /**
     * Remove objects matching specified criteria
     * @param condition criteria for selecting records for deletion
     * @param limit maximal number of removed records (0 - unlimited)
     * @return number of selected objects
     */
    template <class T>
    size_t remove(dbQuery const& condition, size_t limit = 0)
    {
        return remove(&T::dbDescriptor, condition, limit);
    }
    

    /**
     * Commit current transaction (new transaction is started implcitly)
     */
    void commit();

    /**
     * Exclusively lock the database 
     */
    void lock();

    /**
     * Release transaction locks
     */
    void unlock();

    /**
     * Rollback current transaction
     */
    void rollback();

  private:
    oid_t  insert(dbTableDescriptor* table, void const* record, bool batch);
    bool   update(dbTableDescriptor* table, void const* record, oid_t oid);
    void   remove(dbTableDescriptor* table, oid_t oid);
    oid_t  next(dbTableDescriptor* table, void* record);
    void   select(dbTableDescriptor* table, dbQuery const& query, size_t limit);
    size_t remove(dbTableDescriptor* table, dbQuery const& query, size_t limit);

    void   fillBuffer(size_t size);
    void   sendCommand(int cmd);
    void   sendQuery(dbTableDescriptor* table, dbQuery const& query, size_t limit, int cmd);
    void   reloadSchema();
    void   login(char_t const* user_name, char_t const* password);

    socket_t* socket;
    char      sockBuf[SOCKET_BUFFER_SIZE];
    size_t    bufUsed;
    size_t    bufPos;
    byte*     currObj;
    dbTableDescriptor** tables;
    bool      selected;
};

END_GIGABASE_NAMESPACE

#endif

