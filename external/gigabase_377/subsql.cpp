//-< SUBSQL.CPP >----------------------------------------------------*--------*
// GigaBASE                  Version 1.0         (c) 1999  GARRET    *     ?  *
// (Post Relational Database Management System)                      *   /\|  *
//                                                                   *  /  \  *
//                          Created:     20-Nov-98    K.A. Knizhnik  * / [] \ *
//                          Last update: 10-Dec-98    K.A. Knizhnik  * GARRET *
//-------------------------------------------------------------------*--------*
// Interactive data manipulation language (subset of SQL)
//-------------------------------------------------------------------*--------*

#include "gigabase.h"
#include "compiler.h"
#include "wwwapi.h"
#include "symtab.h"
#include "hashtab.h"
#include "btree.h"
#include "rtree.h"
#include "subsql.h"
#include <locale.h>

#if THREADS_SUPPORTED
#include "server.h"
#endif

static char_t const* typeMnem[] = {
        STRLITERAL("Boolean"),
        STRLITERAL("Int1"),
        STRLITERAL("Int2"),
        STRLITERAL("Int4"),
        STRLITERAL("Int8"),
        STRLITERAL("Real4"),
        STRLITERAL("Real8"),
        STRLITERAL("String"),
        STRLITERAL("Reference"),
        STRLITERAL("Array"),
        STRLITERAL("MethodBool"),
        STRLITERAL("MethodInt1"),
        STRLITERAL("MethodInt2"),
        STRLITERAL("MethodInt4"),
        STRLITERAL("MethodInt8"),
        STRLITERAL("MethodReal4"),
        STRLITERAL("MethodReal8"),
        STRLITERAL("MethodString"),
        STRLITERAL("MethodReference"),
        STRLITERAL("Structure"),
        STRLITERAL("RawBinary"),
        STRLITERAL("StdString"),
        STRLITERAL("MfcString"),
        STRLITERAL("Rectangle"),
        STRLITERAL("Unknown") 
};

#ifndef OID_FORMAT
#if dbDatabaseOidBits > 32
#define OID_FORMAT "#" INT8_FORMAT_PREFIX "x"
#else
#define OID_FORMAT "#%x"
#endif
#endif



char const* dbSubSql::prompt = ">> ";
const int initBufSize = 4096;


static bool interactiveMode;



dbSubSql::dbSubSql(dbAccessType accessType, size_t pagePoolSize)
: dbDatabase(accessType, pagePoolSize)
{
    static struct {
        char_t* name;
        int     tag;
    } keywords[] = {
        {STRLITERAL("alter"),   tkn_alter},
        {STRLITERAL("array"),   tkn_array},
        {STRLITERAL("autocommit"),   tkn_autocommit},
        {STRLITERAL("autoincrement"),tkn_autoincrement},
        {STRLITERAL("backup"),  tkn_backup},
        {STRLITERAL("bool"),    tkn_bool},
        {STRLITERAL("commit"),  tkn_commit},
        {STRLITERAL("compactify"),tkn_compactify},
        {STRLITERAL("count"),   tkn_count},
        {STRLITERAL("create"),  tkn_create},
        {STRLITERAL("delete"),  tkn_delete},
        {STRLITERAL("describe"),tkn_describe},
        {STRLITERAL("drop"),    tkn_drop},
        {STRLITERAL("exit"),    tkn_exit},
        {STRLITERAL("export"),  tkn_export},
        {STRLITERAL("hash"),    tkn_hash},
        {STRLITERAL("help"),    tkn_help},
        {STRLITERAL("http"),    tkn_http},        
        {STRLITERAL("import"),  tkn_import},
        {STRLITERAL("index"),   tkn_index},
        {STRLITERAL("inverse"), tkn_inverse},
        {STRLITERAL("int1"),    tkn_int1},
        {STRLITERAL("int2"),    tkn_int2},
        {STRLITERAL("int4"),    tkn_int4},
        {STRLITERAL("int8"),    tkn_int8},
        {STRLITERAL("memory"),  tkn_memory},
        {STRLITERAL("of"),      tkn_of},
        {STRLITERAL("off"),     tkn_off},
        {STRLITERAL("on"),      tkn_on},
        {STRLITERAL("open"),    tkn_open},
        {STRLITERAL("profile"), tkn_profile},        
        {STRLITERAL("reference"),tkn_reference},
        {STRLITERAL("real4"),   tkn_real4},
        {STRLITERAL("real8"),   tkn_real8},
        {STRLITERAL("rectangle"), tkn_rectangle},
        {STRLITERAL("rename"),  tkn_rename},
        {STRLITERAL("restore"), tkn_restore},
        {STRLITERAL("rollback"),tkn_rollback},
        {STRLITERAL("server"),  tkn_server},
        {STRLITERAL("set"),     tkn_set},
        {STRLITERAL("start"),   tkn_start},
        {STRLITERAL("stop"),    tkn_stop},
        {STRLITERAL("show"),    tkn_show},
        {STRLITERAL("to"),      tkn_to},
        {STRLITERAL("update"),  tkn_update},
        {STRLITERAL("values"),  tkn_values},
        {STRLITERAL("version"), tkn_version}
    };
    for (unsigned i = 0; i < itemsof(keywords); i++) {
        dbSymbolTable::add(keywords[i].name, keywords[i].tag, GB_CLONE_ANY_IDENTIFIER);
    }
    buflen = initBufSize;
    buf = new char_t[buflen];
    droppedTables = NULL;
    existedTables = NULL;
    opened = false;
    httpServerRunning = false;
    databasePath = NULL;
    historyUsed = historyCurr = 0;
    autocommit = false;
    ungetToken = -1;
    dotIsPartOfIdentifier = false;

#ifdef _WIN32_WCE
    dateFormat = NULL;
#else 
    dateFormat = GETENV(STRLITERAL("SUBSQL_DATE_FORMAT"));
#endif 
}

dbSubSql::~dbSubSql() { delete[] buf; }


inline int strincmp(const char_t* p, const char_t* q, size_t n)
{
    while (n > 0) { 
        int diff = TOUPPER(*(char_t*)p) - TOUPPER(*(char_t*)q);
        if (diff != 0) { 
            return diff;
        } else if (*p == '\0') { 
            return 0;
        }
        p += 1;
        q += 1;
        n -= 1; 
    }
    return 0;
}

//
// Find one string within another, ignoring case
//

inline char_t* stristr(const char_t* haystack, const char_t* needle)
{
    size_t i, hayLen, ndlLen;

    ndlLen = STRLEN(needle);
    hayLen = STRLEN(haystack);

    if (ndlLen > hayLen) {
        return NULL;
    }

    for (i = 0; i <= (hayLen - ndlLen); i++) {
        if (strincmp(&haystack[i], needle, ndlLen) == 0) {
            return (char_t*)&haystack[i];
        }
    }
    return NULL;
}


bool  __cdecl contains(dbUserFunctionArgument& arg1, dbUserFunctionArgument& arg2) { 
    assert(arg1.type == dbUserFunctionArgument::atString && arg2.type == dbUserFunctionArgument::atString);
    return stristr(arg1.u.strValue, arg2.u.strValue) != NULL;
}

USER_FUNC(contains);

inline int dbSubSql::get()
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

inline void dbSubSql::unget(int ch) {
    if (ch != T_EOF) {
        if (ch != '\n') {
            pos -= 1;
        } else {
            line -= 1;
        }
        UNGETC(ch, in);
    }
}

void dbSubSql::warning(char const* msg)
{
    fprintf(stderr, "%s at line %d position %d\n", msg, line, tknPos > 0 ? tknPos - 1 : 0);
}

void dbSubSql::error(char const* msg)
{
#ifdef THROW_EXCEPTION_ON_ERROR
    dbDatabaseThreadContext* ctx = threadContext.get();
    if (ctx != NULL) {
        ctx->interactive = true;
    }
    try {
        handleError(QueryError, msg, tknPos > 0 ? tknPos - 1 : 0);
    } catch(dbException) {}
#else
    dbDatabaseThreadContext* ctx = threadContext.get();
    if (ctx != NULL) {
        ctx->interactive = true;
        ctx->catched = true;
        if (setjmp(ctx->unwind) == 0) {
            handleError(QueryError, msg, tknPos > 0 ? tknPos - 1 : 0);
        }
        ctx->catched = false;
    } else {
        handleError(QueryError, msg, tknPos > 0 ? tknPos - 1 : 0);
    }
#endif
}


int dbSubSql::scan()
{
    int i, ch, nextCh, digits;
    char numbuf[64];

    if (ungetToken >= 0) { 
        int tkn = ungetToken;
        ungetToken = -1;
        return tkn;
    }
    bool dotIsIdentifierChar = dotIsPartOfIdentifier;
    dotIsPartOfIdentifier = false;
  nextToken:
    do {
        if ((ch = get()) == T_EOF) {
            return tkn_eof;
        }
    } while (ch > 0 && ch <= ' ');

    tknPos = pos;
    switch (ch) {
      case '*':
        return tkn_all;
      case '(':
        return tkn_lpar;
      case ')':
        return tkn_rpar;
      case ',':
        return tkn_comma;
      case '.':
        return tkn_dot;
      case ';':
        return tkn_semi;
      case '=':
        return tkn_eq;
      case '\'':
        i = 0;
        while (true) {
            ch = get();
            if (ch == '\'') {
                if ((ch = get()) != '\'') {
                    unget(ch);
                    break;
                }
            } else if (ch == '\n' || ch == T_EOF) {
                unget(ch);
                error("New line within character constant");
                return tkn_error;
            }
            if (i+1 == buflen) {
                char_t* newbuf = new char_t[buflen*2];
                memcpy(newbuf, buf, buflen*sizeof(char_t));
                delete[] buf;
                buf = newbuf;
                buflen *= 2;
            }
            buf[i++] = ch;
        }
        buf[i] = '\0';
        return tkn_sconst;
      case '-':
        if ((ch = get()) == '-') {
            // ANSI comments
            while ((ch = get()) != T_EOF && ch != '\n');
            goto nextToken;
        }
        unget(ch);
        ch = '-';

        // no break
      case '+':
          nextCh = get(); unget(nextCh);    //< peek next char
          if (!isdigit(nextCh))
          {
              if (ch == '-') return tkn_exclude;
              if (ch == '+') return tkn_include;
              error("Expected '+' or '-'"); return tkn_error;
          }
          // no break

      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        i = 0;
        do {
            if (i == sizeof(numbuf)) {
                error("Numeric constant too long");
                return tkn_error;
            }
            numbuf[i++] = (char)ch;
            ch = get();
        } while (ch != T_EOF
                 && ((ch >= '0' && ch <= '9') || ch == '+' || ch == '-' || ch == 'e' ||
                     ch == 'E' || ch == '.'));
        unget(ch);
        numbuf[i] = '\0';
        if (sscanf(numbuf, INT8_FORMAT "%n", &ival, &digits) != 1) {
            error("Bad integer constant");
            return tkn_error;
        }
        if (digits != i) {
            if (sscanf(numbuf, "%lf%n", &fval, &digits) != 1 || digits != i) {
                error("Bad float constant");
                return tkn_error;
            }
            return tkn_fconst;
        }
        return tkn_iconst;

      case '#':
        ival = 0;
        while (true) { 
            ch = get();
            if (ch >= '0' && ch <= '9') { 
                ival = (ival << 4) + ch-'0';
            } else if (ch >= 'a' && ch <= 'f') {
                ival = (ival << 4) +  ch-'a'+10;
            } else if (ch >= 'A' && ch <= 'F') {
                ival = (ival << 4) + ch-'A'+10;
            } else { 
                unget(ch);
                return tkn_iconst;
            }
        }
      default:
        if (ISALNUM(ch) || ch == '$' || ch == '_') {
            i = 0;
            do {
                if (i == buflen) {
                    error("Identifier too long");
                    return tkn_error;
                }
                buf[i++] = ch;
                ch = get();
            } while (ch != T_EOF && (ISALNUM(ch) || ch == '$' || ch == '_' || (ch == '.' && dotIsIdentifierChar)));
            unget(ch);
            buf[i] = '\0';
            name = buf;
            return dbSymbolTable::add(name, tkn_ident);
        } else {
            error("Invalid symbol");
            return tkn_error;
        }
    }
}


bool dbSubSql::expect(char const* expected, int token)
{
    int tkn = scan();
    if (tkn != token) {
        if (tkn != tkn_error) {
            char buf[256];
            sprintf(buf, "Token '%s' expected", expected);
            error(buf);
        }
        return false;
    }
    return true;
}


bool dbSubSql::updateTable(bool create)
{
    int tkn;
    dotIsPartOfIdentifier = true;
    if (!expect("table name", tkn_ident) || !expect("(", tkn_lpar)) {
        return false;
    }
    char_t* name = this->name;
    int varyingLength = (int)((STRLEN(name)+1)*sizeof(char_t));

    static const struct {
        int size;
        int alignment;
    } typeDesc[] = {
        { sizeof(bool), sizeof(bool) },
        { sizeof(int1), sizeof(int1) },
        { sizeof(int2), sizeof(int2) },
        { sizeof(int4), sizeof(int4) },
        { sizeof(db_int8), sizeof(db_int8) },
        { sizeof(real4), sizeof(real4) },
        { sizeof(real8), sizeof(real8) },
        { sizeof(dbVarying), 4 },
        { sizeof(oid_t), sizeof(oid_t) },
        { sizeof(dbVarying), 4 },
        {0}, // tpMethodBool,
        {0}, // tpMethodInt1,
        {0}, // tpMethodInt2,
        {0}, // tpMethodInt4,
        {0}, // tpMethodInt8,
        {0}, // tpMethodReal4,
        {0}, // tpMethodReal8,
        {0}, // tpMethodString,
        {0}, // tpMethodReference,
        {0}, // tpStructure,
        {0}, // tpRawBinary,
        {0}, // tpStdString,
        {0}, // tpMfcString,
        { sizeof(rectangle), sizeof(coord_t) }, // tpRectangle,
        {0} // tpUnknown
    };

    const int maxFields = 256;
    tableField fields[maxFields];
    int nFields = 0;
    int nColumns = 0;
    tkn = tkn_comma;
    while (tkn == tkn_comma) {
        if (nFields+1 == maxFields) {
            error("Too many fields");
            break;
        }
        if (!expect("field name", tkn_ident)) {
            break;
        }
        int nameLen = (int)(STRLEN(buf)+1);
        fields[nFields].name = new char_t[nameLen];
        STRCPY(fields[nFields].name, buf);
        varyingLength += (nameLen + 2)*sizeof(char_t);
        char_t* refTableName;
        char_t* inverseRefName;
        int type = parseType(refTableName, inverseRefName);
        fields[nFields++].type = type;
        if (type == dbField::tpUnknown) {
            break;
        }
        nColumns += 1;
        if (type == dbField::tpArray) {
            if (nFields+1 == maxFields) {
                error("Too many fields");
                break;
            }
            fields[nFields].name = new char_t[nameLen+2];
            SPRINTF(SPRINTF_BUFFER(fields[nFields].name), STRLITERAL("%s[]"), fields[nFields-1].name);
            varyingLength += (nameLen+2+2)*sizeof(char_t);
            type = parseType(refTableName, inverseRefName);
            if (type == dbField::tpUnknown) {
                break;
            }
            if (type == dbField::tpArray) {
                error("Arrays of arrays are not supported by CLI");
                break;
            }
            if (type == dbField::tpReference) {
                fields[nFields].refTableName = refTableName;
                varyingLength += (int)(STRLEN(refTableName)*sizeof(char_t));
                if (inverseRefName != NULL) { 
                    fields[nFields-1].inverseRefName = inverseRefName;
                    varyingLength += (int)(STRLEN(inverseRefName)*sizeof(char_t));
                }                   
            }
            fields[nFields++].type = type;
        } else if (type == dbField::tpReference) {
            fields[nFields-1].refTableName = refTableName;
            varyingLength += (int)(STRLEN(refTableName)*sizeof(char_t));
            if (inverseRefName != NULL) { 
                fields[nFields-1].inverseRefName = inverseRefName;
                varyingLength += (int)(STRLEN(inverseRefName)*sizeof(char_t));
            }                   
        }
        tkn = scan();
    }
    if (tkn == tkn_rpar) {
        dbTableDescriptor* oldDesc = findTable(name);
        if (oldDesc != NULL) {
            if (create) { 
                error("Table already exists");
                return false;
            }
        } else { 
            if (!create) { 
                error("Table not found");
                return false;
            }
        }
        beginTransaction(dbExclusiveLock);
        dbPutTie putTie;
        dbTable* table; 
        oid_t oid;
        
        if (create) { 
            modified = true;
            oid = allocateRow(dbMetaTableId,
                              sizeof(dbTable) + sizeof(dbField)*nFields + varyingLength);
            table = (dbTable*)putRow(putTie, oid);
        } else { 
            oid = oldDesc->tableId;
            table = (dbTable*)new char[sizeof(dbTable) + sizeof(dbField)*nFields + varyingLength];
        }
        int offs = sizeof(dbTable) + sizeof(dbField)*nFields;
        table->name.offs = offs;
        table->name.size = (nat4)STRLEN(name)+1;
        STRCPY((char_t*)((byte*)table + offs), name);
        offs += table->name.size*sizeof(char_t);
        size_t size = sizeof(dbRecord);
        table->fields.offs = sizeof(dbTable);
        dbField* field = (dbField*)((char*)table + table->fields.offs);
        offs -= sizeof(dbTable);
        bool arrayComponent = false;

        for (int i = 0; i < nFields; i++) {
            field->name.offs = offs;
            field->name.size = (nat4)(STRLEN(fields[i].name) + 1);
            STRCPY((char_t*)((char*)field + offs), fields[i].name);
            offs += field->name.size*sizeof(char_t);

            field->tableName.offs = offs;
            if (fields[i].refTableName) {
                field->tableName.size = (nat4)(STRLEN(fields[i].refTableName) + 1);
                STRCPY((char_t*)((byte*)field + offs), fields[i].refTableName);
                offs += field->tableName.size*sizeof(char_t);
            } else {
                field->tableName.size = 1;
                *(char_t*)((char*)field + offs) = '\0';
                offs += sizeof(char_t);
            }

            field->inverse.offs = offs;
            if (fields[i].inverseRefName) {
                field->inverse.size = (nat4)(STRLEN(fields[i].inverseRefName) + 1);
                STRCPY((char_t*)((byte*)field + offs), fields[i].inverseRefName);
                offs += field->inverse.size*sizeof(char_t);
            } else {
                field->inverse.size = 1;
                *(char_t*)((char*)field + offs) = '\0';
                offs += sizeof(char_t);
            }

            field->flags = 0;
            field->type = fields[i].type;
            field->size = typeDesc[fields[i].type].size;
            if (!arrayComponent) {
                size = DOALIGN(size, typeDesc[fields[i].type].alignment);
                field->offset = (int4)size;
                size += field->size;
            } else {
                field->offset = 0;
            }
            field->hashTable = 0;
            field->bTree = 0;
            arrayComponent = field->type == dbField::tpArray;
            field += 1;
            offs -= sizeof(dbField);
        }
        table->fields.size = nFields;
        table->fixedSize = (nat4)size;
        table->nRows = 0;
        table->nColumns = nColumns;
        table->firstRow = 0;
        table->lastRow = 0;

        if (create) { 
            linkTable(new dbTableDescriptor(table), oid);
        } else { 
            dbGetTie getTie;
            dbTableDescriptor* newDesc = new dbTableDescriptor(table);      
            delete[] (char*)table;
            dbTable* oldTable = (dbTable*)getRow(getTie, oid);
            if (!newDesc->equal(oldTable)) {
                bool saveConfirmDeleteColumns = confirmDeleteColumns; 
                confirmDeleteColumns = true;
                modified = true;
                schemeVersion += 1;
                unlinkTable(oldDesc);
                if (oldTable->nRows == 0) {
                    updateTableDescriptor(newDesc, oid, oldTable);
                } else {
                    reformatTable(oid, newDesc);
                }
                delete oldDesc;
                confirmDeleteColumns = saveConfirmDeleteColumns;
                addIndices(newDesc);
            }
        }
        if (!completeDescriptorsInitialization()) {
            warning("Reference to undefined table");
        }
    }
    return tkn == tkn_rpar;
}

int dbSubSql::parseType(char_t*& refTableName, char_t*& inverseRefName)
{
    switch (scan()) {
      case tkn_bool:
        return dbField::tpBool;
      case tkn_int1:
        return dbField::tpInt1;
      case tkn_int2:
        return dbField::tpInt2;
      case tkn_int4:
        return dbField::tpInt4;
      case tkn_int8:
        return dbField::tpInt8;
      case tkn_real4:
        return dbField::tpReal4;
      case tkn_real8:
        return dbField::tpReal8;
      case tkn_array:
        return expect("of", tkn_of) ? dbField::tpArray : dbField::tpUnknown;
      case tkn_string:
        return dbField::tpString;
      case tkn_reference:
        if (expect("to", tkn_to) && expect("referenced table name", tkn_ident)) {
            refTableName = new char_t[STRLEN(buf)+1];
            STRCPY(refTableName, buf);
            int tkn = scan();
            if (tkn == tkn_inverse) {
                if (!expect("inverse reference field name", tkn_ident)) { 
                    return dbField::tpUnknown;
                }
                inverseRefName = new char_t[STRLEN(buf)+1];
                STRCPY(inverseRefName, buf);
            } else { 
                inverseRefName = NULL;
                ungetToken = tkn;
            }
            return dbField::tpReference;
        } else { 
            return dbField::tpUnknown;
        }
      case tkn_rectangle:
        return dbField::tpRectangle;
      default:
        error("Field type expected");
    }
    return dbField::tpUnknown;
}


int dbSubSql::readExpression()
{
    int i, ch;
    int nesting = 0;
    for (i = 0; (ch = get()) != ';' && (ch != ',' || nesting != 0)  && ch != EOF; i++) { 
        if (ch == '(') { 
            nesting += 1;
        } else if (ch == ')') { 
            nesting -= 1;
        }
        if (i+1 >= buflen) { 
            char_t* newbuf = new char_t[buflen*2];
            memcpy(newbuf, buf, buflen*sizeof(char_t));
            delete[] buf;
            buf = newbuf;
            buflen *= 2;
        }
        buf[i] = ch;
    }
    buf[i] = '\0';
    return ch;
}

bool dbSubSql::readCondition()
{
    int i, ch;
    for (i = 0; (ch = get()) != ';' && ch !=  T_EOF; i++) {
        if (i+1 == buflen) {
            char_t* newbuf = new char_t[buflen*2];
            memcpy(newbuf, buf, buflen*sizeof(char_t));
            delete[] buf;
            buf = newbuf;
            buflen *= 2;
        }
        buf[i] = ch;
    }
    buf[i] = '\0';
    if (ch != ';') {
        error("unexpected end of input");
        return false;
    }
    return true;
}


void dbSubSql::dumpRecord(byte* base, dbFieldDescriptor* first)
{
    int i, n;
    byte* elem;
    dbFieldDescriptor* fd = first;
    do {
        if (fd != first) {
            printf(", ");
        }
        switch (fd->type) {
          case dbField::tpBool:
            printf("%s", *(bool*)(base + fd->dbsOffs)
                   ? "true" : "false");
            continue;
          case dbField::tpInt1:
            printf("%d", *(int1*)(base + fd->dbsOffs));
            continue;
          case dbField::tpInt2:
            printf("%d", *(int2*)(base + fd->dbsOffs));
            continue;
          case dbField::tpInt4:
            printf("%d", *(int4*)(base + fd->dbsOffs));
            continue;
          case dbField::tpInt8:
            printf(INT8_FORMAT, *(db_int8*)(base + fd->dbsOffs));
            continue;
          case dbField::tpReal4:
            printf("%f", *(real4*)(base + fd->dbsOffs));
            continue;
          case dbField::tpReal8:
            printf("%f", *(real8*)(base + fd->dbsOffs));
            continue;
          case dbField::tpRectangle:
            {
                int i, sep = '(';
                rectangle& r = *(rectangle*)(base + fd->dbsOffs);
                for (i = 0; i < rectangle::dim*2; i++) { 
                    printf("%c%f", sep, (double)r.boundary[i]);
                    sep = ',';
                }
                printf(")");
            }
            continue;
          case dbField::tpString:
            PRINTF(STRLITERAL("'%s'"), 
                   (char*)base+((dbVarying*)(base+fd->dbsOffs))->offs);
            continue;
          case dbField::tpReference:
            printf(OID_FORMAT, *(oid_t*)(base + fd->dbsOffs));
            continue;
          case dbField::tpRawBinary:
            n = (int)fd->dbsSize;
            elem = base + fd->dbsOffs;
            printf("(");
            for (i = 0; i < n; i++) {
                if (i != 0) {
                    printf(", ");
                }
                printf("%02x", *elem++);
            }
            printf(")");
            continue;
          case dbField::tpArray:
            n = ((dbVarying*)(base + fd->dbsOffs))->size;
            elem = base + ((dbVarying*)(base + fd->dbsOffs))->offs;
            printf("(");
            for (i = 0; i < n; i++) {
                if (i != 0) {
                    printf(", ");
                }
                dumpRecord(elem, fd->components);
                elem += fd->components->dbsSize;
            }
            printf(")");
            continue;
          case dbField::tpStructure:
            if (dateFormat != NULL 
                && fd->components->next == fd->components 
                && STRCMP(fd->components->name, STRLITERAL("stamp")) == 0) 
            { 
                char_t buf[64];
                PRINTF(((dbDateTime*)(base + fd->components->dbsOffs))->asString(buf, itemsof(buf), dateFormat));
                continue;
            }
            printf("(");
            dumpRecord(base, fd->components);
            printf(")");
        }
    } while ((fd = fd->next) != first);
}

int dbSubSql::calculateRecordSize(dbList* node, int offs,
                                  dbFieldDescriptor* first)
{
    dbFieldDescriptor* fd = first;
    do {
        if (node == NULL) {
            return -1;
        }
        if (fd->type == dbField::tpArray) {
            if (node->type != dbList::nTuple) {
                return -1;
            }
            int nElems = node->aggregate.nComponents;
            offs = (int)(DOALIGN(offs, fd->components->alignment)
                 + nElems*fd->components->dbsSize);
            if (fd->attr & dbFieldDescriptor::HasArrayComponents) {
                dbList* component = node->aggregate.components;
                while (--nElems >= 0) {
                    int d = calculateRecordSize(component,offs,fd->components);
                    if (d < 0) return d;
                    offs = d;
                    component = component->next;
                }
            }
        } else if (fd->type == dbField::tpString) {
            if (node->type != dbList::nString) {
                return -1;
            }
            offs += (int)((STRLEN(node->sval) + 1)*sizeof(char_t));
        } else if (fd->type == dbField::tpRectangle) {
            if (node->type != dbList::nTuple) { 
                return -1;
            }
            int nCoords = node->aggregate.nComponents;
            if (nCoords != rectangle::dim*2) {
                return -1;
            }
            dbList* component = node->aggregate.components;
            while (--nCoords >= 0) {
                if (component->type != dbList::nInteger && component->type != dbList::nReal) {
                    return -1;
                }
                component = component->next;
            }
        } else if (fd->type == dbField::tpRawBinary) {
            if (node->type != dbList::nTuple) {
                return -1;
            }
            int nElems = node->aggregate.nComponents;
            dbList* component = node->aggregate.components;
            if (size_t(nElems) > fd->dbsSize) {
                return -1;
            }
            while (--nElems >= 0) {
                if (component->type != dbList::nInteger
                    || (component->ival & ~0xFF) != 0)
                {
                    return -1;
                }
                component = component->next;
            }
#ifdef AUTOINCREMENT_SUPPORT
        } else if (node->type == dbList::nAutoinc) {        
            if (fd->type != dbField::tpInt4) {
                return -1;
            }
#endif
        } else {
            if (!((node->type == dbList::nBool && fd->type == dbField::tpBool)
                  || (node->type == dbList::nInteger
                      && (fd->type == dbField::tpInt1
                          || fd->type == dbField::tpInt2
                          || fd->type == dbField::tpInt4
                          || fd->type == dbField::tpInt8
                          || fd->type == dbField::tpReference))
                  || (node->type == dbList::nReal
                      && (fd->type == dbField::tpReal4
                          || fd->type == dbField::tpReal8))
                  || (node->type == dbList::nTuple
                      && fd->type == dbField::tpStructure)
                  || (node->type == dbList::nString && fd->type < dbField::tpString)))
            {
                return -1;
            }
            if (fd->attr & dbFieldDescriptor::HasArrayComponents) {
                int d = calculateRecordSize(node->aggregate.components,
                                            offs, fd->components);
                if (d < 0) return d;
                offs = d;
            }
        }
        node = node->next;
    } while ((fd = fd->next) != first);
    return offs;
}

bool dbSubSql::isValidOid(oid_t oid) 
{
    return oid == 0 || 
        (oid < currIndexSize 
         && (getPos(oid) & (dbFreeHandleFlag|dbPageObjectFlag)) == 0);
}

int dbSubSql::initializeRecordFields(dbList* node, byte* dst, int offs,
                                     dbFieldDescriptor* first)
{
    dbFieldDescriptor* fd = first;
    dbList* component;
    byte* elem;
    coord_t* coord;
    int len, elemOffs, elemSize;

    do {
        if (node->type == dbList::nString && fd->type != dbField::tpString) { 
            char_t* s = node->sval;
            long  ival;
            switch (fd->type) {
              case dbField::tpBool:
                *(bool*)(dst+fd->dbsOffs) = *s == '1' || *s == 't' || *s == 'T';
                break;
              case dbField::tpInt1:
                if (SSCANF(s, STRLITERAL("%ld"), &ival) != 1) { 
                    return -1;
                }
                *(int1*)(dst+fd->dbsOffs) = (int1)ival;
              case dbField::tpInt2:
                if (SSCANF(s, STRLITERAL("%ld"), &ival) != 1) { 
                    return -1;
                }
                *(int2*)(dst+fd->dbsOffs) = (int2)ival;
              case dbField::tpInt4:
                if (SSCANF(s, STRLITERAL("%ld"), &ival) != 1) { 
                    return -1;
                }
                *(int4*)(dst+fd->dbsOffs) = (int4)ival;
              case dbField::tpInt8:
                if (SSCANF(s, STRLITERAL("%ld"), &ival) != 1) { 
                    return -1;
                }
                *(db_int8*)(dst+fd->dbsOffs) = ival;
                break;
              case dbField::tpReal4:
                if (SSCANF(s, STRLITERAL("%f"), (real4*)(dst+fd->dbsOffs)) != 1) { 
                    return -1;
                }
                break;
              case dbField::tpReal8:
                if (SSCANF(s, STRLITERAL("%lf"), (real8*)(dst+fd->dbsOffs)) != 1) { 
                    return -1;
                }
                break;
            }
#ifdef AUTOINCREMENT_SUPPORT
        } else if (node->type == dbList::nAutoinc) {        
            if (fd->type == dbField::tpInt4) {
                *(int4*)(dst+fd->dbsOffs) = fd->defTable->autoincrementCount;
            } else { 
                return -1;
            }
#endif
        } else { 
            switch (fd->type) {
              case dbField::tpBool:
                *(bool*)(dst+fd->dbsOffs) = node->bval;
                break;
              case dbField::tpInt1:
                *(int1*)(dst+fd->dbsOffs) = (int1)node->ival;
                break;
              case dbField::tpInt2:
                *(int2*)(dst+fd->dbsOffs) = (int2)node->ival;
                break;
              case dbField::tpInt4:
                *(int4*)(dst+fd->dbsOffs) = (int4)node->ival;
                break;
              case dbField::tpInt8:
                *(db_int8*)(dst+fd->dbsOffs) = node->ival;
                break;
              case dbField::tpReal4:
                *(real4*)(dst+fd->dbsOffs) = (real4)node->fval;
                break;
              case dbField::tpReal8:
                *(real8*)(dst+fd->dbsOffs) = node->fval;
                break;
              case dbField::tpReference:
                if (isValidOid((oid_t)node->ival)) {               
                    *(oid_t*)(dst+fd->dbsOffs) = (oid_t)node->ival;
                } else { 
                    return -1;
                }
                break;
              case dbField::tpString:
                ((dbVarying*)(dst+fd->dbsOffs))->offs = offs;
                len = (int)STRLEN(node->sval) + 1;
                ((dbVarying*)(dst+fd->dbsOffs))->size = len;
                memcpy(dst + offs, node->sval, len*sizeof(char_t));
                offs += len*sizeof(char_t);
                break;
              case dbField::tpRawBinary:
                len = node->aggregate.nComponents;
                component = node->aggregate.components;
                elem = dst + fd->dbsOffs;
                while (--len >= 0) {
                    *elem++ = (byte)component->ival;
                    component = component->next;
                }
                break;
              case dbField::tpRectangle:
                len = node->aggregate.nComponents;
                component = node->aggregate.components;
                coord = (coord_t*)(dst + fd->dbsOffs);
                assert(len == rectangle::dim*2);                    
                while (--len >= 0) {
                    *coord++ = (component->type == dbList::nInteger) 
                        ? (coord_t)component->ival : (coord_t)component->fval;
                    component = component->next;
                }
                break;
              case dbField::tpArray:
                len = node->aggregate.nComponents;
                elem = (byte*)DOALIGN(size_t(dst) + offs, fd->components->alignment);
                offs = (int)(elem - dst);
                ((dbVarying*)(dst+fd->dbsOffs))->offs = offs;
                ((dbVarying*)(dst+fd->dbsOffs))->size = len;
                elemSize = (int)fd->components->dbsSize;
                elemOffs = len*elemSize;
                offs += elemOffs;
                component = node->aggregate.components;
                while (--len >= 0) {
                    elemOffs = initializeRecordFields(component, elem, elemOffs,
                                                      fd->components);
                    if (elemOffs < 0) { 
                        return elemOffs;
                    }
                    elemOffs -= elemSize;
                    elem += elemSize;
                    component = component->next;
                }
                offs += elemOffs;
                break;
              case dbField::tpStructure:
                offs = initializeRecordFields(node->aggregate.components,
                                              dst, offs, fd->components);
                if (offs < 0) {
                    return offs;
                }
            }
        }
        node = node->next;
    } while ((fd = fd->next) != first);

    return offs;
}


bool dbSubSql::insertRecord(dbList* list, dbTableDescriptor* desc)
{
    int size = calculateRecordSize(list, (int)desc->fixedSize, desc->columns);
    if (size < 0) {
        error("Incompatible types in insert statement");
        return false;
    }
    oid_t oid = allocateRow(desc->tableId, size, desc);
    dbPutTie tie;
    byte* dst = (byte*)putRow(tie, oid);
    if (initializeRecordFields(list, dst, (int)desc->fixedSize, desc->columns) < 0) { 
        error("Conversion  error");
        return false;
    }

    dbFieldDescriptor* fd;
    for (fd = desc->indexedFields; fd != NULL; fd = fd->nextIndexedField) {
        if ((fd->indexType & UNIQUE) != 0 && fd->type != dbField::tpRectangle) { 
            if (!dbBtree::insert(this, fd->bTree, oid, fd->dbsOffs, fd->comparator)) { 
                for (dbFieldDescriptor* fdu = desc->indexedFields; fdu != fd; fdu = fdu->nextIndexedField) {
                    if ((fdu->indexType & UNIQUE) != 0 && fdu->type != dbField::tpRectangle) { 
                        dbBtree::remove(this, fdu->bTree, oid, fdu->dbsOffs, fdu->comparator);
                    }
                }
                freeRow(desc->tableId, oid, desc);
                error("Unique constraint violation");
                return false;
            }
        }
    }

    size_t nRows = desc->nRows;
    for (fd = desc->hashedFields; fd != NULL; fd = fd->nextHashedField){
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

int dbSubSql::readValues(dbList** chain)
{
    int i, n = 0;
    int tkn;
    dbList* node;

    while (true) {
        switch (scan()) {
          case tkn_lpar:
            node = new dbList(dbList::nTuple);
            node->aggregate.components = NULL;
            i = readValues(&node->aggregate.components);
            if (i < 0) {
                return -1;
            }
            node->aggregate.nComponents = i;
            break;
          case tkn_rpar:
            return -n; // valid only in case of empty list
          case tkn_iconst:
            node = new dbList(dbList::nInteger);
            node->ival = ival;
            break;
          case tkn_true:
            node = new dbList(dbList::nBool);
            node->bval = true;
            break;
          case tkn_false:
            node = new dbList(dbList::nBool);
            node->bval = false;
            break;
          case tkn_fconst:
            node = new dbList(dbList::nReal);
            node->fval = fval;
            break;
          case tkn_sconst:
            node = new dbList(dbList::nString);
            node->sval = new char_t[STRLEN(buf)+1];
            STRCPY(node->sval, buf);
            break;
          case tkn_autoincrement:
            node = new dbList(dbList::nAutoinc);
            break;
          case tkn_error:
            return -1;
          default:
            error("Syntax error in insert list");
            return -1;
        }
        *chain = node;
        chain = &node->next;
        n += 1;
        if ((tkn = scan()) == tkn_rpar) {
            return n;
        }
        if (tkn != tkn_comma) {
            error("',' expected");
            return -1;
        }
    }
}


dbFieldDescriptor* dbSubSql::readFieldName(int terminator)
{
    int tkn;

    if (expect("table name", tkn_ident)) {
        dbTableDescriptor* desc;
        dbFieldDescriptor* fd;
        if ((desc = findTable(name)) == NULL) {
            error("No such table in database");
            return NULL;
        }
        if (expect(".", tkn_dot) && expect("field name", tkn_ident)) {
            if ((fd = desc->findSymbol(name)) == NULL) {
                error("No such field in the table");
                return NULL;
            } else if (fd->type == dbField::tpArray) {
                error("Array components can not be indexed");
                return NULL;
            }
        } else {
            return NULL;
        }
        while ((tkn = scan()) != terminator) {
            if (tkn != tkn_dot) {
                error("'.' expected");
                return NULL;
            }
            if (expect("field name", tkn_ident)) {
                if ((fd = fd->findSymbol(name)) == NULL) {
                    error("No such field in the table");
                    return NULL;
                } else if (fd->type == dbField::tpArray) {
                    error("Array components can not be indexed");
                    return NULL;
                }
            } else {
                return NULL;
            }
        }
        if (fd->type == dbField::tpStructure) {
            error("Structures can not be indexed");
            return NULL;
        }
        return fd;
    }
    return NULL;
}


inline int getListSize(dbExprNode* expr) 
{ 
    int nElems;
    if (expr->type == tpList) { 
        nElems = 0;
        if (expr->operand[0] != NULL) {
            do { 
                nElems += 1;
            } while ((expr = expr->operand[1]) != NULL);
        } 
    } else { 
        nElems = 1;
    }
    return nElems;
}

bool dbSubSql::updateFields(dbAnyCursor* cursor, dbUpdateElement* elems)
{
     char_t buf[64], *src;
     dbInheritedAttribute iattr;
     dbSynthesizedAttribute sattr;
     iattr.db = this;
     iattr.oid = cursor->currId;
     iattr.table = cursor->table;
     iattr.record = cursor->tie.get();
     iattr.paramBase = (size_t)cursor->paramBase;

     do { 
         dbExprNode* expr = elems->value;
         dbFieldDescriptor* fd = elems->field;
         execute(expr, iattr, sattr);
         byte* dst = cursor->record + fd->appOffs;

         switch (fd->type) {
           case dbField::tpArray:
           {
               int nElems = getListSize(expr);
               if (nElems != 0) 
               {
                   switch (fd->components->type) {    
                     case dbField::tpBool:
                     {
                         bool* arr = new bool[nElems];
                         switch (expr->type) { 
                           case tpList:
                           {
                               int i = 0;
                               do { 
                                   dbExprNode* elem = expr->operand[0];
                                   execute(elem, iattr, sattr);
                                   switch (elem->type) { 
                                     case tpInteger:
                                       arr[i++] = sattr.ivalue != 0;
                                       continue;
                                     case tpBoolean:
                                       arr[i++] = sattr.bvalue;
                                       continue;
                                     case tpReal:
                                       arr[i++] = sattr.fvalue != 0;
                                       continue;
                                     case tpString:
                                       arr[i++] = *sattr.base == 'T' || *sattr.base == 't' || *sattr.base == '1';
                                       continue;
                                     default:
                                       return false;
                                   }                                 
                               } while ((expr = expr->operand[1]) != NULL);
                               break;
                           } 
                           case tpInteger:
                             arr[0] = sattr.ivalue != 0;
                             break;
                           case tpBoolean:
                             arr[0] = sattr.bvalue;
                             break;
                           case tpReal:
                             arr[0] = sattr.fvalue != 0;
                             break;
                           case tpString:
                             arr[0] = *sattr.base == 'T' || *sattr.base == 't' || *sattr.base == '1';
                             break;
                           default:
                             error("Invalid array element type");
                             return false;
                         }
                         ((dbArray<bool>*)dst)->assign(arr, nElems, false);
                         elems->strValue = (char_t*)arr;
                         continue;
                     }
                     case dbField::tpInt1:
                     {
                         int1* arr = new int1[nElems];
                         switch (expr->type) { 
                           case tpList:
                           {
                               int i = 0;
                               do { 
                                   dbExprNode* elem = expr->operand[0];
                                   execute(elem, iattr, sattr);
                                   switch (elem->type) { 
                                     case tpInteger:
                                       arr[i++] = (int1)sattr.ivalue;
                                       continue;
                                     case tpBoolean:
                                       arr[i++] = sattr.bvalue ? 1 : 0;
                                       continue;
                                     case tpReal:
                                       arr[i++] = (int1)sattr.fvalue;
                                       continue;
                                     case tpString:
                                       arr[i++] = (int1)atoi((char*)sattr.base);
                                       continue;
                                     default:
                                       return false;
                                   }                                 
                               } while ((expr = expr->operand[1]) != NULL);
                               break;
                           } 
                           case tpInteger:
                             arr[0] = (int1)sattr.ivalue;
                             break;
                           case tpBoolean:
                             arr[0] = sattr.bvalue ? 1 : 0;
                             break;
                           case tpReal:
                             arr[0] = (int1)sattr.fvalue;
                             break;
                           case tpString:
                             arr[0] = (int1)atoi((char*)sattr.base);
                             break;
                           default:
                             error("Invalid array element type");
                             return false;
                         }
                         ((dbArray<int1>*)dst)->assign(arr, nElems, false);
                         elems->strValue = (char_t*)arr;
                         continue;
                     }
                     case dbField::tpInt2:
                     {
                         int2* arr = new int2[nElems];
                         switch (expr->type) { 
                           case tpList:
                           {
                               int i = 0;
                               do { 
                                   dbExprNode* elem = expr->operand[0];
                                   execute(elem, iattr, sattr);
                                   switch (elem->type) { 
                                     case tpInteger:
                                       arr[i++] = (int2)sattr.ivalue;
                                       continue;
                                     case tpBoolean:
                                       arr[i++] = sattr.bvalue ? 1 : 0;
                                       continue;
                                     case tpReal:
                                       arr[i++] = (int2)sattr.fvalue;
                                       continue;
                                     case tpString:
                                       arr[i++] = (int2)atoi((char*)sattr.base);
                                       continue;
                                     default:
                                       return false;
                                   }                                 
                               } while ((expr = expr->operand[1]) != NULL);
                               break;
                           } 
                           case tpInteger:
                             arr[0] = (int2)sattr.ivalue;
                             break;
                           case tpBoolean:
                             arr[0] = sattr.bvalue ? 1 : 0;
                             break;
                           case tpReal:
                             arr[0] = (int2)sattr.fvalue;
                             break;
                           case tpString:
                             arr[0] = (int2)atoi((char*)sattr.base);
                             break;
                           default:
                             error("Invalid array element type");
                             return false;
                         }
                         ((dbArray<int2>*)dst)->assign(arr, nElems, false);
                         elems->strValue = (char_t*)arr;
                         continue;
                     }
                     case dbField::tpInt4:
                     {
                         int4* arr = new int4[nElems];
                         switch (expr->type) { 
                           case tpList:
                           {
                               int i = 0;
                               do { 
                                   dbExprNode* elem = expr->operand[0];
                                   execute(elem, iattr, sattr);
                                   switch (elem->type) { 
                                     case tpInteger:
                                       arr[i++] = (int4)sattr.ivalue;
                                       continue;
                                     case tpBoolean:
                                       arr[i++] = sattr.bvalue ? 1 : 0;
                                       continue;
                                     case tpReal:
                                       arr[i++] = (int4)sattr.fvalue;
                                       continue;
                                     case tpString:
                                       arr[i++] = (int4)atoi((char*)sattr.base);
                                       continue;
                                     default:
                                       return false;
                                   }                                 
                               } while ((expr = expr->operand[1]) != NULL);
                               break;
                           } 
                           case tpInteger:
                             arr[0] = (int4)sattr.ivalue;
                             break;
                           case tpBoolean:
                             arr[0] = sattr.bvalue ? 1 : 0;
                             break;
                           case tpReal:
                             arr[0] = (int4)sattr.fvalue;
                             break;
                           case tpString:
                             arr[0] = (int4)atoi((char*)sattr.base);
                             break;
                           default:
                             error("Invalid array element type");
                             return false;
                         }
                         ((dbArray<int4>*)dst)->assign(arr, nElems, false);
                         elems->strValue = (char_t*)arr;
                         continue;
                     }
                     case dbField::tpInt8:
                     {
                         db_int8* arr = new db_int8[nElems];
                         switch (expr->type) { 
                           case tpList:
                           {
                               int i = 0;
                               do { 
                                   dbExprNode* elem = expr->operand[0];
                                   execute(elem, iattr, sattr);
                                   switch (elem->type) { 
                                     case tpInteger:
                                       arr[i++] = sattr.ivalue;
                                       continue;
                                     case tpBoolean:
                                       arr[i++] = sattr.bvalue ? 1 : 0;
                                       continue;
                                     case tpReal:
                                       arr[i++] = (db_int8)sattr.fvalue;
                                       continue;
                                     case tpString:
                                       arr[i++] = atol((char*)sattr.base);
                                       continue;
                                     default:
                                       return false;
                                   }                                 
                               } while ((expr = expr->operand[1]) != NULL);
                               break;
                           } 
                           case tpInteger:
                             arr[0] = sattr.ivalue;
                             break;
                           case tpBoolean:
                             arr[0] = sattr.bvalue ? 1 : 0;
                             break;
                           case tpReal:
                             arr[0] = (db_int8)sattr.fvalue;
                             break;
                           case tpString:
                             arr[0] = atol((char*)sattr.base);
                             break;
                           default:
                             error("Invalid array element type");
                             return false;
                         }
                         ((dbArray<db_int8>*)dst)->assign(arr, nElems, false);
                         elems->strValue = (char_t*)arr;
                         continue;
                     }
                     case dbField::tpReal4:
                     {
                         real4* arr = new real4[nElems];
                         switch (expr->type) { 
                           case tpList:
                           {
                               int i = 0;
                               do { 
                                   dbExprNode* elem = expr->operand[0];
                                   execute(elem, iattr, sattr);
                                   switch (elem->type) { 
                                     case tpInteger:
                                       arr[i++] = (real4)sattr.ivalue;
                                       continue;
                                     case tpBoolean:
                                       arr[i++] = (real4)(sattr.bvalue ? 1.0 : 0.0);
                                       continue;
                                     case tpReal:
                                       arr[i++] = (real4)sattr.fvalue;
                                       continue;
                                     case tpString:
                                       arr[i++] = (real4)atof((char*)sattr.base);
                                       continue;
                                     default:
                                       return false;
                                   }                                 
                               } while ((expr = expr->operand[1]) != NULL);
                               break;
                           } 
                           case tpInteger:
                             arr[0] = (real4)sattr.ivalue;
                             break;
                           case tpBoolean:
                             arr[0] = (real4)(sattr.bvalue ? 1.0 : 0.0);
                             break;
                           case tpReal:
                             arr[0] = (real4)sattr.fvalue;
                             break;
                           case tpString:
                             arr[0] = (real4)atof((char*)sattr.base);
                             break;
                           default:
                             error("Invalid array element type");
                             return false;
                         }
                         ((dbArray<real4>*)dst)->assign(arr, nElems, false);
                         elems->strValue = (char_t*)arr;
                         continue;
                     }
                     case dbField::tpReal8:
                     {
                         real8* arr = new real8[nElems];
                         switch (expr->type) { 
                           case tpList:
                           {
                               int i = 0;
                               do { 
                                   dbExprNode* elem = expr->operand[0];
                                   execute(elem, iattr, sattr);
                                   switch (elem->type) { 
                                     case tpInteger:
                                       arr[i++] = (real8)sattr.ivalue;
                                       continue;
                                     case tpBoolean:
                                       arr[i++] = sattr.bvalue ? 1.0 : 0.0;
                                       continue;
                                     case tpReal:
                                       arr[i++] = sattr.fvalue;
                                       continue;
                                     case tpString:
                                       arr[i++] = atoi((char*)sattr.base);
                                       continue;
                                     default:
                                       return false;
                                   }                                 
                               } while ((expr = expr->operand[1]) != NULL);
                               break;
                           } 
                           case tpInteger:
                             arr[0] = (real8)sattr.ivalue;
                             break;
                           case tpBoolean:
                             arr[0] = sattr.bvalue ? 1.0 : 0.0;
                             break;
                           case tpReal:
                             arr[0] = sattr.fvalue;
                             break;
                           case tpString:
                             arr[0] = atof((char*)sattr.base);
                             break;
                           default:
                             error("Invalid array element type");
                             return false;
                         }
                         ((dbArray<real8>*)dst)->assign(arr, nElems, false);
                         elems->strValue = (char_t*)arr;
                         continue;
                     }
                     case dbField::tpReference:
                     {
                         oid_t* arr = new oid_t[nElems];
                         switch (expr->type) { 
                           case tpList:
                           {
                               int i = 0;
                               do { 
                                   dbExprNode* elem = expr->operand[0];
                                   execute(elem, iattr, sattr);
                                   switch (elem->type) { 
                                     case tpInteger:
                                       arr[i++] = (oid_t)sattr.ivalue;
                                       continue;
                                     case tpReference:
                                       arr[i++] = sattr.oid;
                                       continue;
                                     case tpString:
                                       arr[i++] = (oid_t)atol((char*)sattr.base);
                                       continue;
                                     default:
                                       return false;
                                   }                                 
                               } while ((expr = expr->operand[1]) != NULL);
                               break;
                           } 
                           case tpInteger:
                             arr[0] = (oid_t)sattr.ivalue;
                             break;
                           case tpReference:
                             arr[0] = sattr.oid;
                             break;
                           case tpString:
                             arr[0] = (oid_t)atol((char*)sattr.base);
                             break;
                           default:
                             error("Invalid array element type");
                             return false;
                         }
                         ((dbArray<oid_t>*)dst)->assign(arr, nElems, false);
                         elems->strValue = (char_t*)arr;
                         continue;
                     }
                     default:
                       error("Usupported array type");
                       return false;
                   }
               } else {
                   dbAnyArray::arrayAllocator((dbAnyArray*)dst, NULL, 0);                   
               }
               continue;
           }
           case dbField::tpRectangle:
             if (expr->type != tpList) { 
                 error("Array coordinates expected");
                 return false;
             } else { 
                 rectangle& r = *(rectangle*)dst;
                 for (int i = 0; i < RECTANGLE_DIMENSION*2; i++) { 
                     if (expr == NULL || expr->operand[0] == NULL) { 
                         error("Bad rectangle constant");
                         return false;
                     }
                     dbExprNode* elem = expr->operand[0];
                     dbExprNode* tail = expr->operand[1];
                     if (elem->type == tpReal) {                         
                         r.boundary[i] = (coord_t)elem->fvalue;
                     } else if (elem->type == tpInteger) {  
                         r.boundary[i] = (coord_t)elem->ivalue;
                     } else { 
                         error("Bad rectangle constant");
                         return false;
                     }
                     expr = tail;
                 }
                 continue;
             }
           case dbField::tpBool:
             switch (expr->type) { 
               case tpInteger:
                 *(bool*)dst = sattr.ivalue != 0;
                 continue;
               case tpBoolean:
                 *(bool*)dst = sattr.bvalue;
                 continue;
               case tpReal:
                 *(bool*)dst = sattr.fvalue != 0;
                 continue;
               case tpString:
                 *(bool*)dst = *sattr.base == 'T' || *sattr.base == 't' || *sattr.base == '1';
                 continue;
             }
             break;
           case dbField::tpInt1:
             switch (expr->type) { 
               case tpInteger:
                 *(int1*)dst = (int1)sattr.ivalue;
                 continue;
               case tpBoolean:
                 *(int1*)dst = sattr.bvalue ? 1 : 0;
                 continue;
               case tpReal:
                 *(int1*)dst = (int1)sattr.fvalue;
                 continue;
               case tpString:
                 *(int1*)dst = (int1)atoi((char*)sattr.base);
                 continue;
             }
             break;
           case dbField::tpInt2:
             switch (expr->type) { 
               case tpInteger:
                 *(int2*)dst = (int2)sattr.ivalue;
                 continue;
               case tpBoolean:
                 *(int2*)dst = sattr.bvalue ? 1 : 0;
                 continue;
               case tpReal:
                 *(int2*)dst = (int2)sattr.fvalue;
                 continue;
               case tpString:
                 *(int2*)dst = (int2)atoi((char*)sattr.base);
                 continue;
             }
             break;
           case dbField::tpInt4:
             switch (expr->type) { 
               case tpInteger:
                 *(int4*)dst = (int4)sattr.ivalue;
                 continue;
               case tpBoolean:
                 *(int4*)dst = sattr.bvalue ? 1 : 0;
                 continue;
               case tpReal:
                 *(int4*)dst = (int4)sattr.fvalue;
                 continue;
               case tpString:
                 *(int4*)dst = (int1)atoi((char*)sattr.base);
                 continue;
             }
             break;
           case dbField::tpInt8:
             switch (expr->type) { 
               case tpInteger:
                 *(db_int8*)dst = sattr.ivalue;
                 continue;
               case tpBoolean:
                 *(db_int8*)dst = sattr.bvalue ? 1 : 0;
                 continue;
               case tpReal:
                 *(db_int8*)dst = (db_int8)sattr.fvalue;
                 continue;
               case tpString:
                 *(db_int8*)dst = (db_int8)atoi((char*)sattr.base);
                 continue;
             }
             break;
           case dbField::tpReal4:
             switch (expr->type) { 
               case tpInteger:
                 *(real4*)dst = (real4)sattr.ivalue;
                 continue;
               case tpBoolean:
                 *(real4*)dst = (real4)(sattr.bvalue ? 1.0 : 0.0);
                 continue;
               case tpReal:
                 *(real4*)dst = (real4)sattr.fvalue;
                 continue;
               case tpString:
                 *(real4*)dst = (real4)atof((char*)sattr.base);
                 continue;
             }
             break;
           case dbField::tpReal8:
             switch (expr->type) { 
               case tpInteger:
                 *(real8*)dst = (real8)sattr.ivalue;
                 continue;
               case tpBoolean:
                 *(real8*)dst = sattr.bvalue ? 1.0 : 0.0;
                 continue;
               case tpReal:
                 *(real8*)dst = sattr.fvalue;
                 continue;
               case tpString:
                 *(real8*)dst = atof((char*)sattr.base);
                 continue;
             }
             break;
           case dbField::tpString:
             src = buf;
             switch (expr->type) { 
               case tpInteger:
                 SPRINTF(SPRINTF_BUFFER(buf), T_INT8_FORMAT, sattr.ivalue);
                 break;
               case tpBoolean:
                 STRCPY(buf, sattr.bvalue ? STRLITERAL("t") : STRLITERAL("f"));
                 break;
               case tpReal:
                 SPRINTF(SPRINTF_BUFFER(buf), STRLITERAL("%f"), sattr.fvalue);
                 break;
               case tpString:
                 src = (char_t*)sattr.base;
                 break;
             }
             *(char_t**)dst = new char_t[STRLEN(src)+1];
             STRCPY(*(char_t**)dst, src);
             elems->strValue = *(char_t**)dst;
             continue;
           case dbField::tpReference:
             if (expr->type == tpReference) { 
                 *(oid_t*)dst = sattr.oid;
                 continue;
             } else if (expr->type == tpInteger) { 
                 *(oid_t*)dst = (oid_t)sattr.ivalue;
                 continue;
             }
         }
         error("Mismatched type of update expression");
         return false;
     } while ((elems = elems->next) != NULL);

     return true;
}

void dbSubSql::deleteColumns(dbFieldDescriptor* columns)
{
    if (columns != NULL) { 
        dbFieldDescriptor *next, *fd = columns;
        do {
            next = fd->next;
            fd->type = dbField::tpUnknown;
            fd->longName = NULL;
            delete fd;
            fd = next;
        } while (next != columns);
    }                    
}

void dbSubSql::profile()
{
    printf("TABLES:\n");
    printf("   Fixed   Fields  Columns     Rows    Total  Aligned  TableName\n");
    printf("----------------------------------------------------------------\n");
    beginTransaction(dbSharedLock);
    for (dbTableDescriptor* desc=tables; desc != NULL; desc=desc->nextDbTable)
    { 
        refreshTable(desc);
        size_t totalSize = 0;
        size_t totalAlignedSize = 0;
        oid_t oid = desc->firstRow; 
        while (oid != 0) {
            dbRecord rec;
            getHeader(rec, oid);
            totalSize += rec.size;
            totalAlignedSize += DOALIGN(rec.size, dbAllocationQuantum);
            oid = rec.next;
        }            
        printf("%8ld %8ld %8ld %8ld %8ld %8ld  %s\n",
               (long)desc->fixedSize,(long)desc->nFields, (long)desc->nColumns, 
               (long)desc->nRows,  (long)totalSize, (long)totalAlignedSize,
               desc->name);
    }
}        

bool dbSubSql::parse()
{
    dbTableDescriptor* desc;
    dbFieldDescriptor* fd;
    int tkn;
    bool outputOid, compactify, count;
    dbFieldDescriptor* columns = NULL;

    line = 1;
    pos = 0;

    while (true) {
        if (interactiveMode) {
            printf(prompt);
            tkn = scan();
            pos += (int)strlen(prompt);
        } else {
            tkn = scan();
        }

        switch (tkn) {
          case tkn_update:
            if (!opened) { 
                error("Database not opened");
                continue;
            }
            if (accessType == dbReadOnly || accessType == dbMulticlientReadOnly) { 
                error("Operation is not possible in read-only mode");
                continue;
            }
            dotIsPartOfIdentifier = true;
            if (expect("table name", tkn_ident)) {
                if ((desc = findTable(name)) == NULL) { 
                    error("No such table in database");
                    continue;
                }
                if (!expect("set", tkn_set)) { 
                    continue;
                }

                dbDatabaseThreadContext* ctx = threadContext.get();
                byte *record = dbMalloc(desc->appSize);
                memset(record, 0, desc->appSize);
                ctx->interactive = true;
                ctx->catched = true;
                dbUpdateElement* elems = NULL;
                if (!expect("field name", tkn_ident)) { 
                    goto updateCleanup;
                }
        
#ifdef THROW_EXCEPTION_ON_ERROR
                try {
#else
                if (setjmp(ctx->unwind) == 0) {
#endif
                    
                    char_t* condition = NULL;
                    int startPos = pos;
                    while (true) { 
                        dbUpdateElement* elem = new dbUpdateElement;
                        dbFieldDescriptor* fd = desc->find(name);
                        if (fd == NULL) { 
                            error("No such field in the table");
                            goto updateCleanup;
                        }
                        if (fd->type > dbField::tpArray && fd->type != dbField::tpRectangle) { 
                            error("Field can not be updated");
                            goto updateCleanup;
                        }
                        elem->field = fd;
                        elem->next = elems;
                        elems = elem;
                        if (!expect("=", tkn_eq)) { 
                            goto updateCleanup;
                        }
                        startPos = pos;
                        int ch = readExpression();
                        if (ch == EOF) { 
                            error("unexpected end of input");
                            goto updateCleanup;
                        } 
                        condition = stristr(buf, _T("where"));
                        if (condition != NULL) {
                            *condition = '\0';
                        }
                        dbExprNode* expr = ctx->compiler.compileExpression(desc, buf, startPos);
                        if (expr == NULL) { 
                            goto updateCleanup;
                        }
                       if (expr->type > tpReference && expr->type != tpList) {
                            error("Invalid expression type");
                            goto updateCleanup;
                        }
                        elem->value = expr;
                        if (condition == NULL && ch == ',') { 
                            if (!expect("field name", tkn_ident)) { 
                                goto updateCleanup;
                            }
                        } else { 
                            break;
                        }
                    }
                    dbAnyCursor cursor(*desc, dbCursorForUpdate, record);
                    cursor.reset();
                        
                    if (condition != NULL) { 
                        query.pos = startPos + (int)(condition - buf) + 5;
                        query = condition + 5;
                        select(&cursor, query);
                        if (query.compileError()) { 
                            goto updateCleanup;
                        }                                   
                    } else { 
                        select(&cursor);
                    }
                    if (cursor.gotoFirst()) { 
                        do { 
                            cursor.fetch();
                            if (!updateFields(&cursor, elems)) { 
                                goto updateCleanup;
                            }   
                            if (!cursor.update()) { 
                                error("Unique constrain violation");                                
                                goto updateCleanup;
                            }                                   
                        } while (cursor.gotoNext());
                    }
                    printf("\n\t%d records updated\n", cursor.getNumberOfRecords());
#ifdef THROW_EXCEPTION_ON_ERROR
                } catch(dbException const&) {}
#else
                } else { 
                    if (query.mutexLocked) { 
                        query.mutexLocked = false;
                        query.mutex.unlock();
                    }
                }
#endif
              updateCleanup:
                query.reset();
                while (elems != NULL) { 
                    dbUpdateElement* elem = elems;
                    elems = elems->next;
                    delete elem;
                }
                dbExprNodeAllocator::instance.reset();
                ctx->catched = false;
                dbFree(record);
            }
            break;

          case tkn_select:
            if (!opened) {
                error("Database not opened");
                continue;
            }
            outputOid = true;
            count = false;
            if ((tkn = scan()) == tkn_all) {
                outputOid = false;
                tkn = scan();
            } else if (tkn == tkn_count) { 
                if (!expect("'('", tkn_lpar)
                    || !expect("'*'", tkn_all)
                    || !expect("')'", tkn_rpar))
                {
                    continue;
                }
                count = true;
                tkn = scan();
            }
            columns = NULL;
            if (tkn != tkn_from) {
                while (true) { 
                    if (tkn != tkn_ident) { 
                        error("Field name or 'from' expected");
                    }
                    dbFieldDescriptor* column = new dbFieldDescriptor(name);
                    if (columns != NULL) { 
                        column->next = columns;
                        column->prev = columns->prev;
                        column->prev->next = column;
                        columns->prev = column;
                    } else { 
                        columns = column;
                        column->prev = column->next = column;
                    }
                    tkn = scan();
                    if (tkn != tkn_comma) { 
                        break;
                    }
                    tkn = scan();
                }
            }
            if (tkn != tkn_from) {
                deleteColumns(columns);
                error("FROM expected");
                continue;
            }
            dotIsPartOfIdentifier = true;
            if (scan() != tkn_ident) {
                deleteColumns(columns);
                error("Table name expected");
                continue;
            }
            if ((desc = findTable(name)) != NULL) {
                dbAnyCursor cursor(*desc, dbCursorViewOnly, NULL);
                query.pos = pos;
                dbDatabaseThreadContext* ctx = threadContext.get();
                ctx->interactive = true;
                ctx->catched = true;
#ifdef THROW_EXCEPTION_ON_ERROR
                try {
#else
                if (setjmp(ctx->unwind) == 0) {
#endif
                    if (readCondition()) {
                        query = buf;
                        cursor.reset();
                        select(&cursor, query);
                        if (query.compileError()) {
                            deleteColumns(columns);
                            dbExprNodeAllocator::instance.reset();
                            ctx->catched = false;
                            break;
                        }
                    } else {
                        ctx->catched = false;
                        deleteColumns(columns);
                        break;
                    }
                    if (count) { 
                        printf("%d records selected\n",
                               cursor.getNumberOfRecords());
                    } else { 
                        if (cursor.gotoFirst()) {
                            dbGetTie tie;
                            dbFieldDescriptor* columnList;
                            if (columns != NULL) { 
                                columnList = columns;
                                dbFieldDescriptor* cc = columns; 
                                do { 
                                    dbFieldDescriptor* next = cc->next;
                                    dbFieldDescriptor* fd = desc->columns;
                                    do { 
                                        if (cc->name == fd->name) { 
                                            *cc = *fd;
                                            cc->next = next;
                                            goto Found;
                                        }
                                    } while ((fd = fd->next) != desc->columns);                                
#ifdef UNICODE
                                    char columnName[128];
                                    char buf[256];
                                    wcstombs(columnName, cc->name, sizeof columnName);
                                    sprintf(buf, "Column '%s' is not found\n", columnName);
                                    error(buf);
#else 
                                    char buf[256];
                                    sprintf(buf, "Column '%s' is not found\n", cc->name);
                                    error(buf);
#endif
                                  Found:
                                    PRINTF(STRLITERAL("%s "), cc->name);
                                    cc = next;
                                } while (cc != columns);
                            } else {                  
                                columnList = desc->columns;
                                dbFieldDescriptor* fd = columnList;
                                do {
                                    PRINTF(STRLITERAL("%s "), fd->name);
                                } while ((fd = fd->next) != columnList);
                            }
                            if (outputOid) {
                                printf("\n" OID_FORMAT ": (", cursor.currId);
                            } else {
                                printf("\n(");
                            }
                            dumpRecord((byte*)getRow(tie, cursor.currId), columnList);
                            printf(")");
                            while (cursor.gotoNext()) {
                                if (outputOid) {
                                    printf(",\n" OID_FORMAT ": (", cursor.currId);
                                } else {
                                    printf(",\n(");
                                }
                                dumpRecord((byte*)getRow(tie, cursor.currId), columnList);
                                printf(")");
                            }
                            printf("\n\t%d records selected\n",
                                   cursor.getNumberOfRecords());
                        } else {
                            fprintf(stderr, "No records selected\n");
                        }
                    }
#ifdef THROW_EXCEPTION_ON_ERROR
                } catch(dbException const&) {}
#else
                } else { 
                    if (query.mutexLocked) { 
                        query.mutexLocked = false;
                        query.mutex.unlock();
                    }
                }
#endif
                deleteColumns(columns); 
                ctx->catched = false;
            } else {
                error("No such table in database");
            }
            break;

          case tkn_open:
            if (expect("database file name", tkn_sconst)) {
                if (opened) {
                    delete[] databasePath;
                    close();
                    while (droppedTables != NULL) {
                        dbTableDescriptor* next = droppedTables->nextDbTable;
                        delete droppedTables;
                        droppedTables = next;
                    }
                    opened = false;
                }
                time_t transactionCommitDelay = 0;
                char* delay = getenv("GIGABASE_COMMIT_DELAY");
                if (delay != NULL) { 
                    transactionCommitDelay = atoi(delay);
                }
                if (!open(buf, transactionCommitDelay)) {
                    fprintf(stderr, "Database not opened\n");
                } else {
                    databasePath = new char_t[STRLEN(buf) + 1];
                    STRCPY(databasePath, buf);
                    opened = true;
                    metatable = loadMetaTable();
                    existedTables = tables;
                    char* backupName = getenv("GIGABASE_BACKUP_NAME");
                    if (backupName != NULL) { 
                        char* backupPeriod = getenv("GIGABASE_BACKUP_PERIOD");
                        time_t period = 60*60*24; // one day
                        if (backupPeriod != NULL) { 
                            period = atoi(backupPeriod);
                        }
#ifdef UNICODE
                        char_t backupFilePath[1024];
                        mbstowcs(backupFilePath, backupName, (sizeof backupFilePath)/sizeof(wchar_t));
#else 
                        char* backupFilePath = backupName;
#endif
                        PRINTF(_T("Schedule backup to file %s each %u seconds\n"), 
                               backupFilePath, (unsigned)period);   
                        scheduleBackup(backupFilePath, period);                     
                    }
                }
            }
            break;

          case tkn_drop:
            if (!opened) {
                error("Database not opened");
                continue;
            }
            if (accessType == dbReadOnly || accessType == dbMulticlientReadOnly) { 
                error("Operation is not possible in read-only mode");
                continue;
            }
            switch (scan()) {
              case tkn_table:
                dotIsPartOfIdentifier = true;
                if (expect("table name", tkn_ident)) {
                    desc = findTable(name);
                    if (desc == NULL) {
                        error("No such table in database");
                    } else {
                        dropTable(desc);
                        if (desc == existedTables) { 
                            existedTables = desc->nextDbTable;
                        }
                        unlinkTable(desc);
                        desc->nextDbTable = droppedTables;
                        droppedTables = desc;
                    }
                }
                break;
              case tkn_hash:
              case tkn_index:
                fd = readFieldName();
                if (fd != NULL) {
                    if (fd->bTree == 0) {
                        error("There is no index for this field");
                    } else {
                        dropIndex(fd);
                    }
                }
                break;
              default:
                error("Expecting 'table', 'hash' or 'index' keyword");
                continue;
            }
            break;

          case tkn_rename:
            if (!opened) {
                error("Database not opened");
                continue;
            }
            if (accessType == dbReadOnly || accessType == dbMulticlientReadOnly) { 
                error("Operation is not possible in read-only mode");
                continue;
            }
            fd = readFieldName(tkn_to);
            if (fd != NULL) { 
                if (expect("new field name", tkn_ident) && expect(";", tkn_semi)) { 
                    beginTransaction(dbExclusiveLock);
                    fd->name = name;
                    delete[] fd->longName;
                    fd->longName = new char_t[STRLEN(name)+1];
                    STRCPY(fd->longName, name);
                    size_t newSize = sizeof(dbTable) + desc->nFields*sizeof(dbField)
                        + fd->defTable->totalNamesLength()*sizeof(char_t);
                    dbPutTie tie;
                    dbTable* table = (dbTable*)putRow(tie, fd->defTable->tableId, newSize);
                    fd->defTable->storeInDatabase(table);
                }
            }
            break;

          case tkn_backup:
            if (!opened) {
                error("Database not opened");
                continue;
            }
            compactify = false;
            if ((tkn = scan()) == tkn_compactify) {
                compactify = true;
                tkn = scan();
            }
            if (tkn != tkn_sconst) { 
                 error("Backup file name expected");
            } else { 
                if (!backup(buf, compactify)) {
                    printf("Backup failed\n");
                } else {
                    while (droppedTables != NULL) {
                        dbTableDescriptor* next = droppedTables->nextDbTable;
                        delete droppedTables;
                        droppedTables = next;
                    }
                    commit();
                    existedTables = tables;
                }
            }
            continue;

          case tkn_restore:
            if (opened) {
                error("Can not restore online database");
                continue;
            }
            if (accessType == dbReadOnly || accessType == dbMulticlientReadOnly) { 
                error("Operation is not possible in read-only mode");
                continue;
            }
            if (expect("backup file name", tkn_sconst)) {
                char_t bckName[initBufSize];
                STRCPY(bckName, buf);
                if (expect("database file name", tkn_sconst)) {
                    if (!restore(bckName, buf)) {
                        printf("Restore failed\n");
                    }
                }
            }
            break;

          case tkn_alter:
            if (!opened) {
                error("Database not opened");
                continue;
            }
            if (accessType == dbReadOnly || accessType == dbMulticlientReadOnly) { 
                error("Operation is not possible in read-only mode");
                continue;
            }
            switch (scan()) {
              case tkn_table:
                updateTable(false);
                break;
              default:
                error("Expecting 'table' keyword");
                continue;
            }
            break;


          case tkn_create:
            if (!opened) {
                error("Database not opened");
                continue;
            }
            if (accessType == dbReadOnly || accessType == dbMulticlientReadOnly) { 
                error("Operation is not possible in read-only mode");
                continue;
            }
            switch (scan()) {
              case tkn_hash:
              case tkn_index:
                if (!expect("on", tkn_on)) {
                    continue;
                }
                fd = readFieldName();
                if (fd != NULL) {
                    if (fd->bTree != 0) {
                        error("Index already exists");
                    } else {
                        createIndex(fd);
                    }
                }
                break;

              case tkn_table:
                updateTable(true);
                break;

              default:
                error("Expecting 'table', 'hash' or 'index' keyword");
                continue;
            }
            break;

          case tkn_insert:
            if (!opened) {
                error("Database not opened");
                continue;
            }
            if (accessType == dbReadOnly || accessType == dbMulticlientReadOnly) { 
                error("Operation is not possible in read-only mode");
                continue;
            }
            if (expect("into", tkn_into)) { 
                dotIsPartOfIdentifier = true;
                if (expect("table name", tkn_ident)) {
                    if ((desc = findTable(name)) == NULL) {
                        error("No such table in database");
                        continue;
                    }
                    if (!expect("values", tkn_values)) {
                        continue;
                    }
                    beginTransaction(dbExclusiveLock);
                    modified = true;
                    while (expect("(", tkn_lpar)) {
                        dbList* list = NULL;
                        int n = readValues(&list);
                        if (n <= 0 || !insertRecord(list, desc)) {
                            if (n == 0) {
                                error("Empty fields list");
                            }
                            tkn = tkn_semi; // just avoid extra error messages
                        } else {
                            tkn = scan();
                        }
                        while (list != NULL) {
                            dbList* tail = list->next;
                            delete list;
                            list = tail;
                        }
                        if (tkn == tkn_semi) {
                            break;
                        } else if (tkn != tkn_comma) {
                            error("';' or ',' expected");
                        }
                    }
                }
            }
            break;

          case tkn_delete:
            if (!opened) {
                error("Database not opened");
                continue;
            }
            if (accessType == dbReadOnly || accessType == dbMulticlientReadOnly) { 
                error("Operation is not possible in read-only mode");
                continue;
            }
            if (expect("from", tkn_from)) { 
                dotIsPartOfIdentifier = true;
                if (expect("table name", tkn_ident)) {
                    if ((desc = findTable(name)) == NULL) {
                        error("No such table in database");
                        continue;
                    } 
                    dbAnyCursor cursor(*desc, dbCursorForUpdate, NULL);
                    dbDatabaseThreadContext* ctx = threadContext.get();
                    ctx->interactive = true;
                    ctx->catched = true;
                    
#ifdef THROW_EXCEPTION_ON_ERROR
                    try {
#else
                        if (setjmp(ctx->unwind) == 0) {
#endif
                            if (readCondition()) {
                                query = buf;
                                cursor.reset();
                                select(&cursor, query);
                                if (query.compileError()) {
                                    dbExprNodeAllocator::instance.reset();
                                    ctx->catched = false;
                                    break;
                                }
                            } else {
                                ctx->catched = false;
                                break;
                            }
                            int n_deleted = cursor.getNumberOfRecords();
                            cursor.removeAllSelected();
                            printf("\n\t%d records deleted\n", n_deleted);
#ifdef THROW_EXCEPTION_ON_ERROR
                        } catch(dbException const&) {}
#else
                    } else { 
                        if (query.mutexLocked) { 
                            query.mutexLocked = false;
                            query.mutex.unlock();
                        }
                    }
#endif
                    ctx->catched = false;
                }
            }
            break;

          case tkn_commit:
            if (!opened) {
                error("Database not opened");
            } else {
                while (droppedTables != NULL) {
                    dbTableDescriptor* next = droppedTables->nextDbTable;
                    delete droppedTables;
                    droppedTables = next;
                }
                commit();
                existedTables = tables;
            }
            continue;

          case tkn_rollback:
            if (!opened) {
                error("Database not opened");
            } else {
                while (droppedTables != NULL) {
                    dbTableDescriptor* next = droppedTables->nextDbTable;
                    linkTable(droppedTables, droppedTables->tableId);
                    droppedTables = next;
                }
                rollback();
                while (tables != existedTables) { 
                    dbTableDescriptor* table = tables;
                    unlinkTable(table);
                    delete table;
                }
            }
            continue;

          case tkn_memory:
            if (!opened) { 
                error("Database not opened");
            } else { 
                dbMemoryStatistic stat;
                beginTransaction(dbSharedLock);
                getMemoryStatistic(stat);
                printf("Used memory: %ld\nFree memory: %ld\nNumber of holes: %ld\nMaximal hole size: %ld\nMinimal hole size: %ld\nAverage hole size: %ld\n\n", 
                       (long)stat.used,
                       (long)stat.free,
                       (long)stat.nHoles,
                       (long)stat.maxHoleSize,
                       (long)stat.minHoleSize,
                       (long)(stat.nHoles != 0 ? (stat.free / stat.nHoles) : 0));
                for (int i = 0; i < dbDatabaseOffsetBits; i++) { 
                    if (stat.nHolesOfSize[i] != 0) { 
                        printf("Number of holes of size [%ld...%ld): %ld\n", 1L << i, 1L << (i+1), (long)stat.nHolesOfSize[i]);
                    }
                }
            }
            break;

          case tkn_profile:
            if (!opened) { 
                error("Database not opened");
            } else { 
                profile();
            }
            break;

          case tkn_show:
            if (!opened) {
                error("Database not opened");
                continue;
            } else {
                beginTransaction(dbSharedLock);
                printf("GigaBASE version  :  %d.%02d\n"
                       "Database version  :  %d.%02d\n"
                       "Database file size: " INT8_FORMAT " Kb\n"
                       "Used database size: " INT8_FORMAT " Kb\n"
                       "Object index size : " INT8_FORMAT " handles\n"
                       "Used part of index: " INT8_FORMAT " handles\n",
                       GIGABASE_MAJOR_VERSION, GIGABASE_MINOR_VERSION, 
                       header->versionMajor, header->versionMinor,
                       db_int8(header->root[1-curr].size / 1024),
                       db_int8(used() / 1024), 
                       db_int8(header->root[1-curr].indexSize),
                       db_int8(header->root[1-curr].indexUsed)
                       );
                printf("\nTABLES:\n");
                printf("OID       FixedSize   Fields  Columns Rows     TableName\n");
                printf("---------------------------------------------------------\n");
                for (dbTableDescriptor* desc=tables; desc != NULL; desc=desc->nextDbTable)
                { 
                    PRINTF(STRLITERAL("0x%06x  %8ld %8ld %8ld %8ld  %s\n"),
                           (int)desc->tableId, (long)desc->fixedSize,
                           (long)desc->nFields, (long)desc->nColumns, (long)desc->nRows, desc->name);
                }
            }
            break;

          case tkn_describe:
            if (!opened) {
                error("Database not opened");
                continue;
            }   
            dotIsPartOfIdentifier = true;
            if (expect("table name", tkn_ident)) {
                if ((desc = findTable(name)) == NULL) {
                    error("No such table in database");
                    continue;
                }
                PRINTF(_T("\nOID=0x%06x, TableName=%s\n"), (int)desc->tableId, desc->name);
                printf("No Index FieldType        RefTableName     FieldName        InverseFieldName Flg\n");
                printf("--------------------------------------------------------------------------------\n");
                dbFieldDescriptor* fd = desc->columns; 
                for (int i = (int)desc->nColumns; --i >= 0;) { 
                    PRINTF(STRLITERAL("%-2d %-5s %-16s %-16s %-16s %-16s %x\n"), 
                           fd->fieldNo, 
                           fd->bTree != 0 ? "+" : "-",
                           typeMnem[fd->type],
                           fd->refTableName != NULL
                               ? fd->refTableName 
                               : (fd->type == dbField::tpArray && fd->components->refTableName != NULL)
                                  ? fd->components->refTableName
                                  : _T("(null)"),
                           fd->name, 
                           (fd->inverseRefName != NULL ? fd->inverseRefName : _T("(null)")),
                           fd->indexType
                        );
                    fd = fd->next;
                }
            }
            continue;
        
          case tkn_export:
            if (!opened) {
                error("Database not opened");
                continue;
            }   
            if (expect("xml file name", tkn_sconst)) { 
                FILE* f;
                if (STRCMP(buf, _T("-")) == 0) { 
                    f = stdout;
                } else { 
#ifdef UNICODE
#if defined(_WIN32)
                    f = _wfopen(buf, _T("w"));
#else
                    char filePath[1024];
                    wcstombs(filePath, buf, sizeof filePath);
                    f = fopen(filePath, "w");
#endif
#else
                    f = fopen(buf, "w");
#endif
                }
                if (f != NULL) { 

                    dbArray<char_t*> tables;
                    SelectionMethod  method = sel_all;

                    if (parseExportTables(tables, method)) {
                        exportDatabaseToXml(f, tables.get(), tables.length(), method);
                    }
                    fclose(f);

                } else { 
                    error("Failed to open output file");
                }
            }
            break;

          case tkn_import:
            if (!opened) {
                error("Database not opened");
                continue;
            }   
            if (accessType == dbReadOnly || accessType == dbMulticlientReadOnly) { 
                error("Operation is not possible in read-only mode");
                continue;
            }
            if (expect("xml file name", tkn_sconst)) { 
                FILE* f;
                if (STRCMP(buf, _T("-")) == 0) { 
                    f = stdin;
                } else { 
#ifdef UNICODE
#if defined(_WIN32)
                    f = _wfopen(buf, _T("r"));
#else
                    char filePath[1024];
                    wcstombs(filePath, buf, sizeof filePath);
                    f = fopen(filePath, "r");
#endif
#else
                    f = fopen(buf, "r");
#endif
                }
                if (f != NULL) { 
                    if (!importDatabaseFromXml(f)) { 
                        error("Import from XML file failed: incorrect file format");
                    }
                    fclose(f);
                } else { 
                    error("Failed to open input file");
                }
            }
            break;

          case tkn_autocommit:
            switch (scan()) {
              case tkn_on:
                autocommit = true;
                break;
               case tkn_off:
                autocommit = false;
                break;
              default:
                error("ON or OFF expected");
            }
            continue;

          case tkn_help:
            fprintf(stderr, "SubSQL commands:\n\n\
open 'database-file-name' ';'\n\
select ('*') from <table-name> where <condition> ';'\n\
create table <table-name> '('<field-name> <field-type> {',' <field-name> <field-type>}')' ';' \n\
alter table <table-name> '('<field-name> <field-type> {',' <field-name> <field-type>}')' ';' \n\
rename <table-name> '.' <old-field-name> 'to' <new-field-name> ';' \n\
update <table-name> set <field-name> '=' <expression> {',' <field-name> '=' <expression>} where <condition> ';'\n\
delete from <table-name>\n\
drop table <table-name>\n\
drop index <table-name> {'.' <field-name>} ';'\n\
create index on <table-name> {'.' <field-name>} ';'\n\
drop hash <table-name> {'.' <field-name>};\n\
create hash on <table-name> {'.' <field-name>}field> ';'\n\
insert into <table-name> values '(' <value>{',' <value>} ')' ';'\n\
backup [compactify] 'backup-file-name'\n\
restore 'backup-file-name' 'database-file-name'\n\
start server URL number-of-threads\n\
stop server URL\n\
start http server URL\n\
stop http server\n\
describe <table-name>\n\
import 'xml-file-name'\n\
export 'xml-file-name' [ ('+' | '-') <table-name> ( ',' <table-name>)* ] ';'\n\
commit\n\
rollback\n\
autocommit (on|off)\n\
show\n\
profile\n\
exit\n\
help\n\n");
            continue;
          case tkn_start:
            if (!opened) { 
                error("Database not opened");
            } else { 
                commit(); // allow server threads to process
                existedTables = tables;
                tkn = scan();
                if (tkn == tkn_http) { 
                    if (expect("server", tkn_server)
                        && expect("HTTP server URL", tkn_sconst))
                    {
#if !THREADS_SUPPORTED
                        error("Database was build without pthread support");
#else
                        startHttpServer(buf);
#endif
                    }
                } else if (tkn == tkn_server && expect("server URL", tkn_sconst)) { 
#if !THREADS_SUPPORTED
                    error("Database was build without pthread support");
#else
                    dbServer* server = dbServer::find(buf);
                    if (server == NULL) {
                        char_t* serverURL = new char_t[STRLEN(buf)+1];
                        STRCPY(serverURL, buf);
                        if (expect("number of threads", tkn_iconst)) {
                            server = new dbServer(this, serverURL, (int)ival);
                            PRINTF(_T("Server started for URL %s\n"), serverURL);
                        }
                        delete[] serverURL;
                    }
                    if (server != NULL) {
                        server->start();
                    }                    
#endif
                } else { 
                    error("Token 'server' expected");
                }
            }
            continue;
          case tkn_stop:
            tkn = scan();
            if (tkn == tkn_http) { 
                if (expect("server", tkn_server) && expect("HTTP server URL", tkn_sconst))
                {
#if !THREADS_SUPPORTED
                    error("Database was build without pthread support");
#else
                    stopHttpServer(buf);
#endif
                }
            } else if (tkn == tkn_server) { 
                if (expect("server URL", tkn_sconst))
                {
#if !THREADS_SUPPORTED
                    error("Database was build without pthread support");
#else
                    dbServer* server = dbServer::find(buf);
                    if (server != NULL) {
                        server->stop();
                        PRINTF(_T("Server stopped for URL %s\n"), buf);
                    } else {
                        FPRINTF(stderr, _T("No server was started for URL %s\n"), buf);
                    }
#endif
                }
            } else { 
                error("Token 'server' expected");
            }
            continue;
          case tkn_semi:
            putchar('\n');
            // no break
          case tkn_error:
            continue;
          case tkn_exit:
            return false;
          case tkn_version:
            printf("GigaBASE version %d.%02d\n", GIGABASE_MAJOR_VERSION, GIGABASE_MINOR_VERSION);
            continue;
          case tkn_eof:
            return true;
          default:
            error("Unexpected token");
            continue;
        }
        if (autocommit || !modified) { 
            while (droppedTables != NULL) {
                dbTableDescriptor* next = droppedTables->nextDbTable;
                delete droppedTables;
                droppedTables = next;
            }
            commit();
            existedTables = tables;
        }
    }
}

bool dbSubSql::parseExportTables(dbArray<char_t*>/*OUT*/ &tables, SelectionMethod /*OUT*/ &method)
{
    int tk = scan();

    switch(tk)
    {
    /* semi-colon: end of SQL statement. Exports all tables */
    case tkn_semi:        method = sel_all;            return true;

    /**    '+' or '-' : select tables.
     *    Reads next token: first tablename.
     */
    case tkn_include:    method = sel_named_only;    tk = scan(); break;
    case tkn_exclude:    method = sel_all_except;    tk = scan(); break;

    /**    Convenience: if filename is followed by tablename an implicit '+' is assumed.
     *    The first tablename alread read, no need to call scan()
     */
    case tkn_ident:        method = sel_named_only;    break;

    default:
        error("Table selection method ('+' or '-' ) or ';' expected ");
        return false;
    }

    /* List of tablenames [table1, table2 ,table3] */
    for(;;)
    {
        if (tk != tkn_ident)
        {
            error("Expected table name");
            return false;
        }

        if (findTable(name) == NULL)
        {
            error("No such table in database");
            return false;
        }
        tables.append(name);

        /* ';' = end of list, ',' = seperator, else fail */
        int tk = scan();
        if (tk == tkn_semi) break;
        if (tk != tkn_comma)
        {
            error("Comma expected");
            return false;
        }

        /* prepare: next token */
        tk = scan();
    }

    return true;
}
 

void dbSubSql::handleError(dbErrorClass error, char const* msg, int arg)
{
    dbDatabaseThreadContext* ctx = threadContext.get();
    if (ctx == NULL || ctx->interactive) {
        const int screenWidth = 80;
        int col;
        switch (error) {
          case QueryError:
            col = arg % screenWidth;
            if (interactiveMode) {
                while (--col >= 0) putc('-', stderr);
                fprintf(stderr, "^\n%s\n", msg);
            } else {
                fprintf(stderr, "%s at line %d position %d\n", msg, line, arg);
            }
            break;
          case ArithmeticError:
            fprintf(stderr, "%s\n", msg);
            break;
          case IndexOutOfRangeError:
            fprintf(stderr, "Index %d is out of range\n", arg);
            break;
          case NullReferenceError:
            fprintf(stderr, "Null object reference is accessed\n");
            break;
          case DatabaseOpenError:
            return;
          default:
            dbDatabase::handleError(error, msg, arg);
        }
        //
        // Recovery
        //
        if (interactiveMode) {
            int ch;
            while ((ch = get()) != '\n' && ch != T_EOF);
        } else {
            fseek(in, 0, SEEK_END);
        }
    }
    switch (error) { 
      case DatabaseOpenError:
      case InconsistentInverseReference:
        fprintf(stderr, "%s\n", msg);
        break;
      default:
#ifdef THROW_EXCEPTION_ON_ERROR
        if (msg == NULL) { 
            msg = errorMessage[error];
        }
        throw dbException(error, msg, arg);
#else
        if (ctx != NULL) { 
            if (ctx->catched) {     
                longjmp(ctx->unwind, error);
            } else { 
                abort();
            }
        }
#endif
    }
}

void dbSubSql::run(int argc, char* argv[])
{
    int i;
    bool daemon = false;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-") == 0) {
            break;
        } 
        if (strcmp(argv[i], "-daemon") == 0) {
            daemon = true;
            continue;
        }
        in = fopen(argv[i], "r");
        if (in == NULL) {
            fprintf(stderr, "Failed to open '%s' file\n", argv[i]);
        } else {
            if (!parse()) {
                if (opened) {
                    delete[] databasePath;
                    close();
                }
#if THREADS_SUPPORTED
                dbServer::cleanup();
#endif
                return;
            }
        }
    }
    if (i == argc) { 
        printf("SubSQL interactive utility for GigaBASE v. %d.%02d\n"
               "Type 'help' for more information\n",
               GIGABASE_MAJOR_VERSION, GIGABASE_MINOR_VERSION);
        interactiveMode = true;
        dbDatabaseThreadContext* ctx = threadContext.get();
        if (ctx != NULL) { 
            ctx->interactive = true;
        }
    }
    if (daemon) { 
        dbMutex mutex;
        dbCriticalSection cs(mutex);
        daemonTerminationEvent.open();
        daemonTerminationEvent.wait(mutex);
        daemonTerminationEvent.close();
    } else { 
        in = stdin;
        parse();
    }
    if (opened) {
        delete[] databasePath;
        close();
    }
#if THREADS_SUPPORTED
    dbServer::cleanup();
#endif
}

#define HTML_HEAD "Content-type: text/html\r\n\r\n\
<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2//EN\"><HTML><HEAD>\
<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\"/>"

#define BODY "<BODY BGCOLOR=\"#c0c0c0\">"

#define EMPTY_LIST "<OPTION>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</OPTION></SELECT><BR>"


void httpQueryError(WWWconnection& con, char const* msg, char const* table)
{
    con << TAG <<
        HTML_HEAD "<TITLE>BUGDB error</TITLE></HEAD>"
        BODY
        "<CENTER><FONT SIZE=+2 COLOR=\"#FF0000\">"
        << msg << "</FONT></CENTER><P><FORM METHOD=POST ACTION=\"" << con.getStub() << "\">"
        "<INPUT TYPE=HIDDEN NAME=socket VALUE=\"" << con.getAddress() << "\">"
        "<INPUT TYPE=hidden NAME=table VALUE=\"" << table << "\">"
        "<INPUT TYPE=hidden NAME=page VALUE=queryPage>"
        "<CENTER><INPUT TYPE=submit VALUE=\"Ok\"></CENTER></FORM></BODY></HTML>";
}


void httpError(WWWconnection& con, char const* msg)
{
    con << TAG <<
        HTML_HEAD "<TITLE>BUGDB error</TITLE></HEAD>"
        BODY
        "<CENTER><FONT SIZE=+2 COLOR=\"#FF0000\">"
        << msg << "</FONT></CENTER><P><FORM METHOD=POST ACTION=\"" << con.getStub() << "\">"
        "<INPUT TYPE=HIDDEN NAME=\"socket\" VALUE=\"" << con.getAddress() << "\">"
        "<INPUT TYPE=hidden NAME=\"page\" VALUE=defaultPage>"
        "<CENTER><INPUT TYPE=submit VALUE=\"Ok\"></CENTER></FORM></BODY></HTML>";
}


bool defaultPage(WWWconnection& con)
{
    ((dbSubSql*)con.userData)->defaultPage(con);
    return true;
}


void dbSubSql::defaultPage(WWWconnection& con)
{
    con << TAG <<
        HTML_HEAD "<TITLE>Database browser</TITLE></HEAD>"
        BODY
        "<TABLE><TR><TH align=left>Database path</TH><TD>" << databasePath << "</TD></TR>"
        "<TR><TH align=left>GigaBASE version</TH><TD>" << GIGABASE_MAJOR_VERSION << "." << GIGABASE_MINOR_VERSION << "</TD></TR>"
        "<TR><TH align=left>Database version</TH><TD>" << header->versionMajor << "." << header->versionMinor << "</TD></TR>"
        "<TR><TH align=left>Database size</TH><TD>" << (db_int8)(header->root[1-curr].size / 1024) << "Kb</TD></TR>"
        "</TABLE><P>"
        "<H2>Tables:</H2><FORM METHOD=POST ACTION=\"" << con.getStub() << "\">"
        "<INPUT TYPE=HIDDEN NAME=socket VALUE=\"" << con.getAddress() << "\">"
        "<INPUT TYPE=hidden NAME=page VALUE=queryPage>"
        "<SELECT SIZE=10 NAME=\"table\">";
    if (tables != NULL && tables->nextDbTable != NULL) { 
        for (dbTableDescriptor* desc=tables; desc != NULL; desc=desc->nextDbTable)
        { 
            if (STRCMP(desc->name, STRLITERAL("Metatable")) != 0) { 
                con << TAG << "<OPTION VALUE=\"" << desc->name << "\">" << desc->name << "</OPTION>";
            }
        }
    } else { 
        con << TAG << EMPTY_LIST;
    }
    con << TAG << "</SELECT><P><INPUT TYPE=submit VALUE=Query></FORM></BODY></HTML>";
}
    
bool queryPage(WWWconnection& con)
{
    ((dbSubSql*)con.userData)->queryPage(con);
    return true;
}

void dbSubSql::queryPage(WWWconnection& con)
{
    char* tableName = con.get("table");
    if (tableName == NULL) { 
        httpError(con, "Table not selected");
        return;
    }
    dbTableDescriptor* desc;
#ifdef UNICODE 
    char_t buf[1024];
    mbstowcs(buf, tableName, itemsof(buf));
    desc = findTableByName(buf);
#else
    desc = findTableByName(tableName);
#endif
    if (desc == NULL) {
        httpError(con, "No such table");
        return;
    }
    char* history = con.get("history");
    if (history == NULL) { 
        history = "";
    }
    con << TAG <<
        HTML_HEAD "<TITLE>Table query</TITLE></HEAD>"
        BODY
        "<TABLE><TR><TH align=left>Table name</TH><TD>" << tableName << "</TD></TR>"
        "<TR><TH align=left>Number of rows</TH><TD>" << (int)desc->nRows << "</TD></TR>"
        "</TABLE><P>"
        "<TABLE BORDER><TR><TH>Field name</TH><TH>Field type</TH><TH>Referenced table</TH><TH>Inverse reference</TH><TH>Indexed</TH></TR>";
    dbFieldDescriptor* fd = desc->columns; 
    for (int i = (int)desc->nColumns; --i >= 0;) { 
        con << TAG << "<TR><TD>" << fd->name << "</TD><TD>" << typeMnem[fd->type] << "</TD><TD>" 
            << (fd->refTableName ? fd->refTableName : _T(" "))  << "</TD><TD>" 
            << (fd->refTableName != NULL 
                ? fd->refTableName 
                : (fd->type == dbField::tpArray && fd->components->refTableName != NULL)
                  ? fd->components->refTableName
                  : _T(" "))
            << "</TD><TD align=center>"
            << ((fd->bTree != 0) ? _T("+") : _T(" ")) << "</TD></TR>";
        fd = fd->next;
    }
    con << TAG << "</TABLE><P><TABLE>"
        "<FORM METHOD=POST ACTION=\"" << con.getStub() << "\">"
        "<INPUT TYPE=HIDDEN NAME=socket VALUE=\"" << con.getAddress() << "\">"
        "<INPUT TYPE=hidden NAME=page VALUE=selectionPage>"
        "<INPUT TYPE=hidden NAME=table VALUE=\"" << tableName << "\">"
        "<TR><TD>SELECT FROM <B>" << tableName << "</B> WHERE</TD>"
        "<TD><INPUT TYPE=text NAME=query VALUE=\""
        << history << "\" SIZE=40></TD>"
        "<TD><INPUT type=submit value=Select></TD></TR></FORM>";
    if (historyUsed != 0) { 
        con << TAG << "<FORM METHOD=POST ACTION=\"" << con.getStub() << "\">"
            "<INPUT TYPE=HIDDEN NAME=socket VALUE=\"" << con.getAddress() << "\">"
            "<INPUT TYPE=hidden NAME=page VALUE=queryPage>"
            "<INPUT TYPE=hidden NAME=table VALUE=\"" << tableName << "\">"
            "<TR><TD align=right>Query history</TD>"
            "<TD><SELECT SIZE=1 NAME=history>";
        for (unsigned i = historyCurr, j = historyUsed; j != 0; j -= 1) { 
            char* h = queryHistory[--i % MAX_HISTORY_SIZE];
            con << TAG << "<OPTION VALUE=\"" << h << "\">" << h << "</OPTION>";
        }
        con << TAG << "</TD><TD><INPUT type=submit value=Edit></TD></TR></FORM>";
    }
    con << TAG << "</TABLE></FORM>"
        "<P><FORM METHOD=POST ACTION=\"" << con.getStub() << "\">" 
        "<INPUT TYPE=HIDDEN NAME=socket VALUE=\"" << con.getAddress() << "\">"
        "<INPUT TYPE=hidden NAME=page VALUE=defaultPage>"
        "<INPUT TYPE=submit VALUE=\"Another table\"></FORM></BODY></HTML>";
}

    
enum ComponentType { 
    RecordComponent,
    ArrayComponent, 
    StructureComponent
};

void httpDumpRecord(WWWconnection& con, byte* base, dbFieldDescriptor* first, ComponentType componentType)
{
    int i, n;
    byte* elem;
    dbFieldDescriptor* fd = first;
    do {
        if (componentType == StructureComponent) { 
            con << TAG << "<TR><TH>" << fd->name << "</TH><TD>";
        } else if (componentType == RecordComponent) { 
            con << TAG << "<TD>";
        }
        switch (fd->type) {
          case dbField::tpBool:
            con << TAG << (*(bool*)(base + fd->dbsOffs) ? "true" : "false");
            break;
          case dbField::tpInt1:
            con << TAG << *(int1*)(base + fd->dbsOffs);
            break;
          case dbField::tpInt2:
            con << TAG << *(int2*)(base + fd->dbsOffs);
            break;
          case dbField::tpInt4:
            con << TAG << *(int4*)(base + fd->dbsOffs);
            break;
          case dbField::tpInt8:
            con << TAG << *(db_int8*)(base + fd->dbsOffs);
            break;
          case dbField::tpReal4:
            con << TAG << *(real4*)(base + fd->dbsOffs);
            break;
          case dbField::tpReal8:
            con << TAG << *(real8*)(base + fd->dbsOffs);
            break;
          case dbField::tpRectangle:
            {
                rectangle& r = *(rectangle*)(base + fd->dbsOffs);
                con << TAG << "<TABLE BORDER><TR>";                
                for (i = 0; i < rectangle::dim; i++) { 
                    con << TAG << "<TD>" << r.boundary[i] << "</TD>";
                }
                con << TAG << "</TR><TR>";                
                for (i = 0; i < rectangle::dim; i++) { 
                    con << TAG << "<TD>" << r.boundary[rectangle::dim+i] << "</TD>";
                }
                con << TAG << "</TR></TABLE>";
            }
            break;
          case dbField::tpString:
            con << TAG << "\"" << (char_t*)((char*)base+((dbVarying*)(base+fd->dbsOffs))->offs) << "\"";
            break;
          case dbField::tpReference:
            {
                oid_t oid = *(oid_t*)(base + fd->dbsOffs);
                if (oid == 0) { 
                    con << TAG << "null";
                } else { 
                    con << TAG << "<A HREF=\"" <<  con.getStub() << "?socket="
                        << con.getAddress() << "&page=selectionPage&table=" << URL << fd->refTableName <<  "&query=" 
                        << URL << "current=" << oid << TAG << "\">@" << oid << "</A>";
                }
            }
            break;
          case dbField::tpRawBinary:
            n = (int)fd->dbsSize;
            elem = base + fd->dbsOffs;
            con << TAG << "\"";
            for (i = 0; i < n; i++) {
                char buf[8];
                sprintf(buf, "\\x%02x", *elem++);
                con << TAG << buf;
            }
            con << TAG << "\"";
            break;
          case dbField::tpArray:
            n = ((dbVarying*)(base + fd->dbsOffs))->size;
            elem = base + ((dbVarying*)(base + fd->dbsOffs))->offs;
            con << TAG << "<OL>";
            for (i = 0; i < n; i++) {
                con << TAG << "<LI>";                
                httpDumpRecord(con, elem, fd->components, ArrayComponent);
                elem += fd->components->dbsSize;
            }
            con << TAG << "</OL>";
            break;
          case dbField::tpStructure:
            con << TAG << "<TABLE BORDER>"; 
            httpDumpRecord(con, base, fd->components,RecordComponent);
            con << TAG << "</TABLE>"; 
        }
        if (componentType == StructureComponent) { 
            con << TAG << "</TD></TR>";
        } else if (componentType == RecordComponent) { 
            con << TAG << "</TD>";
        }
    } while ((fd = fd->next) != first);
}

bool selectionPage(WWWconnection& con)
{
    ((dbSubSql*)con.userData)->selectionPage(con);
    return true;
}

void dbSubSql::selectionPage(WWWconnection& con)
{
    char const* tableName = con.get("table");
    char const* condition = con.get("query");
    dbTableDescriptor* desc;
#ifdef UNICODE 
    char_t buf[1024];
    mbstowcs(buf, tableName, itemsof(buf));
    desc = findTableByName(buf);
#else
    desc = findTableByName(tableName);
#endif
    if (desc == NULL) {
        httpError(con, "No such table");
        return;
    }
    if (condition == NULL) {
        httpError(con, "Condition was not specified");
        return;
    }
    if (strlen(condition) > 0 
        && (historyUsed == 0 
            || strcmp(condition, queryHistory[unsigned(historyCurr-1)%MAX_HISTORY_SIZE]) != 0))
    {
        char* h = new char[strlen(condition)+1];
        strcpy(h, condition);
        if (historyCurr == historyUsed) { 
            historyUsed += 1;
        } else { 
            delete[] queryHistory[historyCurr];
        }
        queryHistory[historyCurr] = h;
        if (++historyCurr == MAX_HISTORY_SIZE) { 
            historyCurr = 0;
        }
    }
    dbAnyCursor cursor(*desc, dbCursorViewOnly, NULL);
    query.pos = pos;
    dbDatabaseThreadContext* ctx = threadContext.get();
    ctx->interactive = false;
    ctx->catched = true;
#ifdef THROW_EXCEPTION_ON_ERROR
    try {
#else
    if (setjmp(ctx->unwind) == 0) {
#endif
#ifdef UNICODE 
        mbstowcs(buf, condition, sizeof(buf)/sizeof(wchar_t));
        query = buf;
#else
        query = condition;
#endif
        cursor.reset();
        select(&cursor, query);
        if (query.compileError()) {
            dbExprNodeAllocator::instance.reset();
            ctx->catched = false;
            httpQueryError(con, "query syntax error", tableName);
            return;
        }
        con << TAG <<
            HTML_HEAD "<TITLE>Selection</TITLE></HEAD>" 
            BODY
            "<H2>Selection from table " << tableName << "</H2>"
            "<TABLE BORDER><TR><TH>OID</TH>";
        dbFieldDescriptor* fd = desc->columns;
        do {
            con << TAG << "<TH>" << fd->name << "</TH>";
        } while ((fd = fd->next) != desc->columns);
        con << TAG << "</TR>";

        int nSelected = 0;
        dbGetTie tie;
        if (cursor.gotoFirst()) {
            do {
                nSelected += 1;
                con << TAG << "<TR><TD>@" << cursor.currId << "</TD>";
                httpDumpRecord(con, (byte*)getRow(tie, cursor.currId),
                               cursor.table->columns, RecordComponent);
                con << TAG << "</TR>";
            } while (cursor.gotoNext());
            con << TAG << "</TABLE>";
            if (nSelected > 1) {
                con << TAG << "<P>" << nSelected << " records selected";
            }
        } else { 
            con << TAG << "</TABLE><P>No records selected";
        }
#ifdef THROW_EXCEPTION_ON_ERROR
    } catch(dbException const& x) {
        httpQueryError(con, x.getMsg(), tableName);
        ctx->catched = false;
        commit(); // release locks
        return;
    }
#else
    } else { 
        httpQueryError(con, "Query error", tableName);
        if (query.mutexLocked) { 
            query.mutexLocked = false;
            query.mutex.unlock();
        }
        ctx->catched = false;
        commit(); // release locks
        return;
    }
#endif
    ctx->catched = false;
    commit(); // release locks
    
    con << TAG << 
        "<P><FORM METHOD=POST ACTION=\"" << con.getStub() << "\">" 
        "<INPUT TYPE=HIDDEN NAME=socket VALUE=\"" << con.getAddress() << "\">"
        "<INPUT TYPE=hidden NAME=table VALUE=\"" << tableName << "\">"
        "<INPUT TYPE=hidden NAME=page VALUE=queryPage>"
        "<INPUT TYPE=submit VALUE=\"New query\"></FORM></BODY></HTML>";
}
    

WWWapi::dispatcher dispatchTable[] = {
    {"defaultPage", defaultPage},
    {"queryPage", queryPage},
    {"selectionPage", selectionPage}
};




void dbSubSql::startHttpServer(char_t const* address) 
{
    if (httpServerRunning) { 
        error("HTTP server already started");
    } else { 
        httpServer = new HTTPapi(*this, itemsof(dispatchTable), dispatchTable);
        char const* socketAddress;
#ifdef UNICODE 
        char buf[1024];
        wcstombs(buf, address, sizeof buf);
        socketAddress = buf;
#else
        socketAddress = address;
#endif
        if (!httpServer->open(socketAddress, socket_t::sock_global_domain)) {
            delete httpServer;
            error("Failed to open HTTP session");
        } else { 
            httpServerRunning = true;
            httpServerThread.create(httpServerThreadProc, this);
        }
    }
}

void dbSubSql::stopHttpServer(char_t const*) 
{
    if (!httpServerRunning) { 
        error("HTTP server was not started");
    } else {
        httpServerRunning = false;
        httpServer->cancel();        
    }
}

void thread_proc dbSubSql::httpServerThreadProc(void* arg) 
{
    ((dbSubSql*)arg)->httpServerLoop();
}
    
void  dbSubSql::httpServerLoop() 
{
    WWWconnection con;
    con.userData = this;
    attach();
    while (httpServer->connect(con) && httpServerRunning && httpServer->serve(con));
    delete httpServer;
    detach();
    httpServerRunning = false;
}
             
    
#ifdef _WINCE

#define MAX_ARGS 32

#ifdef WINONLY
int WINAPI WinMain(
                   HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    char* line = m_cmdline;
#else
int WINAPI WinMain(
                   HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPTSTR lpCmdLine,
                   int nCmdShow)
{
    int len = wcslen(lpCmdLine);
    char* line   = new char[len+1];
    wcstombs(line, lpCmdLine, len);    
#endif
    int argc = 1;
    char* argv[MAX_ARGS];
    argv[0] = NULL;
    if (*line != '\0') {
        while (true) { 
            if (*line == '\"') { 
                argv[argc++] = ++line;
                while (*++line != '\"' && *line != '\0');
            } else { 
                argv[argc++] = line;
                while (*++line != ' ' && *line != '\0');
            }
            if (*line == '\0') { 
                break;
            }
            *line++ = '\0';
        }
    }
    argv[argc] = NULL;
#else
int __cdecl main(int argc, char* argv[]) 
{
#endif    
    dbDatabase::dbAccessType accessType;
    if (getenv("SUBSQL_MULTICLIENT") != NULL) {
        accessType = (getenv("SUBSQL_READONLY") != NULL)
            ?  dbDatabase::dbMulticlientReadOnly : dbDatabase::dbMulticlientReadWrite;
    } else { 
        accessType = (getenv("SUBSQL_READONLY") != NULL)
            ?  dbDatabase::dbReadOnly : dbDatabase::dbAllAccess;
    }
    char* locale = getenv("SUBSQL_LOCALE");
    if (locale != NULL) { 
        setlocale(LC_COLLATE, locale);
        setlocale(LC_CTYPE, locale);
    }
    size_t pagePoolSize = 0;
    char* pagePoolSizeStr = getenv("SUBSQL_POOL_SIZE");
    if (pagePoolSizeStr != NULL) { 
        pagePoolSize = atoi(pagePoolSizeStr);
    }
    dbSubSql db(accessType, pagePoolSize);
    db.run(argc, argv);
    return 0;
}

