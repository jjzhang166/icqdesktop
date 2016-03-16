//-< XML.CPP >-------------------------------------------------------*--------*
// GigaBASE                  Version 1.0         (c) 1999  GARRET    *     ?  *
// (Post Relational Database Management System)                      *   /\|  *
//                                                                   *  /  \  *
//                          Created:     14-Feb-2008  K.A. Knizhnik  * / [] \ *
//                          Last update: 14-Feb-2008  K.A. Knizhnik  * GARRET *
//-------------------------------------------------------------------*--------*
// XML export/import and related stuff
//-------------------------------------------------------------------*--------*

#define INSIDE_GIGABASE

#include "gigabase.h"
#include "compiler.h"
#include "hashtab.h"
#include "btree.h"
#include "rtree.h"
#include "symtab.h"

BEGIN_GIGABASE_NAMESPACE

class dbTmpAllocator { 
    enum { 
        CHUNK_SIZE = 4096
    };
    struct Chunk { 
        Chunk* next;
        Chunk* prev; // is not used, added for alignment
    };
    Chunk* curr;
    size_t used;

  public:
    dbTmpAllocator() { 
        curr = NULL;
        used = CHUNK_SIZE;
    }

    ~dbTmpAllocator() { 
        reset();
    }

    void reset() { 
        Chunk *c, *next; 
        for (c = curr; c != NULL; c = next) { 
            next = c->next;
            dbFree(c);
        }
        curr = NULL;
        used = CHUNK_SIZE;
    }


    void* alloc(size_t size) { 
        size = DOALIGN(size, 8);
        if (size > CHUNK_SIZE/2) { 
            Chunk* newChunk = (Chunk*)dbMalloc(size + sizeof(Chunk));
            if (curr != NULL) { 
                newChunk->next = curr->next;
                curr->next = newChunk;
            } else { 
                curr = newChunk;
                newChunk->next = NULL;
                used = CHUNK_SIZE;
            }
            return newChunk+1;
        } else if (size <= CHUNK_SIZE - used) { 
            used += size;
            return (char*)curr + used - size;
        } else { 
            Chunk* newChunk = (Chunk*)dbMalloc(CHUNK_SIZE);
            used = sizeof(Chunk) + size;
            newChunk->next = curr;
            curr = newChunk;
            return newChunk+1;
        }
    }
};

#ifdef USE_STD_STRING
template <class T>
class std_tmp_allocator {
    dbTmpAllocator* allocator;
  public:
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef T *pointer;
    typedef const T *const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef T value_type;

    std_tmp_allocator(dbTmpAllocator& al) : allocator(&al) {}
    std_tmp_allocator() : allocator(NULL) {}
    
    template<class _Other>
    struct rebind  { // convert an std_tmp_allocator<T> to an std_tmp_allocator<_Other>
        typedef std_tmp_allocator<_Other> other;
    };

    pointer allocate(size_type _Count)
    {	// allocate array of _Count elements
        return (allocate(_Count, (pointer)0));
    }

    pointer allocate(size_type _Count, const void*) 
    {
        return (pointer)allocator->alloc(_Count*sizeof(T));
    }

    void deallocate(pointer, size_t) {
    }
 
    size_t max_size() const {
        size_t n = (size_t)(-1) / sizeof(T);
        return (0 < n ? n : 1);
    }
};

#ifndef _THROW0
#define _THROW0 throw
#endif

template<class _Ty, class _Other> inline
bool operator==(const std_tmp_allocator<_Ty>&, const std_tmp_allocator<_Other>&) _THROW0()
{	// test for allocator equality (always true)
    return (true);
}

typedef std::basic_string<char_t,std::char_traits<char_t>,std_tmp_allocator<char_t> > tmp_basic_string;
#endif

class dbXmlContext {
  public:
    oid_t*  oidMap;
    oid_t   oidMapSize;
    dbTmpAllocator tmpAlloc;    
#ifdef USE_STD_STRING
    std_tmp_allocator<char_t> stdAlloc;
    dbXmlContext() : oidMap(NULL), stdAlloc(tmpAlloc) {}
#else
    dbXmlContext() : oidMap(NULL) {}
#endif

    ~dbXmlContext() { 
        delete[] oidMap;
    }
};


class dbXmlScanner { 
  public:
    enum { 
        MaxIdentSize = 256
    };
    enum token { 
        xml_ident, 
        xml_sconst, 
        xml_iconst, 
        xml_fconst, 
        xml_lt, 
        xml_gt, 
        xml_lts, 
        xml_gts,
        xml_eq, 
        xml_eof,
        xml_error
    };    
    dbXmlScanner(FILE* f) { 
        in = f;
        sconst = new char_t[size = 1024];
        line = 1;
        pos = 0;
    }

    ~dbXmlScanner() {
        delete[] sconst;
    }

    token scan();

    void warning(char const* msg)           
    {
        fprintf(stderr, "%s at line %d position %d\n", msg, line, pos > 0 ? pos - 1 : 0);
    }

    char_t* getString() { 
        return sconst;
    }

    char_t* getIdentifier() { 
        return ident;
    }

    size_t  getStringLength() { 
        return slen;
    }

    db_int8 getInt() { 
        return iconst;
    }

    double getReal() { 
        return fconst;
    }

    bool expect(int sourcePos, token expected) { 
        return assure(scan(), sourcePos, expected);
    }

    bool assure(token tkn, int sourcePos, token expected) { 
        if (tkn != expected) { 
            fprintf(stderr, "xml.cpp:%d: line %d, column %d: Get token %d instead of expected token %d\n", 
                    sourcePos, line, pos, tkn, expected);
            return false;
        }
        return true;
    }

    bool expect(int sourcePos, char_t* expected) { 
        token tkn = scan();
        if (tkn != xml_ident) { 
            fprintf(stderr, "xml.cpp:%d: line %d, column %d: Get token %d instead of expected identifier\n", 
                    sourcePos, line, pos, tkn);
            return false;
        }
        if (STRCMP(ident, expected) != 0) { 
            FPRINTF(stderr, STRLITERAL("xml.cpp:%d: line %d, column %d: Get tag '%s' instead of expected '%s'\n"), 
                    sourcePos, line, pos, ident, expected);
            return false;
        }
        return true;
    }

  private:
    int   get();
    void  unget(int ch);

    int       line;
    int       pos;
    FILE*     in;
    char_t*   sconst;
    size_t    size;
    size_t    slen;
    db_int8   iconst;
    double    fconst;
    char_t    ident[MaxIdentSize];
};

static void exportString(FILE* out, char_t* src, int len)
{
    FPRINTF(out, STRLITERAL("\""));
    while (--len > 0) { 
        char_t ch = *src++;
        switch (ch) { 
          case '&':
            FPRINTF(out, _T("&amp;"));
            break;
          case '<':
            FPRINTF(out, _T("&lt;"));
            break;
          case '>':
            FPRINTF(out, _T("&gt;"));
            break;
          case '"':
            FPRINTF(out, _T("&quot;"));
            break;
          default:
            FPRINTF(out, _T("%c"), ch);
        }
    }
    FPRINTF(out, STRLITERAL("\""));
}

static void exportBinary(FILE* out, byte* src, int len)
{
    FPRINTF(out, STRLITERAL("\""));
    while (--len >= 0) { 
        FPRINTF(out, STRLITERAL("%02X"), *src++);
    }
    FPRINTF(out, STRLITERAL("\""));
}

static void exportRecord(dbFieldDescriptor* fieldList, FILE* out, byte* src, int indent)
{
    int i;
    dbFieldDescriptor* fd = fieldList;
    do {
        byte* ptr = src + fd->dbsOffs;
        for (i = indent; --i >= 0;) { 
            FPRINTF(out, STRLITERAL(" "));
        }
        char_t* fieldName = fd->name;
        if (STRCMP(fieldName, STRLITERAL("[]")) == 0) { 
            fieldName = STRLITERAL("array-element");
        }
        FPRINTF(out, STRLITERAL("<%s>"), fieldName);
        switch (fd->type) {
          case dbField::tpBool:
            FPRINTF(out, STRLITERAL("%d"), *(bool*)ptr);
            break;
          case dbField::tpInt1:
            FPRINTF(out, STRLITERAL("%d"), *(int1*)ptr);
            break;
          case dbField::tpInt2:
            FPRINTF(out, STRLITERAL("%d"), *(int2*)ptr);
            break;
          case dbField::tpInt4:
            FPRINTF(out, STRLITERAL("%d"), *(int4*)ptr);
            break;
          case dbField::tpInt8:
            FPRINTF(out, T_INT8_FORMAT, *(db_int8*)ptr);
            break;
          case dbField::tpReal4:
            FPRINTF(out, STRLITERAL("%.8G"), *(real4*)ptr);
            break;
          case dbField::tpReal8:
            FPRINTF(out, STRLITERAL("%.16G"), *(real8*)ptr);
            break;
          case dbField::tpRawBinary:
            exportBinary(out, src+fd->dbsOffs, (int)fd->dbsSize);
            break;
          case dbField::tpString:
            exportString(out, (char_t*)(src + ((dbVarying*)ptr)->offs), ((dbVarying*)ptr)->size);
            break;
          case dbField::tpArray:
            {
                int nElems = ((dbVarying*)ptr)->size;
                byte* srcElem = src + ((dbVarying*)ptr)->offs;
                dbFieldDescriptor* element = fd->components;
                FPRINTF(out, STRLITERAL("\n"));
                while (--nElems >= 0) {
                    exportRecord(element, out, srcElem, indent+1);
                    srcElem += element->dbsSize;
                }
                for (i = indent; --i >= 0;) { 
                    FPRINTF(out, STRLITERAL(" "));
                }
                break;
            }
          case dbField::tpReference:
            FPRINTF(out, STRLITERAL("<ref id=\"%lu\"/>"), (unsigned long)*(oid_t*)ptr);
            break;
          case dbField::tpRectangle:
            { 
                rectangle& r = *(rectangle*)ptr;
                FPRINTF(out, STRLITERAL("<rectangle><vertex"));
                for (i = 0; i < rectangle::dim; i++) { 
                    FPRINTF(out, STRLITERAL(" c%d=\"%d\""), i, r.boundary[i]);
                }
                FPRINTF(out, STRLITERAL("/><vertex"));
                for (i = 0; i < rectangle::dim; i++) { 
                    FPRINTF(out, STRLITERAL(" c%d=\"%d\")"), i, r.boundary[rectangle::dim+i]);
                }
                FPRINTF(out, STRLITERAL("/></rectangle>"));
            }
            break;
          case dbField::tpStructure:
            FPRINTF(out, STRLITERAL("\n"));
            exportRecord(fd->components, out, src, indent+1);
            for (i = indent; --i >= 0;) { 
                FPRINTF(out, STRLITERAL(" "));
            }
            break;
        }
        FPRINTF(out, STRLITERAL("</%s>\n"), fieldName);
    } while ((fd = fd->next) != fieldList);
}

inline bool containsTable(char_t const* const* tables, size_t nTables, const char_t* name) { 
    for (size_t i = 0; i < nTables; i++) { 
        if (STRCMP(tables[i], name) == 0) { 
            return true;
        }
    }
    return false;
}

static bool shouldExportTable(const char_t *name, char_t const* const* tables, size_t nTables, dbDatabase::SelectionMethod method)
{
    switch(method)
    {
    default:
      case dbDatabase::sel_all:            
        return true;        
      case dbDatabase::sel_all_except:    
        return !containsTable(tables, nTables, name);
      case dbDatabase::sel_named_only:    
        return containsTable(tables, nTables, name);
    }
}

void dbDatabase::exportDatabaseToXml(FILE* out, char_t const* const* selectedTables, size_t nTables, SelectionMethod method) 
{
    exportDatabaseToXml(out, selectedTables, nTables, method, 
                        sizeof(char_t) == 1 ? _T("UTF-8") : sizeof(char_t) == 2 ? _T("UTF-16") : _T("UTF-32"));
}

void dbDatabase::exportDatabaseToXml(FILE* out, char_t const* const* selectedTables, size_t nTables, SelectionMethod method, char_t const* encoding) 
{
    dbGetTie tie;
    FPRINTF(out, STRLITERAL("<?xml version=\"1.0\" encoding=\"%s\"?>\n<database>\n"), encoding);
    beginTransaction(dbSharedLock);
    if (tables == NULL) { 
        loadMetaTable();
    }
    for (dbTableDescriptor* desc = tables; desc != NULL; desc=desc->nextDbTable) { 
        if (desc->tableId != dbMetaTableId) {
            if (shouldExportTable(desc->name, selectedTables, nTables, method))
            {
                refreshTable(desc);
                oid_t oid = desc->firstRow; 
                size_t n = desc->nRows;
                int percent = 0;
                for (size_t i = 0; oid != 0; i++) { 
                    dbRecord* rec = getRow(tie, oid);
                    FPRINTF(out, STRLITERAL(" <%s id=\"%ld\">\n"), desc->name, (long)oid); 
                    exportRecord(desc->columns, out, (byte*)rec, 2);
                    FPRINTF(out, STRLITERAL(" </%s>\n"), desc->name); 
                    oid = rec->next;
                    int p = (int)((i+1)*100/n);
                    if (p != percent) { 
                        FPRINTF(stderr, STRLITERAL("Exporting table %s: %d%%\r"), desc->name, p);
                        fflush(stderr);
                        percent = p;
                    }
                }
                
                FPRINTF(stderr, STRLITERAL("Exporting table %s: 100%%\n"), desc->name);
            }
            else
            {
                FPRINTF(stderr, STRLITERAL("*** Skipping table %s\n"), desc->name);
            }
        }
    }
    FPRINTF(out, STRLITERAL("</database>\n"));
}

#define HEX_DIGIT(ch) ((ch) >= 'a' ? ((ch) - 'a' + 10) : (ch) >= 'A' ? (((ch) - 'A' + 10)) : ((ch) - '0'))

inline int dbXmlScanner::get()
{
    int ch = GETC(in);
    if (ch == '\n') {
        pos = 0;
        line += 1;
    } else if (ch == '\t') {
        pos = DOALIGN(pos + 1, 8);
    } else {
        pos += 1;
    }
    return ch;
}

inline void dbXmlScanner::unget(int ch) {
    if (ch != T_EOF) {
        if (ch != '\n') {
            pos -= 1;
        } else {
            line -= 1;
        }
        UNGETC(ch, in);
    }
}


dbXmlScanner::token dbXmlScanner::scan()
{
    int ch, i, pos;
    bool floatingPoint;

  retry:
    do {
        if ((ch = get()) == T_EOF) {
            return xml_eof;
        }
    } while (ch <= ' ');
    
    switch (ch) { 
      case '<':
        ch = get();
        if (ch == '?') { 
            while ((ch = get()) != '?') { 
                if (ch == EOF) { 
                    return xml_error;
                }
            }
            if ((ch = get()) != '>') { 
                return xml_error;
            }
            goto retry;
        } 
        if (ch != '/') { 
            unget(ch);
            return xml_lt;
        }
        return xml_lts;
      case '>':
        return xml_gt;
      case '/':
        ch = get();
        if (ch != '>') { 
            unget(ch);
            return xml_error;
        }
        return xml_gts;
      case '=':
        return xml_eq;
      case '"':
        i = 0;
        while (true) { 
            ch = get();
            switch (ch) { 
              case T_EOF:
                return xml_error;
              case '&':
                switch (get()) { 
                  case 'a':
                    if (get() != 'm' || get() != 'p' || get() != ';') { 
                        return xml_error;
                    }
                    ch = '&';
                    break;
                  case 'l':
                    if (get() != 't' || get() != ';') { 
                        return xml_error;
                    }
                    ch = '<';
                    break;
                  case 'g':
                    if (get() != 't' || get() != ';') { 
                        return xml_error;
                    }
                    ch = '>';
                    break;
                  case 'q':
                    if (get() != 'u' || get() != 'o' || get() != 't' || get() != ';') { 
                        return xml_error;
                    }
                    ch = '"';
                    break;
                  default:
                    return xml_error;
                }
                break;
              case '"':
                slen = i;
                sconst[i] = 0;
                return xml_sconst;
            }
            if ((size_t)i+1 >= size) { 
                char_t* newBuf = new char_t[size *= 2];
                memcpy(newBuf, sconst, i*sizeof(char_t));
                delete[] sconst;
                sconst = newBuf;
            }
            sconst[i++] = (char_t)ch;
        } 
      case '-': case '+':
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        i = 0;
        floatingPoint = false;
        while (true) { 
            if ((size_t)i == size) { 
                return xml_error;
            }
            if (!isdigit(ch) && ch != '-' && ch != '+' && ch != '.' && ch != 'E') { 
                unget(ch);
                sconst[i] = '\0';
                if (floatingPoint) { 
                   return SSCANF(sconst, _T("%lf%n"), &fconst, &pos) == 1 && pos == i
                       ? xml_fconst : xml_error;
                } else { 
                    return SSCANF(sconst, T_INT8_FORMAT _T("%n"), &iconst, &pos) == 1 && pos == i
                       ? xml_iconst : xml_error;
                }
            }
            sconst[i++] = (char_t)ch;
            if (ch == '.' || ch == 'E') { 
                floatingPoint = true;
            }
            ch = get();
        }
      default:
        i = 0;
        while (ISALNUM(ch) || ch == '-' || ch == ':' || ch == '_') { 
            if (i == MaxIdentSize) { 
                return xml_error;
            }
            ident[i++] = (char_t)ch;
            ch = get();
        }
        unget(ch);
        if (i == MaxIdentSize || i == 0) { 
            return xml_error;            
        }
        ident[i] = '\0';
        return xml_ident;
    }
}

#define EXPECT(x) scanner.expect(__LINE__, x)
#define ASSURE(x, y) scanner.assure(x, __LINE__, y)

static bool skipElement(dbXmlScanner& scanner) 
{
    int depth = 1;
    do {  
        switch (scanner.scan()) { 
          case dbXmlScanner::xml_lt:
            depth += 1;
            continue;
          case dbXmlScanner::xml_lts:
            depth -= 1;
            if (depth < 0 || !EXPECT(dbXmlScanner::xml_ident) || !EXPECT(dbXmlScanner::xml_gt))
            { 
                return false;
            }
            break;
          case dbXmlScanner::xml_gts:
            depth -= 1;
            break;
          default:
            continue;            
        }
    } while (depth != 0);

    return true;
}

bool dbDatabase::importRecord(char_t* terminator, dbFieldDescriptor* fieldList, byte* rec, dbXmlScanner& scanner) 
{
    dbXmlScanner::token tkn;

    while ((tkn = scanner.scan()) != dbXmlScanner::xml_lts) { 
        if (!ASSURE(tkn, dbXmlScanner::xml_lt) || !EXPECT(dbXmlScanner::xml_ident)
            || !EXPECT(dbXmlScanner::xml_gt)) 
        { 
            return false;
        }
        char_t* fieldName = scanner.getIdentifier();
        dbSymbolTable::add(fieldName, tkn_ident, GB_CLONE_ANY_IDENTIFIER);
        dbFieldDescriptor* fd = fieldList;
        while (true) {
            if (fd->name == fieldName) {
                if (!importField(fd->name, fd, rec, scanner)) { 
                    return false;
                }
                break;
            }
            if ((fd = fd->next) == fieldList) { 
                if (!skipElement(scanner)) { 
                    return false;
                }   
                break;
            }
        } 
    }
    return EXPECT(terminator) && EXPECT(dbXmlScanner::xml_gt);
} 

bool dbDatabase::importField(char_t* terminator, dbFieldDescriptor* fd, byte* rec, dbXmlScanner& scanner) 
{
    dbXmlScanner::token tkn;
    int i;
    long id;
    byte* dst = rec + fd->appOffs;
    
    switch (fd->appType) { 
      case dbField::tpStructure:
        return importRecord(terminator, fd->components, dst, scanner);
      case dbField::tpArray:
      { 
          int arrSize = 8;
          int elemSize = (int)fd->components->appSize;
          byte* arr = (byte*)xmlContext->tmpAlloc.alloc(elemSize*arrSize);
          memset(arr, 0, elemSize*arrSize);
          for (i = 0; (tkn = scanner.scan()) == dbXmlScanner::xml_lt; i++) { 
              if (!EXPECT(STRLITERAL("array-element"))
                  || !EXPECT(dbXmlScanner::xml_gt))
              {
                  return false;
              }
              if (i == arrSize) { 
                  arrSize *= 2;
                  byte* newArr = (byte*)xmlContext->tmpAlloc.alloc(elemSize*arrSize);
                  memcpy(newArr, arr, i*elemSize);
                  memset(newArr + i*elemSize, 0, i*elemSize);
                  arr = newArr;
              }
              importField(STRLITERAL("array-element"), fd->components, arr + i*elemSize, scanner);
          }
          dbAnyArray::arrayAllocator((dbAnyArray*)dst, arr, i); 
          return ASSURE(tkn, dbXmlScanner::xml_lts)
              && EXPECT(terminator)
              && EXPECT(dbXmlScanner::xml_gt);
      }
      case dbField::tpReference:
        if (!EXPECT(dbXmlScanner::xml_lt)
            || !EXPECT(STRLITERAL("ref"))
            || !EXPECT(STRLITERAL("id"))
            || !EXPECT(dbXmlScanner::xml_eq)
            || !EXPECT(dbXmlScanner::xml_sconst)
            || SSCANF(scanner.getString(), _T("%ld"), &id) != 1
            || !EXPECT(dbXmlScanner::xml_gts))
        { 
            return false;
        }
        *(oid_t*)dst = mapId(id);
        break;
      case dbField::tpRectangle:
        if (!EXPECT(STRLITERAL("rectangle"))
            || !EXPECT(dbXmlScanner::xml_gt)
            || !EXPECT(dbXmlScanner::xml_lt)
            || !EXPECT(STRLITERAL("vertex")))
        { 
            return false;
        } else {
            rectangle& r = *(rectangle*)dst;
            for (i = 0; i < rectangle::dim; i++) { 
                if (!EXPECT(dbXmlScanner::xml_ident)
                    || !EXPECT(dbXmlScanner::xml_eq)
                    || !EXPECT(dbXmlScanner::xml_sconst)
                    || SSCANF(scanner.getString(), _T("%d"), &r.boundary[i]) != 1)
                {
                    return false;
                }
            }
            if (!EXPECT(dbXmlScanner::xml_gts)
                || !EXPECT(dbXmlScanner::xml_lt)
                || !EXPECT(STRLITERAL("vertex")))
            {
                return false;
            }
            for (i = 0; i < rectangle::dim; i++) { 
                if (!EXPECT(dbXmlScanner::xml_ident)
                    || !EXPECT(dbXmlScanner::xml_eq)
                    || !EXPECT(dbXmlScanner::xml_sconst)
                    || SSCANF(scanner.getString(), _T("%d"), &r.boundary[rectangle::dim+i]) != 1)
                {
                    return false;
                }
            }
            if (!EXPECT(dbXmlScanner::xml_gts)
                || !EXPECT(dbXmlScanner::xml_lts)
                || !EXPECT(STRLITERAL("rectangle"))
                || !EXPECT(dbXmlScanner::xml_gt))
            {
                return false;
            }
            break;
        }
      case dbField::tpBool:
        switch (scanner.scan()) { 
          case dbXmlScanner::xml_iconst:
            *(bool*)dst = scanner.getInt() != 0;
            break;
          case dbXmlScanner::xml_fconst:
            *(bool*)dst = scanner.getReal() != 0.0;
            break;
          default:
            scanner.warning("Failed to convert field");
        }
        break;            
      case dbField::tpInt1:
        switch (scanner.scan()) { 
          case dbXmlScanner::xml_iconst:
            *(int1*)dst = (int1)scanner.getInt();
            break;
          case dbXmlScanner::xml_fconst:
            *(int1*)dst = (int1)scanner.getReal();
            break;
          default:
            scanner.warning("Failed to convert field");
        }
        break;            
      case dbField::tpInt2:
        switch (scanner.scan()) { 
          case dbXmlScanner::xml_iconst:
            *(int2*)dst = (int2)scanner.getInt();
            break;
          case dbXmlScanner::xml_fconst:
            *(int2*)dst = (int2)scanner.getReal();
            break;
          default:
            scanner.warning("Failed to convert field");
        }
        break;            
      case dbField::tpInt4:
        switch (scanner.scan()) { 
          case dbXmlScanner::xml_iconst:
            *(int4*)dst = (int4)scanner.getInt();
            break;
          case dbXmlScanner::xml_fconst:
            *(int4*)dst = (int4)scanner.getReal();
            break;
          default:
            scanner.warning("Failed to convert field");
        }
        break;            
      case dbField::tpInt8:
        switch (scanner.scan()) { 
          case dbXmlScanner::xml_iconst:
            *(db_int8*)dst = scanner.getInt();
            break;
          case dbXmlScanner::xml_fconst:
            *(db_int8*)dst = (db_int8)scanner.getReal();
            break;
          default:
            scanner.warning("Failed to convert field");
        }
        break;            
      case dbField::tpReal4:
        switch (scanner.scan()) { 
          case dbXmlScanner::xml_iconst:
            *(real4*)dst = (real4)scanner.getInt();
            break;
          case dbXmlScanner::xml_fconst:
            *(real4*)dst = (real4)scanner.getReal();
            break;
          default:
            scanner.warning("Failed to convert field");
        }
        break;            
      case dbField::tpReal8:
        switch (scanner.scan()) { 
          case dbXmlScanner::xml_iconst:
            *(real8*)dst = (real8)scanner.getInt();
            break;
          case dbXmlScanner::xml_fconst:
            *(real8*)dst = scanner.getReal();
            break;
          default:
            scanner.warning("Failed to convert field");
        }
        break;            
      case dbField::tpString:
        switch (scanner.scan()) { 
          case dbXmlScanner::xml_iconst:
          case dbXmlScanner::xml_sconst:
          case dbXmlScanner::xml_fconst:
          { 
              char_t* str = (char_t*)xmlContext->tmpAlloc.alloc(sizeof(char_t)*(scanner.getStringLength()+1));
              memcpy(str, scanner.getString(), scanner.getStringLength()*sizeof(char_t));
              str[scanner.getStringLength()] = '\0';
              *(char_t**)dst = str;
              break;
          }
          default:
            scanner.warning("Failed to convert field");
        }
        break;            
      case dbField::tpRawBinary:
      {
          if (scanner.scan() != dbXmlScanner::xml_sconst) { 
              scanner.warning("Failed to convert field");
              break;
          }
          char_t* src = scanner.getString();
          int len = (int)(scanner.getStringLength() >> 1);
          if (fd->appSize != (size_t)len) { 
              scanner.warning("Length of raw binary field was changed");
          } else { 
              while (--len >= 0) { 
                  *dst++ = (HEX_DIGIT(src[0]) << 4) | HEX_DIGIT(src[1]);
                  src += 2;
              }
          }
          break;
      }
#ifdef USE_MFC_STRING
      case dbField::tpMfcString:
        if (scanner.scan() != dbXmlScanner::xml_sconst) { 
            scanner.warning("Failed to convert field");
            break;
        }
        *(MFC_STRING*)dst = scanner.getString();
        break;
#endif
#ifdef USE_STD_STRING
      case dbField::tpStdString:
        if (scanner.scan() != dbXmlScanner::xml_sconst) { 
            scanner.warning("Failed to convert field");
            break;
        }
        new (dst) tmp_basic_string(scanner.getString(), scanner.getStringLength(), xmlContext->stdAlloc);
        break;
#endif
    }    
    return EXPECT(dbXmlScanner::xml_lts)
        && EXPECT(terminator)
        && EXPECT(dbXmlScanner::xml_gt); 
}

oid_t dbDatabase::mapId(long id)
{
    oid_t oid;
    if (id == 0) { 
        return 0;
    }
    if ((oid_t)id >= xmlContext->oidMapSize) { 
        oid_t* newOidMap = new oid_t[id*2];
        memcpy(newOidMap, xmlContext->oidMap, (size_t)xmlContext->oidMapSize*sizeof(oid_t));
        memset(newOidMap + xmlContext->oidMapSize, 0, (size_t)(id*2-xmlContext->oidMapSize)*sizeof(oid_t));
        xmlContext->oidMapSize = id*2;
        xmlContext->oidMap = newOidMap;
    }
    oid = xmlContext->oidMap[id];
    if (oid == 0) { 
        oid = allocateId();
        xmlContext->oidMap[id] = oid;
    }
    return oid;
}

bool dbDatabase::insertRecord(dbTableDescriptor* desc, oid_t oid, void const* record) 
{
    dbFieldDescriptor* fd;
    byte* src = (byte*)record;
    size_t size = desc->columns->calculateRecordSize(src, desc->fixedSize);
    allocateRow(desc->tableId, oid, size, desc);
    {
        dbPutTie tie;
        dbRecord* dst = putRow(tie, oid);
#ifdef AUTOINCREMENT_SUPPORT
        int4 autoincrementCount = desc->autoincrementCount;
        desc->columns->storeRecordFields((byte*)dst, src, desc->fixedSize, dbFieldDescriptor::Import);
        if (autoincrementCount != desc->autoincrementCount) {
            dbPutTie tie;
            dbTable* table = (dbTable*)putRow(tie, desc->tableId);
            table->count = desc->autoincrementCount;
        }  
#else
        desc->columns->storeRecordFields((byte*)dst, src, desc->fixedSize, dbFieldDescriptor::Import);
#endif
    }
    for (fd = desc->indexedFields; fd != NULL; fd = fd->nextIndexedField) {
        if ((fd->indexType & UNIQUE) != 0 && fd->type != dbField::tpRectangle) { 
            if (!dbBtree::insert(this, fd->bTree, oid, fd->dbsOffs, fd->comparator)) { 
                for (dbFieldDescriptor* fdu = desc->indexedFields; fdu != fd; fdu = fdu->nextIndexedField) {
                    if ((fdu->indexType & UNIQUE) != 0 && fdu->type != dbField::tpRectangle) { 
                        dbBtree::remove(this, fdu->bTree, oid, fdu->dbsOffs, fdu->comparator);
                    }
                }
                freeRow(desc->tableId, oid, desc);
                return false;
            }
        }
    }
    size_t nRows = desc->nRows;
    for (fd = desc->hashedFields; fd != NULL; fd = fd->nextHashedField) {
        dbHashTable::insert(this, fd->hashTable, oid, fd->type, fd->dbsOffs, nRows);
    }
    for (fd = desc->indexedFields; fd != NULL; fd = fd->nextIndexedField) {
        if (fd->type == dbField::tpRectangle) { 
            dbRtree::insert(this, fd->bTree, oid, fd->dbsOffs);
        } else if ((fd->indexType & UNIQUE) == 0) { 
            dbBtree::insert(this, fd->bTree, oid, fd->dbsOffs, fd->comparator);
        }
    }
    return true;
}

bool dbDatabase::importDatabaseFromXml(FILE* in)
{
    dbXmlContext ctx;
    xmlContext = &ctx;
    dbXmlScanner scanner(in);
    dbXmlScanner::token tkn;
    
    if (!EXPECT(dbXmlScanner::xml_lt) ||
        !EXPECT(STRLITERAL("database")) || 
        !EXPECT(dbXmlScanner::xml_gt))
    {
        return false;
    }

    beginTransaction(dbExclusiveLock);
    if (tables == NULL) { 
        loadMetaTable();
    }

    ctx.oidMapSize = dbDefaultInitIndexSize;
    ctx.oidMap = new oid_t[(size_t)ctx.oidMapSize];
    memset(ctx.oidMap, 0, (size_t)ctx.oidMapSize*sizeof(oid_t));

    while ((tkn = scanner.scan()) != dbXmlScanner::xml_lts) { 
        if (!ASSURE(tkn, dbXmlScanner::xml_lt) || !EXPECT(dbXmlScanner::xml_ident)) { 
            return false;
        }
        dbTableDescriptor* desc = findTableByName(scanner.getIdentifier());
        if (desc == NULL) { 
            FPRINTF(stderr, STRLITERAL("Table '%s' not found\n"), scanner.getIdentifier());
        }
        if (!EXPECT(STRLITERAL("id"))
            || !EXPECT(dbXmlScanner::xml_eq)
            || !EXPECT(dbXmlScanner::xml_sconst)
            || !EXPECT(dbXmlScanner::xml_gt)) 
        {
            return false;
        }
        if (desc != NULL) { 
            long id;
            if (SSCANF(scanner.getString(), _T("%ld"), &id) != 1) { 
                return false;
            }
            oid_t oid = mapId(id);
            byte *record = (byte*)xmlContext->tmpAlloc.alloc(desc->appSize);  
            memset(record, 0, desc->appSize);
            if (!importRecord(desc->name, desc->columns, record, scanner)) {                 
                xmlContext->tmpAlloc.reset();
                return false;
            }
            if (!insertRecord(desc, oid, record)) { 
                FPRINTF(stderr, STRLITERAL("Unique constrain violation for table %s\n"), desc->name);
                return false;
            }
            xmlContext->tmpAlloc.reset();
        } else { // skip record
            if (!skipElement(scanner)) { 
                return false;
            }
        }    
    }
    return EXPECT(STRLITERAL("database")) && EXPECT(dbXmlScanner::xml_gt);
}

END_GIGABASE_NAMESPACE
