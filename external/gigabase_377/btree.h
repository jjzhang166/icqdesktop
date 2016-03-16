//-< BTREE.H >-------------------------------------------------------*--------*
// GigaBASE                  Version 1.0         (c) 1999  GARRET    *     ?  *
// (Post Relational Database Management System)                      *   /\|  *
//                                                                   *  /  \  *
//                          Created:      1-Jan-99    K.A. Knizhnik  * / [] \ *
//                          Last update: 25-Oct-99    K.A. Knizhnik  * GARRET *
//-------------------------------------------------------------------*--------*
// B-Tree interface
//-------------------------------------------------------------------*--------*

#ifndef __BTREE_H__
#define __BTREE_H__


BEGIN_GIGABASE_NAMESPACE

class dbAnyCursor;

#define MAX_BTREE_HEIGHT 8

class dbBtreePage {
  public:
    nat4 nItems;
    nat4 size;

    struct str {
        oid_t oid;
        nat2  size;
        nat2  offs;
    };

    enum { dbMaxKeyLen = (dbPageSize - sizeof(str)*2) / sizeof(char_t) / 2 };

    struct item {
        oid_t oid;
        int   keyLen;
        union {
            int1    keyInt1;
            int2    keyInt2;
            int4    keyInt4;
            db_int8 keyInt8;
            oid_t   keyOid;
            real4   keyReal4;
            real8   keyReal8;
            char_t  keyChar[dbMaxKeyLen];
        };
    };
    enum {
        maxItems = (dbPageSize - 8) / sizeof(oid_t)
    };


    union {
        oid_t  record[maxItems];
        int1   keyInt1[(dbPageSize-8) / sizeof(int1)];
        int2   keyInt2[(dbPageSize-8) / sizeof(int2)];
        int4   keyInt4[(dbPageSize-8) / sizeof(int4)];
        db_int8   keyInt8[(dbPageSize-8) / sizeof(db_int8)];
        oid_t  keyOid[(dbPageSize-8)  / sizeof(oid_t)];
        real4  keyReal4[(dbPageSize-8) / sizeof(real4)];
        real8  keyReal8[(dbPageSize-8) / sizeof(real8)];
        char   keyChar[dbPageSize-8];
        str    keyStr[1];
    };

    static oid_t allocate(dbDatabase* db, oid_t root, int type, int sizeofType, item& ins);

    static int   insert(dbDatabase* db, oid_t pageId,
                        int type, int sizeofType, dbUDTComparator comparator, item& ins, int height, bool unique);
    static int   remove(dbDatabase* db, oid_t pageId,
                        int type, int sizeofType, dbUDTComparator comparator, item& rem, int height);

    static void  purge(dbDatabase* db, oid_t pageId, int type, int height);

    int          insertStrKey(dbDatabase* db, int r, item& ins, int height);
    int          replaceStrKey(dbDatabase* db, int r, item& ins, int height);
    int          removeStrKey(int r);
    void         compactify(dbDatabase* db, int m);

    int          handlePageUnderflow(dbDatabase* db, int r, int type, int sizeofType, item& rem,
                                     int height);

    bool         find(dbDatabase* db, dbSearchContext& sc, int type, int sizeofType,
                      dbUDTComparator comparator, int height);

    bool         traverseForward(dbDatabase* db, dbAnyCursor* cursor,
                                 dbExprNode* condition, int type, int height);
    bool         traverseBackward(dbDatabase* db, dbAnyCursor* cursor,
                                  dbExprNode* condition, int type, int height);
};


class GIGABASE_DLL_ENTRY dbBtree : public dbRecord {
    friend class dbBtreeIterator;
  protected:
    oid_t root;
    int4  height;
    int4  type;
    int4  sizeofType;

  public:
    enum { 
        FLAGS_CASE_INSENSITIVE = 1, 
        FLAGS_THICK = 2,
        FLAGS_UNIQUE = 4
    };
    int1  flags; 

    bool isCaseInsensitive() { 
        return (flags & FLAGS_CASE_INSENSITIVE) != 0;
    }

    bool isThick() { 
        return (flags & FLAGS_THICK) != 0;
    }

    bool isUnique() { 
        return (flags & FLAGS_UNIQUE) != 0;
    }

    enum OperationEffect {
        done,
        overflow,
        underflow,
        not_found,                              
        not_unique
    };

    static oid_t allocate(dbDatabase* db, int type, int sizeofType, int flags = 0);
    static void  find(dbDatabase* db, oid_t treeId, dbSearchContext& sc, dbUDTComparator comparator);
    static bool  insert(dbDatabase* db, oid_t treeId, oid_t recordId, byte* record, int offs, dbUDTComparator comparator);
    static bool  insert(dbDatabase* db, oid_t treeId, oid_t recordId, int offs, dbUDTComparator comparator);
    static bool  insert(dbDatabase* db, oid_t treeId, dbBtreePage::item& ins, dbUDTComparator comparator);
    static void  remove(dbDatabase* db, oid_t treeId, oid_t recordId, byte* record, int offs, dbUDTComparator comparator);
    static void  remove(dbDatabase* db, oid_t treeId, oid_t recordId, int offs, dbUDTComparator comparator);
    static void  drop(dbDatabase* db, oid_t treeId);
    static void  purge(dbDatabase* db, oid_t treeId);

    static void  traverseForward(dbDatabase* db, oid_t treeId,
                                 dbAnyCursor* cursor, dbExprNode* condition);
    static void  traverseBackward(dbDatabase* db, oid_t treeId,
                                  dbAnyCursor* cursor, dbExprNode* condition);
    static void  traverseForward(dbDatabase* db, oid_t treeId,
                                 dbAnyCursor* cursor)
    {
        traverseForward(db, treeId, cursor, NULL);
    }
    static void  traverseBackward(dbDatabase* db, oid_t treeId,
                                  dbAnyCursor* cursor)
    {
        traverseBackward(db, treeId, cursor, NULL);
    }
};

class GIGABASE_DLL_ENTRY dbBtreeIterator : public dbAbstractIterator { 
  public:
    void init(dbDatabase* db, oid_t treeId, dbSearchContext& sc, dbUDTComparator comparator);
    
    oid_t next();
    oid_t prev();
    oid_t first();
    oid_t last();
    
  private:
    oid_t reset(bool ascent); 
    oid_t gotoNextItem(byte* pg, int pos, bool forward);
    
    oid_t getStringBtreePageOid(byte* pg, int i);
    oid_t getScalarBtreePageOid(byte* pg, int i);
    oid_t getStringThickBtreePageOid(byte* pg, int i);
    oid_t getScalarThickBtreePageOid(byte* pg, int i);
    void* getStringBtreePageKey(byte* pg, int i);
    void* getScalarBtreePageKey(byte* pg, int i);
    void* getStringThickBtreePageKey(byte* pg, int i);
    void* getScalarThickBtreePageKey(byte* pg, int i);
    
    oid_t (dbBtreeIterator::*getOid)(byte* pg, int i);
    void* (dbBtreeIterator::*getKey)(byte* pg, int i);
    
    int nItems(byte* pg) { 
        return ((dbBtreePage*)pg)->nItems;
    }

    dbUDTComparator comparator;
    dbDatabase*     db;
    dbSearchContext sc;
    int             sizeofType;
    int             type;
    int             height;
    oid_t           treeId;
    oid_t           pageStack[MAX_BTREE_HEIGHT];
    int             posStack[MAX_BTREE_HEIGHT];
};   

class dbThickBtreePage {
  public:
    nat4 nItems;
    nat4 size;

    struct reference {
        oid_t oid;
        oid_t recId;
    };

    struct str : reference {
        nat2  size;
        nat2  offs;
    };
    
    enum { dbMaxKeyLen = (dbPageSize - sizeof(str)*2) / sizeof(char_t) / 2 };

    struct item : reference {
        int   keyLen;
        union {
            int1    keyInt1;
            int2    keyInt2;
            int4    keyInt4;
            db_int8 keyInt8;
            oid_t   keyOid;
            real4   keyReal4;
            real8   keyReal8;
            char_t  keyChar[dbMaxKeyLen];
        };
    };
    enum {
        maxItems = (dbPageSize - 8) / sizeof(reference)
    };


    union {
        reference ref[maxItems];
        int1   keyInt1[(dbPageSize-8) / sizeof(int1)];
        int2   keyInt2[(dbPageSize-8) / sizeof(int2)];
        int4   keyInt4[(dbPageSize-8) / sizeof(int4)];
        db_int8   keyInt8[(dbPageSize-8) / sizeof(db_int8)];
        oid_t  keyOid[(dbPageSize-8)  / sizeof(oid_t)];
        real4  keyReal4[(dbPageSize-8) / sizeof(real4)];
        real8  keyReal8[(dbPageSize-8) / sizeof(real8)];
        char   keyChar[dbPageSize-8];
        str    keyStr[1];
    };

    static oid_t allocate(dbDatabase* db, oid_t root, int type, int sizeofType, item& ins);

    static int   insert(dbDatabase* db, oid_t pageId,
                        int type, int sizeofType, dbUDTComparator comparator, item& ins, int height);
    static int   remove(dbDatabase* db, oid_t pageId,
                        int type, int sizeofType, dbUDTComparator comparator, item& rem, int height);

    static void  purge(dbDatabase* db, oid_t pageId, int type, int height);

    int          insertStrKey(dbDatabase* db, int r, item& ins, int height);
    int          replaceStrKey(dbDatabase* db, int r, item& ins, int height);
    int          removeStrKey(int r);
    void         compactify(dbDatabase* db, int m);

    int          handlePageUnderflow(dbDatabase* db, int r, int type, int sizeofType, item& rem,
                                     int height);

    bool         find(dbDatabase* db, dbSearchContext& sc, int type, int sizeofType,
                      dbUDTComparator comparator, int height);

    bool         traverseForward(dbDatabase* db, dbAnyCursor* cursor,
                                 dbExprNode* condition, int type, int height);
    bool         traverseBackward(dbDatabase* db, dbAnyCursor* cursor,
                                  dbExprNode* condition, int type, int height);
};

END_GIGABASE_NAMESPACE

#endif
