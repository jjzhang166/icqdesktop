//-< CURSOR.H >------------------------------------------------------*--------*
// GigaBASE                  Version 1.0         (c) 1999  GARRET    *     ?  *
// (Post Relational Database Management System)                      *   /\|  *
//                                                                   *  /  \  *
//                          Created:     20-Nov-98    K.A. Knizhnik  * / [] \ *
//                          Last update: 10-Dec-98    K.A. Knizhnik  * GARRET *
//-------------------------------------------------------------------*--------*
// Table cursor
//-------------------------------------------------------------------*--------*

#ifndef __CURSOR_H__
#define __CURSOR_H__

#include "btree.h"
#include "rtree.h"

BEGIN_GIGABASE_NAMESPACE

#include "selection.h"

enum dbCursorType {
    dbCursorViewOnly,
    dbCursorForUpdate,
    /**
     * Execute query incrementally
     */
    dbCursorIncremental,
    /** Detached cursor commits transaction immediately after execution of the query,
     *  and then temporarily start new transaction for fetching each element.
     *  Such cursor should be used in conjunction with DO_NOT_REUSE_OID_WITHIN_SESSION to detect 
     *  removed objects.
     *  Please notice that this cursor is static - it only executed from the selection removed objects,
     *  but doesn't try to add the selection new records which match search criteria or remove 
     *  record which do not match this criteria any more.
     */
    dbCursorDetached
};

/**
 *  Iterator through table records
 */
class dbTableIterator : public dbAbstractIterator {
    dbAnyCursor* cursor;
    dbExprNode*  filter;
    oid_t        curr;

  public:
    void init(dbAnyCursor* cursor, dbExprNode*  filter) {
        this->cursor = cursor;
        this->filter = filter;
        curr = 0;
    }

    virtual oid_t next();
    virtual oid_t prev();
    virtual oid_t first();
    virtual oid_t last();
};



/**
 * Base class for all cursors
 */
class GIGABASE_DLL_ENTRY dbAnyCursor : public dbL2List {
    friend class dbDatabase;
    friend class dbHashTable;
    friend class dbRtreePage;
    friend class dbBtreePage;
    friend class dbRtreeIterator;
    friend class dbBtreeIterator;
    friend class dbTableIterator;
    friend class dbThickBtreePage;
    friend class dbSubSql;
    friend class dbStatement;
    friend class dbServer;
    friend class dbAnyContainer;
    friend class dbCLI;
    friend class JniResultSet;
  public:
    /**
     * Get number of selected records
     * @return number of selected records
     */
    int getNumberOfRecords() const { return (int)selection.nRows; }

    /**
     * Remove current record
     */
    void remove();

    /**
     * Checks whether selection is empty
     * @return true if there is no current record
     */
    bool isEmpty() const { 
        return currId == 0; 
    }


    /**
     * Get current reocrd OID
     * @return current record OID or 0 if thereis no current record
     */
    oid_t getOid() {
        return currId;
    }

    /**
     * Check whether this cursor can be used for update
     * @return true if it is update cursor
     */
    bool isUpdateCursor() const { 
        return type == dbCursorForUpdate;
    }

    /**
     * Checks whether limit for number of selected reacord is reached
     * @return true if limit is reached
     */
    bool isLimitReached() const { 
        return selection.nRows >= limit || selection.nRows >= stmtLimitLen; 
    }

    /**
     * Execute query.
     * @param query selection criteria
     * @param aType cursor type: <code>dbCursorForUpdate, dbCursorViewOnly</code>
     * @param paramStruct pointer to structure with parameters. If you want to create reentrant precompiled query, i.e.
     * query which can be used concurrently by different threadsm you should avoid to use static variables in 
     * such query, and instead of it place paramters into some structure, specify in query relative offsets to the parameters,
     * fill local structure and pass pointer to it to select method.
     * @return number of selected records
     */
    int select(dbQuery& query, dbCursorType aType, void* paramStruct = NULL);

    /**
     * Extract OIDs of selected recrods in array
     * @param arr if <code>arr</code> is not null, then this array is used as destination (it should
     *   be at least selection.nRows long)<BR>
     *  If <code>arr</code> is null, then new array is created by  new oid_t[] and returned by this method
     * @return if <code>arr</code> is not null, then <code>arr</code>, otherwise array created by this method
     */
    oid_t* toArrayOfOid(oid_t* arr) const;

    /**
     * Execute query with default cursor type.
     * @param query selection criteria
     * @param paramStruct pointer to structure with parameters.
     * @return number of selected records
     */    
    int select(dbQuery& query, void* paramStruct = NULL) {
        return select(query, defaultType, paramStruct);
    }

    /**
     * Execute query.
     * @param condition selection criteria
     * @param aType cursor type: <code>dbCursorForUpdate, dbCursorViewOnly</code>
     * @param paramStruct pointer to structure with parameters.
     * @return number of selected records
     */
    int select(char_t const* condition, dbCursorType aType, void* paramStruct = NULL) {
        dbQuery query(condition);
        return select(query, aType, paramStruct);
    }

    /**
     * Execute query with default cursor type.
     * @param condition selection criteria
     * @param paramStruct pointer to structure with parameters.
     * @return number of selected records
     */    
    int select(char_t const* condition, void* paramStruct = NULL) {
        return select(condition, defaultType, paramStruct);
    }

    /**
     * Select all records from the table
     * @param aType cursor type: <code>dbCursorForUpdate, dbCursorViewOnly</code>
     * @return number of selected records
     */    
    int select(dbCursorType aType) {
        type = aType;
        reset();
        db->select(this);
        if (gotoFirst() && prefetch) {
            fetch();
        }
        return (int)selection.nRows;
    }

    /**
     * Select all records from the table with default cursor type
     * @return number of selected records
     */    
    int select() {
        return select(defaultType);
    }

    /**
     * Select all records from the table with specfied value of the key
     * @param key name of the key field
     * @param value searched value of the key
     * @return number of selected records
     */    
    int selectByKey(char_t const* key, void const* value);

    /**
     * Select all records from the table with specfied value of the key
     * @param field key field
     * @param value searched value of the key
     * @return number of selected records
     */    
    int selectByKey(dbFieldDescriptor* field, void const* value);

    /**
     * Select all records from the table with specfied range of the key values
     * @param key name of the key field
     * @param minValue inclusive low bound for key values, if <code>NULL</code> then there is no low bound
     * @param maxValue inclusive high bound for key values, if <code>NULL</code> then there is no high bound
     * @param ascent key order: <code>true</code> - ascending order, <code>false</code> - descending order
     * @return number of selected records
     */    
    int selectByKeyRange(char_t const* key, void const* minValue, void const* maxValue, bool ascent = true);

    /**
     * Select all records from the table with specfied range of the key values
     * @param field key field
     * @param minValue inclusive low bound for key values, if <code>NULL</code> then there is no low bound
     * @param maxValue inclusive high bound for key values, if <code>NULL</code> then there is no high bound
     * @param ascent key order: <code>true</code> - ascending order, <code>false</code> - descending order
     * @return number of selected records
     */    
    int selectByKeyRange(dbFieldDescriptor* field, void const* minValue, void const* maxValue, bool ascent = true);

    /**
     * Update current record. You should changed value of current record before and then call
     * update method to save changes to the database.
     * @return true if record was successfully updated, false if update cause unique constraint violation
     */
    bool update() {
        if (type != dbCursorForUpdate) {
            db->handleError(dbDatabase::CursorError, "Readonly cursor");
        }
        if (currId == 0) {
            db->handleError(dbDatabase::CursorError, "No current record");
        }
        return db->update(currId, table, record);
    }

    /**
     * Remove all records in the table
     */
    void removeAll() {
        assert(db != NULL);
        reset();
        db->deleteTable(table);
    }

    /**
     * Remove all selected records
     */
    void removeAllSelected();

    /**
     * Specify maximal number of records to be selected
     */
    void setSelectionLimit(size_t lim) { limit = lim; }

    /**
     * Remove selection limit
     */
    void unsetSelectionLimit() { limit = dbDefaultSelectionLimit; }

    /**
     * Set prefetch mode. By default, current record is fetch as soon as it is becomes current.
     * But sometimesyou need only OIDs of selected records. In this case setting prefetchMode to false can help.
     * @param mode if <code>false</code> then current record is not fetched. You should explicitly call <code>fetch</code>
     * method if you want to fetch it.
     */
    void setPrefetchMode(bool mode) { prefetch = mode; }

    /**
     * Check if cursor is incremental. Incremental cursor is used only when type is dbCursorIncremental.
     * But even if the type of the cursor is dbCursorIncremental, the cursor 
     * may not always be incremental - it depends on a query. 
     * Queries containing order by clause except ordering by index key, can not be executed incrementally.
     * In case of incremental cursor, select() returns 0 if no record is selected or 1 
     * if selection is not empty,
     * but precise number of selected records is not reported since it is not known.
     * @return whether cursor is incremental or not.
     */
    bool isIncremental() { 
        return iterator != NULL;
    }

    /**
     * Check if cursor has dbIncrementalType.
     * @return whether cursor has incremental hint or not
     */
    bool hasIncrementalHint() { 
        return type == dbCursorIncremental;
    }

    /**
     * Enable or disable duplicates checking (if programmer knows that disjuncts in query do not intersect, then
     * he can disable duplicate checking and avoid bitmap allocation
     */
    void enableCheckForDuplicates(bool enabled) {
        checkForDuplicatedIsEnabled = enabled;
    }
 
    /**
     * Reset cursor
     */
    void reset();

    /**
     * Check whether current record is the last one in the selection
     * @return true if next() method will return <code>NULL</code>
     */
    bool isLast() const; 

    /**
     * Check whether current record is the first one in the selection
     * @return true if prev() method will return <code>NULL</code>
     */
    bool isFirst() const; 

    /**
     * Freeze cursor. This method makes it possible to save current state of cursor, close transaction to allow
     * other threads to proceed, and then later restore state of the cursor using unfreeze method and continue 
     * traversal through selected records.
     */     
    void freeze();

    /**
     * Unfreeze cursor. This method starts new transaction and restore state of the cursor
     */
    void unfreeze();

    /**
     * Skip specified number of records
     * @param n if positive then skip <code>n</code> records forward, if negative then skip <code>-n</code> 
     * records backward
     * @return <code>true</code> if specified number of records was successfully skipped, <code>false</code> if
     * there is no next (<code>n &gt; 0</code>) or previous (<code>n &lt; 0</code>) record in the selction.
     */
    bool skip(int n);

    /**
     * Position cursor on the record with the specified OID
     * @param oid object identifier of record
     * @return poistion of the record in the selection or -1 if record with such OID is not in selection
     */
    int seek(oid_t oid);

    /**
     * Get descriptor of the table. 
     * @return descriptor of the table associated with the cursor
     */
    dbTableDescriptor* getTable() { return table; }


    /**
     * Check if record with specified OID is in selection
     * @return <code>true</code> if record with such OID was selected
     */
    bool isInSelection(oid_t oid);

    /**
     * Fetch current record.
     * You should use this method only if prefetch mode is disabled 
     */
    void fetch() {
        dbRecord* row = (type == dbCursorDetached) ? db->fetchRow(tie, currId) : db->getRow(tie, currId);
#ifdef GIGABASE_PROTECT
		assert(row); //added by makarov
		if (!row)
		{
			currId = 0;
			record = NULL;
			return;
		}
		assert(row->size <= GIGABASE_MAX_ROW_SIZE);
		if (row->size > GIGABASE_MAX_ROW_SIZE)
		{
			currId = 0;
			record = NULL;
			return;
		}
#endif
        table->columns->fetchRecordFields(record, (byte*)row);
    }

    /**
     * Check if there is more records in the selection
     */
    bool hasNext() const;

    /**
     * Check if there is current record in the selection
     */
    bool hasCurrent() const { 
        return currId != 0;
    }

  protected:
    dbDatabase*        db;
    dbTableDescriptor* table;
    dbCursorType       type;
    dbCursorType       defaultType;
    dbSelection        selection;
    bool               allRecords;
    oid_t              firstId;
    oid_t              lastId;
    oid_t              currId;
    byte*              record;
    size_t             limit;
    dbGetTie           tie;
    void*              paramBase;

    bool               eliminateDuplicates;
    bool               checkForDuplicatedIsEnabled;
    bool               prefetch;
    bool               removed; // current record was removed
    bool               lastRecordWasDeleted; //last record was deleted

    size_t             stmtLimitStart;
    size_t             stmtLimitLen;
    size_t             nSkipped;

    dbAbstractIterator*iterator;
    dbBtreeIterator    btreeIterator;
    dbRtreeIterator    rtreeIterator;
    dbTableIterator    tableIterator;

    void checkForDuplicates() { 
        if (!eliminateDuplicates && checkForDuplicatedIsEnabled && limit > 1) {
            eliminateDuplicates = true;
            selection.allocateBitmap(db);
        }
    }

    bool isMarked(oid_t oid) {
        return selection.bitmap != NULL && (selection.bitmap[(size_t)(oid >> 5)] & (1 << ((int)oid & 31))) != 0;
    }
    
    void deallocateBitmap() {
        selection.deallocateBitmap();
    }

    void mark(oid_t oid) {
        if (selection.bitmap != NULL) {
            selection.bitmap[(size_t)(oid >> 5)] |= 1 << ((int)oid & 31);
        }
    }

    void setStatementLimit(dbQuery const& q) { 
        stmtLimitStart = q.stmtLimitStartPtr != NULL ? (nat4)*q.stmtLimitStartPtr : q.stmtLimitStart;
        stmtLimitLen = q.stmtLimitLenPtr != NULL ? (nat4)*q.stmtLimitLenPtr : q.stmtLimitLen;
    }

    void truncateSelection() { 
        selection.truncate(stmtLimitStart, stmtLimitLen);
    }

    bool add(oid_t oid) {
        if (selection.nRows < limit && selection.nRows < stmtLimitLen) {
            if (nSkipped < stmtLimitStart) { 
                nSkipped += 1;
                return true;
            }
            if (eliminateDuplicates) {
                if (selection.bitmap[(size_t)(oid >> 5)] & (1 << ((int)oid & 31))) {
                    return true;
                }
                selection.bitmap[oid >> 5] |= 1 << (oid & 31);
            }
            selection.add(oid);
            return selection.nRows < limit;
        }
        return false;
    }
    
    byte* fetchNext();
    byte* fetchPrev();
    byte* fetchFirst();
    byte* fetchLast();

    bool gotoNext();
    bool gotoPrev();
    bool gotoFirst();
    bool gotoLast();

    bool moveNext();
    bool movePrev();

    void setCurrent(dbAnyReference const& ref);

    void setTable(dbTableDescriptor* aTable) { 
        table = aTable;
        db = aTable->db;
    }

    void setRecord(void* rec) { 
        record = (byte*)rec;
    }

    dbAnyCursor(dbTableDescriptor& aTable, dbCursorType aType, byte* rec);

  public:
    dbAnyCursor(dbCursorType aType = dbCursorViewOnly);
    ~dbAnyCursor();
};

/**
 * Cursor template parameterized by table class
 */
template<class T>
class dbCursor : public dbAnyCursor {
  private:
    // Itis not possible to copy cursors
    dbCursor<T> operator = (dbCursor<T> const& src) { 
        return *this;
    } 

  protected:
    T record;

  public:
    /**
     * Cursor constructor
     * @param type cursor type (dbCursorViewOnly by default)
     */
    dbCursor(dbCursorType type = dbCursorViewOnly)
        : dbAnyCursor(T::dbDescriptor, type, (byte*)&record) {}

    /**
     * Cursor constructor with explicit specification of database.
     * This cursor should be used for unassigned tables. 
     * @param aDb database in which table lokkup is performed
     * @param type cursor type (dbCursorViewOnly by default)
     */
    dbCursor(dbDatabase* aDb, dbCursorType type = dbCursorViewOnly)
        : dbAnyCursor(T::dbDescriptor, type, (byte*)&record) 
    {
        db = aDb;
        dbTableDescriptor* theTable = db->lookupTable(table);
        if (theTable != NULL) { 
            table = theTable;
        }
    }

    /**
     * Get pointer to the current record
     * @return pointer to the current record or <code>NULL</code> if there is no current record
     */
    T* get() {
        return currId == 0 ? (T*)NULL : &record;
    }

    /**
     * Get next record
     * @return pointer to the next record or <code>NULL</code> if there is no next record
     */     
    T* next() {
        return (T*)fetchNext();
    }

    /**
     * Get previous record
     * @return pointer to the previous record or <code>NULL</code> if there is no previous record
     */     
    T* prev() {
        return (T*)fetchPrev();
    }

    /**
     * Get pointer to the first record
     * @return pointer to the first record or <code>NULL</code> if no records were selected
     */
    T* first() {
        return (T*)fetchFirst();
    }

    /**
     * Get pointer to the last record
     * @return pointer to the last record or <code>NULL</code> if no records were selected
     */
    T* last() {
        return (T*)fetchLast();
    }
    
    /**
     * Position cursor on the record with the specified OID
     * @param ref reference to the object
     * @return position of the record in the selection or -1 if record with such OID is not in selection
     */
    int seek(dbReference<T> const& ref) { 
        return dbAnyCursor::seek(ref.getOid());
    }

    /**
     * Overloaded operator for accessing components of the current record
     * @return pointer to the current record
     */
    T* operator ->() {
        if (currId == 0) {
            db->handleError(dbDatabase::CursorError, "No current record");
        }
        return &record;
    }

    /**
     * Select record by reference
     * @param ref reference to the record
     * @return pointer to the referenced record
     */
    T* at(dbReference<T> const& ref) {
        setCurrent(ref);
        return &record;
    }

    /**
     * Get current object idenitifer
     * @return reference to the current record
     */
    dbReference<T> currentId() const {
        return dbReference<T>(currId);
    }
    
    /**
     * Convert selection to array of reference
     * @param arr [OUT] array of refeences in which references to selected recrods will be placed
     */
    void toArray(dbArray< dbReference<T> >& arr) const { 
        arr.resize(selection.nRows);
        toArrayOfOid((oid_t*)arr.base());
    }

    T* prevAvailable() { 
        if (!removed) { 
            return prev(); 
        } else {
            removed = false;
            return lastRecordWasDeleted ? get() : prev();
        }
    }

    /**
     * Check if record with specified OID is in selection
     * @return <code>true</code> if record with such OID was selected
     */
    bool isInSelection(dbReference<T>& ref) {
        return dbAnyCursor::isInSelection(ref.getOid());
    }
};

class dbParallelQueryContext {
  public:
    dbDatabase* const      db;
    dbCompiledQuery* const query;
    dbAnyCursor*           cursor;
    oid_t                  firstRow;
    dbTableDescriptor*     table;
    dbSelection            selection[dbMaxParallelSearchThreads];

    void search(int i);

    dbParallelQueryContext(dbDatabase* aDb, dbTableDescriptor* desc,
                           dbCompiledQuery* aQuery, dbAnyCursor* aCursor)
      : db(aDb), query(aQuery), cursor(aCursor), firstRow(desc->firstRow), table(desc) {}
};

END_GIGABASE_NAMESPACE

#endif
