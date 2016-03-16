//-< SYMTAB.CPP >----------------------------------------------------*--------*
// GigaBASE                  Version 1.0         (c) 1999  GARRET    *     ?  *
// (Post Relational Database Management System)                      *   /\|  *
//                                                                   *  /  \  *
//                          Created:     20-Nov-98    K.A. Knizhnik  * / [] \ *
//                          Last update: 20-Nov-98    K.A. Knizhnik  * GARRET *
//-------------------------------------------------------------------*--------*
// Symbol table implementation
//-------------------------------------------------------------------*--------*

#define INSIDE_GIGABASE

#include "stdtp.h"
#include "sync.h"
#include "symtab.h"

BEGIN_GIGABASE_NAMESPACE

const size_t hashTableSize = 1009;
dbSymbolTable::HashTableItem* dbSymbolTable::hashTable[hashTableSize];

dbSymbolTable dbSymbolTable::instance;

dbSymbolTable::~dbSymbolTable() { 
    cleanup();
}


void dbSymbolTable::cleanup() { 
    for (int i = hashTableSize; --i >= 0;) { 
        HashTableItem *ip, *next;
        for (ip = hashTable[i]; ip != NULL; ip = next) {
            next = ip->next;
            delete ip;
        }
        hashTable[i] = NULL;
    }
}    

int dbSymbolTable::add(char_t* &str, int tag, bool allocate) {
    static dbMutex mutex;
    dbCriticalSection cs(mutex);
    unsigned hash = 0;
    char_t* p = str;
    while (*p != 0) {
        hash = hash*31 + *p++;
    }
    int index = hash % hashTableSize;
    HashTableItem *ip;
    for (ip = hashTable[index]; ip != NULL; ip = ip->next) {
        if (ip->hash == hash && STRCMP(ip->str, str) == 0) {
            str = ip->str;
            if (tag > ip->tag) { 
                ip->tag = tag;
            }
            return ip->tag;
        }
    }
    ip = new HashTableItem;
    ip->allocated = false;
    if (allocate) {
        char_t* dupstr = new char_t[STRLEN(str) + 1];
        STRCPY(dupstr, str);
        ip->allocated = true;
        str = dupstr;
    }
    ip->str = str;
    ip->hash = hash;
    ip->tag = tag;
    ip->next = hashTable[index];
    hashTable[index] = ip;
    return tag;
}


END_GIGABASE_NAMESPACE

