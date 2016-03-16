//-< CURSOR.CPP >----------------------------------------------------*--------*
// GigaBASE                  Version 1.0         (c) 1999  GARRET    *     ?  *
// (Post Relational Database Management System)                      *   /\|  *
//                                                                   *  /  \  *
//                          Created:     20-Nov-98    K.A. Knizhnik  * / [] \ *
//                          Last update: 21-Dec-98    K.A. Knizhnik  * GARRET *
//-------------------------------------------------------------------*--------*
// Table cursor
//-------------------------------------------------------------------*--------*

#define INSIDE_GIGABASE

#include "gigabase.h"
#include "compiler.h"
#include "btree.h"

BEGIN_GIGABASE_NAMESPACE

size_t dbSelection::buildSelectionBitmapThreshold = 100;

oid_t dbTableIterator::next()
{
    if (curr != 0) { 
        dbRecord rec;
        oid_t oid = curr;
        while (true) { 
            cursor->db->getHeader(rec, oid);
            oid = rec.next;
            if (oid != 0) { 
                if (filter == NULL 
                    || cursor->db->evaluateBoolean(filter, oid, cursor->table, cursor)) 
                {
                    return curr = oid;
                }
            } else { 
                break;
            }
        }
    }
    return 0;
}

oid_t dbTableIterator::prev()
{
    if (curr != 0) { 
        dbRecord rec;
        oid_t oid = curr;
        while (true) { 
            cursor->db->getHeader(rec, oid);
            oid = rec.prev;
            if (oid != 0) { 
                if (filter == NULL 
                    || cursor->db->evaluateBoolean(filter, oid, cursor->table, cursor)) 
                {
                    return curr = oid;
                }
            } else { 
                break;
            }
        }
    }
    return 0;
}

oid_t dbTableIterator::first()
{
    oid_t oid = cursor->table->firstRow;
    while (oid != 0
           && filter != NULL 
           && !cursor->db->evaluateBoolean(filter, oid, cursor->table, cursor))
    {
        dbRecord rec;
        cursor->db->getHeader(rec, oid);
        oid = rec.next;
    }
    return curr = oid;
}

    
oid_t dbTableIterator::last()
{
    oid_t oid = cursor->table->lastRow;
    while (oid != 0
           && filter != NULL 
           && !cursor->db->evaluateBoolean(filter, oid, cursor->table, cursor))
    {
        dbRecord rec;
        cursor->db->getHeader(rec, oid);
        oid = rec.prev;
    }
    return curr = oid;
}


void dbSelection::reset()
{
    while (first.next != &first) {
        delete first.next;
    }
    first.nRows = 0;
    curr = &first;
    nRows = 0;
    pos = 0;
}

void dbSelection::reverse()
{
    segment* seg = &first;
    do {
        segment* next = seg->next;
        seg->next = seg->prev;
        seg->prev = next;
        for (int l = 0, r = (int)seg->nRows-1; l < r; l++, r--) {
            oid_t oid = seg->rows[l];
            seg->rows[l] = seg->rows[r];
            seg->rows[r] = oid;
        }
        seg = next;
    } while (seg != &first);
}

void dbSelection::truncate(size_t from, size_t length)
{
    if (from == 0 && length >= nRows) { 
        // do nothing
        return;
    }
    segment* src = &first;
    bool empty = true;
    if (from < nRows) { 
        do { 
            if (from < src->nRows) { 
                empty = false;
                break;
            }
            from -= src->nRows;
        } while ((src = src->next) != &first);
    }
    if (from + length > nRows) { 
        length = nRows - from;
    }
    nRows = 0;
    segment* dst = &first;
    size_t pos = 0;
    if (!empty) { 
        while (length != 0) { 
            size_t n = src->nRows - from;
            if (n > length) { 
                n = length;
            }
            if (dst->nRows == pos) { 
                dst = dst->next;
                pos = 0;
            }
            if (n > dst->nRows - pos) { 
                n = dst->nRows - pos;
            }
            memcpy(dst->rows + pos, src->rows + from, n*sizeof(oid_t));
            pos += n;
            length -= n;
            nRows += n;
            if ((from += n) == src->nRows) { 
                if ((src = src->next) == &first) { 
                    break;
                }
                from = 0;
            }
        }
    }
    dst->nRows = pos;
    dst = dst->next;
    while (dst != &first) { 
        segment* next = dst->next; 
        delete dst;
        dst = next;
    }
}

int dbSelection::compare(oid_t o1, dbRecord* a, oid_t o2, dbRecord* b, dbOrderByNode* order)
{
    byte* p = (byte*)a;
    byte* q = (byte*)b;
    int diff = 0;
    do {
        if (order->expr != NULL) { 
            dbDatabase* db = order->table->db;
            dbInheritedAttribute   iattr1;
            dbInheritedAttribute   iattr2;
            dbSynthesizedAttribute sattr1;
            dbSynthesizedAttribute sattr2;
            iattr1.db = iattr2.db = db;
            iattr1.table = iattr2.table = order->table;
            iattr1.record = sattr1.base = p;
            iattr1.oid = o1;
            iattr2.record = sattr2.base = q;
            iattr2.oid = o2;
            db->execute(order->expr, iattr1, sattr1);
            db->execute(order->expr, iattr2, sattr2);
            switch (order->expr->type) { 
              case tpInteger:
                diff = sattr1.ivalue < sattr2.ivalue ? -1 : sattr1.ivalue == sattr2.ivalue ? 0 : 1;
                break;
              case tpReal:
                diff = sattr1.fvalue < sattr2.fvalue ? -1 : sattr1.fvalue == sattr2.fvalue ? 0 : 1;
                break;
              case tpBoolean:
                diff = sattr1.bvalue != 0 ? sattr2.bvalue != 0 ? 0 : 1 : sattr2.bvalue != 0 ? -1 : 0;
                break;
              case tpString:
#ifdef USE_LOCALE_SETTINGS
                diff = STRCOLL((char_t*)sattr1.array.base, (char_t*)sattr2.array.base); 
#else
                diff = STRCMP((char_t*)sattr1.array.base, (char_t*)sattr2.array.base);
#endif
                break;
              case tpReference:
                diff = sattr1.oid < sattr2.oid ? -1 : sattr1.oid == sattr2.oid ? 0 : 1;
                break;
              default:
                assert(false);
            }
            iattr1.free(sattr1);
            iattr2.free(sattr2);           
        } else { 
            int offs = order->field->dbsOffs;
            switch (order->field->type) {
              case dbField::tpBool:
                diff = *(bool*)(p + offs) - *(bool*)(q + offs);
                break;
              case dbField::tpInt1:
                diff = *(int1*)(p + offs) - *(int1*)(q + offs);
                break;
              case dbField::tpInt2:
                diff = *(int2*)(p + offs) - *(int2*)(q + offs);
                break;
              case dbField::tpInt4:
              case dbField::tpArray: // compre arrays length
                diff = *(int4*)(p + offs) < *(int4*)(q + offs) ? -1 :
                    *(int4*)(p + offs) == *(int4*)(q + offs) ? 0 : 1;
                break;
              case dbField::tpInt8:
                diff = *(db_int8*)(p + offs) < *(db_int8*)(q + offs) ? -1 :
                    *(db_int8*)(p + offs) == *(db_int8*)(q + offs) ? 0 : 1;
                break;
              case dbField::tpReference:
                diff = *(oid_t*)(p + offs) < *(oid_t*)(q + offs) ? -1 :
                    *(oid_t*)(p + offs) == *(oid_t*)(q + offs) ? 0 : 1;
                break;
              case dbField::tpReal4:
                diff = *(real4*)(p + offs) < *(real4*)(q + offs) ? -1 :
                    *(real4*)(p + offs) == *(real4*)(q + offs) ? 0 : 1;
                break;
              case dbField::tpReal8:
                diff = *(real8*)(p + offs) < *(real8*)(q + offs) ? -1 :
                    *(real8*)(p + offs) == *(real8*)(q + offs) ? 0 : 1;
                break;
              case dbField::tpString:
#ifdef USE_LOCALE_SETTINGS
                diff = STRCOLL((char_t*)(p + ((dbVarying*)(p + offs))->offs),
                               (char_t*)(q + ((dbVarying*)(q + offs))->offs));
#else
                diff = STRCMP((char_t*)(p + ((dbVarying*)(p + offs))->offs),
                              (char_t*)(q + ((dbVarying*)(q + offs))->offs));
#endif
                break;
              case dbField::tpRawBinary:
                diff = order->field->comparator(p + offs, q + offs, order->field->dbsSize);
                break;
              default:
                assert(false);
            }
        }
        if (!order->ascent) {
            diff = -diff;
        }
    } while (diff == 0 && (order = order->next) != NULL);

    return diff;
}

struct dbSortContext {
    dbDatabase*    db;
    dbOrderByNode* order;
};

inline db_int8 packStrKey(byte* p, bool caseInsensitive)
{
    char_t cnvBuf[8/sizeof(char_t) + 1];
    if (caseInsensitive) { 
        size_t i;
        for (i = 0; i < 8/sizeof(char_t) && *((char_t*)p + i) != 0; i++) {
            cnvBuf[i] = TOLOWER(*((char_t*)p + i));
        }
        cnvBuf[i] = 0;
        p = (byte*)cnvBuf;
    }            
#ifdef USE_LOCALE_SETTINGS
    char_t buf[8/sizeof(char_t)];
    STRXFRM(buf, (char_t*)p, itemsof(buf));
    p = (byte*)buf;
#endif
    db_int8 pkey = 0;
    for (size_t i = 0; i < 8/sizeof(char_t) && *((char_t*)p + i) != 0; i++) {
        char_t ch = *((char_t*)p + i);
        pkey |= ((db_int8)ch & ((1 << sizeof(char_t)*8)-1))
            << ((8/sizeof(char_t)-1-i)*8*sizeof(char_t));
    }
    // As far as signed comparison is used for packed key,
    // and strcmp compares characters as unsign, we should make this
    // correction
    return pkey - ((db_int8)-1 << 63);
}

static int __cdecl cmpStrKey(void const* a, void const* b)
{
#ifdef USE_LOCALE_SETTINGS
    return STRCOLL(((dbSortRecord*)a)->u.strKey, ((dbSortRecord*)b)->u.strKey); 
#else
    return STRCMP(((dbSortRecord*)a)->u.strKey, ((dbSortRecord*)b)->u.strKey); 
#endif
}

static int __cdecl cmpIntKey(void const* a, void const* b)
{
    return ((dbSortRecord*)a)->u.intKey < ((dbSortRecord*)b)->u.intKey ? -1
        : ((dbSortRecord*)a)->u.intKey == ((dbSortRecord*)b)->u.intKey ? 0 : 1;
}

static int __cdecl cmpLongKey(void const* a, void const* b)
{
    return ((dbSortRecord*)a)->u.longKey < ((dbSortRecord*)b)->u.longKey ? -1
        : ((dbSortRecord*)a)->u.longKey == ((dbSortRecord*)b)->u.longKey ? 0 : 1;
}

static int __cdecl cmpRealKey(void const* a, void const* b)
{
    return ((dbSortRecord*)a)->u.realKey < ((dbSortRecord*)b)->u.realKey ? -1
        : ((dbSortRecord*)a)->u.realKey == ((dbSortRecord*)b)->u.realKey ? 0 : 1;
}

static int __cdecl cmpStrKeyDesc(void const* a, void const* b)
{
#ifdef USE_LOCALE_SETTINGS
    return -STRCOLL(((dbSortRecord*)a)->u.strKey, ((dbSortRecord*)b)->u.strKey); 
#else
    return -STRCMP(((dbSortRecord*)a)->u.strKey, ((dbSortRecord*)b)->u.strKey); 
#endif
}

static int __cdecl cmpIntKeyDesc(void const* a, void const* b)
{
    return ((dbSortRecord*)a)->u.intKey > ((dbSortRecord*)b)->u.intKey ? -1
        : ((dbSortRecord*)a)->u.intKey == ((dbSortRecord*)b)->u.intKey ? 0 : 1;
}

static int __cdecl cmpLongKeyDesc(void const* a, void const* b)
{
    return ((dbSortRecord*)a)->u.longKey > ((dbSortRecord*)b)->u.longKey ? -1
        : ((dbSortRecord*)a)->u.longKey == ((dbSortRecord*)b)->u.longKey ? 0 : 1;
}

static int __cdecl cmpRealKeyDesc(void const* a, void const* b)
{
    return ((dbSortRecord*)a)->u.realKey > ((dbSortRecord*)b)->u.realKey ? -1
        : ((dbSortRecord*)a)->u.realKey == ((dbSortRecord*)b)->u.realKey ? 0 : 1;
}


static dbThreadContext<dbSortContext> sortThreadContext;


int __cdecl dbSelection::exactKeyCmp(void const* a, void const* b)
{
    dbGetTie ta, tb;
    dbSortContext* ctx = sortThreadContext.get();
    return compare(((dbSortRecord*)a)->oid, ctx->db->getRow(ta, ((dbSortRecord*)a)->oid),
                   ((dbSortRecord*)b)->oid, ctx->db->getRow(tb, ((dbSortRecord*)b)->oid),
                   ctx->order);
}

int __cdecl dbSelection::udtComparator(void const* a, void const* b)
{
    dbSortContext* ctx = sortThreadContext.get();
    int rc = ctx->order->field->comparator(((dbSortRecord*)a)->u.rawKey, ((dbSortRecord*)b)->u.rawKey, 
                                           ctx->order->field->dbsSize);
    return ctx->order->ascent ? rc : -rc;
}

void dbSelection::toArray(oid_t* oids) const
{
    segment const* seg = &first;
    do {
        for (int i = 0, n = (int)seg->nRows; i < n; i++) {
            *oids++ = seg->rows[i];
        }
    } while ((seg = seg->next) != &first);
}

static int __cdecl compareOids(void const* a, void const* b)
{
    return *(oid_t*)a < *(oid_t*)b ? -1 : *(oid_t*)a == *(oid_t*)b ? 0 : 1;
}


void dbSelection::allocateBitmap(dbDatabase* db) 
{
    size_t size = (size_t)((db->currIndexSize + 31) / 32);
    if (size > bitmapSize) {
        delete[] bitmap;
        bitmap = new int4[size];
        bitmapSize = size;
    }
    memset(bitmap, 0, size*4);
}

void dbSelection::deallocateBitmap() 
{
    if (bitmap != NULL) { 
        delete[] bitmap;
        bitmap = NULL;
        bitmapSize = 0;
    }
}

void dbSelection::merge(dbDatabase* db, dbSelection& selection)
{
    size_t n1 = nRows, n2 = selection.nRows;
    TRACE_MSG((STRLITERAL("Performing indexed merge of %d and %d rows\n"), n1, n2));
    segment* dst = &first;
    size_t n = 0, j = 0;

    if (n1 > buildSelectionBitmapThreshold && n2 > buildSelectionBitmapThreshold) { 
        allocateBitmap(db);
        int4* bm = bitmap;
        segment const* seg = &selection.first;        
        do {
            for (size_t i = 0, sn = seg->nRows; i < sn; i++) {
                oid_t o = seg->rows[i];
                bm[(size_t)o >> 5] |= 1 << ((int)o & 31);
            }
        } while ((seg = seg->next) != &selection.first);

        seg = &first;
        do {
            for (size_t i = 0, sn = seg->nRows; i < sn; i++) {
                oid_t o = seg->rows[i];
                if (bm[(size_t)o >> 5] & (1 << ((int)o & 31))) {
                    if (j == dst->nRows) { 
                        dst = dst->next;
                        j = 0;
                    }                    
                    dst->rows[j++] = o;
                    n += 1;
                }
            }
        } while ((seg = seg->next) != &first);     
    } else { 
        dbSmallBuffer<oid_t> buf2(n2);
        dbSmallBuffer<oid_t> buf1(n1);
        oid_t* refs1 = buf1.base();
        oid_t* refs2 = buf2.base();
        toArray(refs1);
        selection.toArray(refs2);
        qsort(refs1, nRows, sizeof(oid_t), &compareOids);
        qsort(refs2, selection.nRows, sizeof(oid_t), &compareOids);
        size_t i1 = 0, i2 = 0;
        
        while (true) {
            if (i1 == n1 || i2 == n2) {
                break;
            }
            if (refs1[i1] > refs2[i2]) {
                i2 += 1;
            } else if (refs1[i1] < refs2[i2]) {
                i1 += 1;
            } else {
                if (j == dst->nRows) {
                    dst = dst->next;
                    j = 0;
                }
                n += 1;
                dst->rows[j++] = refs1[i1];
                i1 += 1;
                i2 += 1;
            }
        }
    }
    dst->nRows = j;
    nRows = n;
    segment* next = dst->next;
    first.prev = dst;
    dst->next = &first;
    while (next != &first) {
        dst = next;
        next = dst->next;
        delete dst;
    }
}


void dbSelection::sort(dbDatabase* db, dbOrderByNode* order, bool caseInsensitive, dbSortResult* sortResult)
{
    int i = 0, j, k, n = (int)nRows;
    dbSortRecord* keys = new dbSortRecord[n];
    char* rawKeys = NULL;
    segment* seg = &first;
    bool partialOrder = false;

    TRACE_MSG((STRLITERAL("Sort %d records\n"), n));
        
    if (order->expr != NULL) { 
        dbSynthesizedAttribute result;
        switch (order->expr->type) { 
          case tpReal: 
            do {
                for (j = 0, k = (int)seg->nRows; j < k; j++, i++) {
                    db->evaluate(order->expr, seg->rows[j], order->table, result);
                    keys[i].u.realKey = result.fvalue;
                    keys[i].oid = seg->rows[j];
                }
            } while ((seg = seg->next) != &first);
            qsort(keys, n, sizeof(dbSortRecord), order->ascent ? cmpRealKey : cmpRealKeyDesc);
            break;
          case tpInteger:
            do {
                for (j = 0, k = (int)seg->nRows; j < k; j++, i++) {
                    db->evaluate(order->expr, seg->rows[j], order->table, result);
                    keys[i].u.longKey = result.ivalue;
                    keys[i].oid = seg->rows[j];
                }
            } while ((seg = seg->next) != &first);
            qsort(keys, n, sizeof(dbSortRecord), order->ascent ? cmpIntKey : cmpIntKeyDesc);
            break;
          case tpBoolean:
            do {
                for (j = 0, k = (int)seg->nRows; j < k; j++, i++) {
                    db->evaluate(order->expr, seg->rows[j], order->table, result);
                    keys[i].u.intKey = result.bvalue;
                    keys[i].oid = seg->rows[j];
                }
            } while ((seg = seg->next) != &first);
            qsort(keys, n, sizeof(dbSortRecord), order->ascent ? cmpIntKey : cmpIntKeyDesc);
            break;
          case tpReference:
            do {
                for (j = 0, k = (int)seg->nRows; j < k; j++, i++) {
                    db->evaluate(order->expr, seg->rows[j], order->table, result);
                    keys[i].u.longKey = result.oid;
                    keys[i].oid = seg->rows[j];
                }
            } while ((seg = seg->next) != &first);
            qsort(keys, n, sizeof(dbSortRecord), order->ascent ? cmpLongKey : cmpLongKeyDesc);
            break;
          case tpString:
            if (sortResult != NULL) { 
                do {
                    for (j = 0, k = (int)seg->nRows; j < k; j++, i++) {
                        char_t buf[dbBtreePage::dbMaxKeyLen];
                        int len = (int)db->evaluateString(order->expr, seg->rows[j], order->table, buf, itemsof(buf));
                        assert(len <= dbBtreePage::dbMaxKeyLen);
                        keys[i].u.strKey = sortResult->strBuf.put(buf, len);
                        if (caseInsensitive) { 
                            strlower(keys[i].u.strKey, keys[i].u.strKey);
                        }
                        keys[i].oid = seg->rows[j];
                    }
                } while ((seg = seg->next) != &first);
                qsort(keys, n, sizeof(dbSortRecord), order->ascent ? cmpStrKey : cmpStrKeyDesc);
            } else { 
                do {
                    for (j = 0, k = (int)seg->nRows; j < k; j++, i++) {
                        char_t buf[8/sizeof(char_t)];
                        db->evaluateString(order->expr, seg->rows[j], order->table, buf, itemsof(buf));
                        keys[i].u.longKey = packStrKey((byte*)buf, caseInsensitive);
                        keys[i].oid = seg->rows[j];
                    }
                } while ((seg = seg->next) != &first);
                qsort(keys, n, sizeof(dbSortRecord), order->ascent ? cmpLongKey : cmpLongKeyDesc);                
                partialOrder = true;
            }
            break;
          default:
            assert(false);      
        }
    } else { 
        int offs = order->field->dbsOffs;
        dbGetTie tie;
        byte* p;

        memset(keys, 0, n*sizeof(dbSortRecord));

        switch (order->field->type) {
          case dbField::tpBool:
          case dbField::tpInt1:
            do {
                for (j = 0, k = (int)seg->nRows; j < k; j++, i++) {
                    p = (byte*)db->getRow(tie, seg->rows[j]);
                    keys[i].u.intKey = *(int1*)(p + offs);
                    keys[i].oid = seg->rows[j];
                }
            } while ((seg = seg->next) != &first);
            qsort(keys, n, sizeof(dbSortRecord), order->ascent ? cmpIntKey : cmpIntKeyDesc);
            break;
          case dbField::tpInt2:
            do {
                for (j = 0, k = (int)seg->nRows; j < k; j++, i++) {
                    p = (byte*)db->getRow(tie, seg->rows[j]);
                    keys[i].u.intKey = *(int2*)(p + offs);
                    keys[i].oid = seg->rows[j];
                }
            } while ((seg = seg->next) != &first);
            qsort(keys, n, sizeof(dbSortRecord), order->ascent ? cmpIntKey : cmpIntKeyDesc);
            break;
          case dbField::tpReal4:
            do {
                for (j = 0, k = (int)seg->nRows; j < k; j++, i++) {
                    p = (byte*)db->getRow(tie, seg->rows[j]);
                    keys[i].u.realKey = *(float*)(p + offs);
                    keys[i].oid = seg->rows[j];
                }
            } while ((seg = seg->next) != &first);
            qsort(keys, n, sizeof(dbSortRecord), order->ascent ? cmpRealKey : cmpRealKeyDesc);
            break;
          case dbField::tpInt4:
          case dbField::tpArray:
            do {
                for (j = 0, k = (int)seg->nRows; j < k; j++, i++) {
                    p = (byte*)db->getRow(tie, seg->rows[j]);
                    keys[i].u.intKey = *(int4*)(p + offs);
                    keys[i].oid = seg->rows[j];
                }
            } while ((seg = seg->next) != &first);
            qsort(keys, n, sizeof(dbSortRecord), order->ascent ? cmpIntKey : cmpIntKeyDesc);
            break;
          case dbField::tpInt8:
            do {
                for (j = 0, k = (int)seg->nRows; j < k; j++, i++) {
                    p = (byte*)db->getRow(tie, seg->rows[j]);
                    keys[i].u.longKey = *(db_int8*)(p + offs);
                    keys[i].oid = seg->rows[j];
                }
            } while ((seg = seg->next) != &first);
            qsort(keys, n, sizeof(dbSortRecord), order->ascent ? cmpLongKey : cmpLongKeyDesc);
            break;
          case dbField::tpReference:
            do {
                for (j = 0, k = (int)seg->nRows; j < k; j++, i++) {
                    p = (byte*)db->getRow(tie, seg->rows[j]);
                    keys[i].u.longKey = *(oid_t*)(p + offs);
                    keys[i].oid = seg->rows[j];
                }
            } while ((seg = seg->next) != &first);
            qsort(keys, n, sizeof(dbSortRecord), order->ascent ? cmpLongKey : cmpLongKeyDesc);
            break;
          case dbField::tpReal8:
            do {
                for (j = 0, k = (int)seg->nRows; j < k; j++, i++) {
                    p = (byte*)db->getRow(tie, seg->rows[j]);
                    keys[i].u.realKey = *(real8*)(p + offs);
                    keys[i].oid = seg->rows[j];
                }
            } while ((seg = seg->next) != &first);
            qsort(keys, n, sizeof(dbSortRecord), order->ascent ? cmpRealKey : cmpRealKeyDesc);
            break;
          case dbField::tpString:
            if (sortResult != NULL) { 
                do {
                    for (j = 0, k = (int)seg->nRows; j < k; j++, i++) {
                        byte* p = (byte*)db->getRow(tie, seg->rows[j]);
                        keys[i].u.strKey = sortResult->strBuf.put((char_t*)(p + ((dbVarying*)(p + offs))->offs), 
                                                                  ((dbVarying*)(p + offs))->size-1);
                        if (caseInsensitive) { 
                            strlower(keys[i].u.strKey, keys[i].u.strKey);
                        }
                        keys[i].oid = seg->rows[j];
                    }
                } while ((seg = seg->next) != &first);
                qsort(keys, n, sizeof(dbSortRecord), order->ascent ? cmpStrKey : cmpStrKeyDesc);
            } else { 
                do {
                    for (j = 0, k = (int)seg->nRows; j < k; j++, i++) {
                        byte* p = (byte*)db->getRow(tie, seg->rows[j]);
                        keys[i].u.longKey = packStrKey(p + ((dbVarying*)(p + offs))->offs, caseInsensitive);
                        keys[i].oid = seg->rows[j];
                    }
                } while ((seg = seg->next) != &first);
                qsort(keys, n, sizeof(dbSortRecord), order->ascent ? cmpLongKey : cmpLongKeyDesc);
                partialOrder = true;
            }
            break;
          case dbField::tpRawBinary:
          { 
            int rawSize = (int)order->field->dbsSize;
            rawKeys = new char[n*rawSize];
            dbSortContext ctx;
            ctx.db = db;
            ctx.order = order;
            sortThreadContext.set(&ctx);
            do {
                for (j = 0, k = (int)seg->nRows; j < k; j++, i++) {
                    p = (byte*)db->getRow(tie, seg->rows[j]);
                    keys[i].oid = seg->rows[j];
                    keys[i].u.rawKey = rawKeys + i*rawSize;
                    memcpy(keys[i].u.rawKey, p + offs, rawSize);
                }
            } while ((seg = seg->next) != &first);
            qsort(keys, n, sizeof(dbSortRecord), udtComparator);
            break;
          }               
        default:
          assert(false);
        }
    }
    if (order->next != NULL || partialOrder) {
        dbSortContext ctx;
        ctx.db = db;
        ctx.order = partialOrder ? order : order->next;
        sortThreadContext.set(&ctx);
        if (rawKeys != NULL) { 
            for (i = 0, k = 0; i < n; i = j) {
                for (j = i+1; j < n && order->field->comparator(keys[j].u.rawKey, 
                                                                keys[i].u.rawKey, 
                                                                order->field->dbsSize) == 0; j++);
                if (j > i + 1) {
                    qsort(keys + i, j - i, sizeof(dbSortRecord), exactKeyCmp);
                }
            }
        } else { 
            for (i = 0, k = 0; i < n; i = j) {
                for (j = i+1; j < n && keys[j].u.longKey == keys[i].u.longKey; j++);
                if (j > i + 1) {
                    qsort(keys + i, j - i, sizeof(dbSortRecord), exactKeyCmp);
                }
            }
        }
    }
    if (sortResult != NULL) { 
        sortResult->keys = keys;
        sortResult->rawKeys = rawKeys;        
    } else { 
        if (n != 0) {
            for (i = 0, j = 0, seg = &first, k = (int)seg->nRows; i < n; i++, j++) {
                if (j == k) {
                    seg = seg->next;
                    k = (int)seg->nRows;
                    j = 0;
                }
                seg->rows[j] = keys[i].oid;
            }
        }
        delete[] keys;
        delete[] rawKeys;
    }
}
    
dbAnyCursor::dbAnyCursor(dbTableDescriptor& aTable, dbCursorType aType, byte* rec)
: table(&aTable),type(aType),defaultType(aType),
  allRecords(false),currId(0),record(rec)
{
    limit = dbDefaultSelectionLimit;
    prefetch = rec != NULL;
    removed = false;
    eliminateDuplicates = false;
    checkForDuplicatedIsEnabled = true;
    db = aTable.db;
    paramBase = NULL;
    stmtLimitLen = dbDefaultSelectionLimit;
    stmtLimitStart = 0;
    nSkipped = 0; 
    iterator = NULL;
}

dbAnyCursor::dbAnyCursor(dbCursorType aType) 
: table(NULL),type(aType),defaultType(aType),
  allRecords(false),currId(0),record(NULL)
{
    limit = dbDefaultSelectionLimit;
    prefetch = false;
    removed = false;
    eliminateDuplicates = false;
    checkForDuplicatedIsEnabled = true;
    db = NULL;
    paramBase = NULL;
    stmtLimitLen = dbDefaultSelectionLimit;
    stmtLimitStart = 0;
    nSkipped = 0;
    iterator = NULL;
}

int dbAnyCursor::select(dbQuery& query, dbCursorType aType, void* paramStruct) 
{
    paramBase = paramStruct;
    type = aType;
    reset();
    db->select(this, query);
    paramBase = NULL;
    if (gotoFirst() && prefetch) {
        fetch();
    }
    if (aType == dbCursorDetached) { 
        unlink();    
        db->commit();
    }
    return (int)selection.nRows;
}

oid_t* dbAnyCursor::toArrayOfOid(oid_t* arr) const
{ 
    assert(iterator == NULL); // do not use incremental cursor if you need to get array of OIDs
    if (arr == NULL) { 
        arr = new oid_t[selection.nRows];
    }
    if (allRecords) { 
        dbRecord rec;
        oid_t* oids = arr;
        for (oid_t oid = firstId; oid != 0; oid = rec.next) { 
             db->getHeader(rec, oid);
             *oids++ = oid;
        }
    } else { 
        selection.toArray(arr);
    }
    return arr;
}
void dbAnyCursor::setCurrent(dbAnyReference const& ref)
{
    removed = false;
    assert(ref.oid != 0);
    reset();
    db->beginTransaction(type == dbCursorForUpdate ? dbUpdateLock : dbSharedLock);
    db->threadContext.get()->cursors.link(this);
    currId = ref.oid;
    selection.first.nRows = 1;
    selection.first.rows[0] = currId;
    selection.nRows = 1;
    if (prefetch) {
        fetch();
    }
}

int dbAnyCursor::selectByKey(char_t const* key, void const* value)
{
    dbFieldDescriptor* field = table->find(key);
    assert(field != NULL);
    return selectByKey(field, value);
}

int dbAnyCursor::selectByKey(dbFieldDescriptor* field, void const* value)
{
    assert(field->hashTable != 0 || field->bTree != 0);
    reset();
    db->beginTransaction(type == dbCursorForUpdate ? dbUpdateLock : dbSharedLock);
    db->threadContext.get()->cursors.link(this);
    dbSearchContext sc;
    sc.db = db;
    sc.probes = 0;
    sc.ascent = true;
    sc.offs = field->dbsOffs;
    sc.cursor = this;
    sc.tmpKeys = false;
    sc.condition = NULL;
    sc.prefixLength = 0;
    sc.spatialSearch = false;
    sc.arraySearch = false;
    sc.firstKey = sc.lastKey = (char_t*)value;
    sc.firstKeyInclusion = sc.lastKeyInclusion = true;
    if (type == dbCursorIncremental) { 
        btreeIterator.init(db, field->bTree, sc, field->comparator);
        iterator = &btreeIterator;
    } else {
        dbBtree::find(db, field->bTree, sc, field->comparator);
    }
    if (gotoFirst() && prefetch) {
        fetch();
    }
    return (int)selection.nRows;
}

int dbAnyCursor::seek(oid_t oid)
{
    int pos = 0;
    if (gotoFirst()) { 
        do { 
            if (currId == oid) { 
                if (prefetch) { 
                    fetch();
                }
                return pos;
            }
            pos += 1;
        } while (gotoNext());
    }
    return -1;
}

bool dbAnyCursor::skip(int n) { 
    while (n > 0) { 
        if (!gotoNext()) { 
            return false;
        }
        n -= 1;
    } 
    while (n < 0) { 
        if (!gotoPrev()) { 
            return false;
        }
        n += 1;
    } 
    if (prefetch) { 
        fetch();
    }
    return true;
}

int dbAnyCursor::selectByKeyRange(char_t const* key, void const* minValue, 
                                  void const* maxValue, bool ascent)
{
    dbFieldDescriptor* field = table->find(key);
    assert(field != NULL);
    return selectByKeyRange(field, minValue, maxValue, ascent);
}

int dbAnyCursor::selectByKeyRange(dbFieldDescriptor* field, void const* minValue, 
                                  void const* maxValue, bool ascent)
{
    assert(field->bTree != 0);
    reset();
    db->beginTransaction(type == dbCursorForUpdate ? dbUpdateLock : dbSharedLock);
    db->threadContext.get()->cursors.link(this);
    dbSearchContext sc;
    sc.db = db;
    sc.probes = 0;
    sc.offs = field->dbsOffs;
    sc.cursor = this;
    sc.tmpKeys = false;
    sc.ascent = ascent;
    sc.condition = NULL;
    sc.prefixLength = 0;
    sc.firstKey = (char_t*)minValue;
    sc.lastKey = (char_t*)maxValue;
    sc.firstKeyInclusion = sc.lastKeyInclusion = true;
    sc.spatialSearch = false;
    sc.arraySearch = false;
    if (type == dbCursorIncremental) { 
        btreeIterator.init(db, field->bTree, sc, field->comparator);
        iterator = &btreeIterator;
    } else { 
        dbBtree::find(db, field->bTree, sc, field->comparator);
    }
    if (gotoFirst() && prefetch) {
        fetch();
    }
    return (int)selection.nRows;
}

void dbAnyCursor::remove()
{
    oid_t removedId = currId;
    lastRecordWasDeleted = false;
    if (type != dbCursorForUpdate) {
        db->handleError(dbDatabase::CursorError, "Readonly cursor");
    }
    if (removedId == 0) {
        db->handleError(dbDatabase::CursorError, "No current record");
    }
    if (allRecords) {
        dbRecord rec;
        db->getHeader(rec, removedId);
        if (rec.next != 0) {
            if (removedId == firstId) {
                firstId = currId = rec.next;
            } else {
                currId = rec.next;
            }
        } else {
            lastRecordWasDeleted = true;
            if (removedId == firstId) {
                firstId = lastId = currId = 0;
            } else {
                lastId = currId = rec.prev;
            }
        }
    } else {
        if (selection.curr != NULL) {
            if (--selection.curr->nRows == 0 || selection.pos == selection.curr->nRows) {
                dbSelection::segment* s = selection.curr, *next= s->next;;
                if (selection.curr->nRows == 0 && s != &selection.first) {
                    delete s;
                }
                if (next != &selection.first) {
                    selection.curr = next;
                    selection.pos = 0;
                } else {
                    lastRecordWasDeleted = true;
                    selection.curr = next->prev;
                    selection.pos = selection.curr->nRows-1;
                }
                if (selection.curr->nRows != 0) {
                    currId = selection.curr->rows[selection.pos];
                } else {
                    currId = 0;
                }
            } else {
                memcpy(selection.curr->rows + selection.pos,
                       selection.curr->rows + selection.pos + 1,
                       (selection.curr->nRows - selection.pos)
                       *sizeof(oid_t));
                currId = selection.curr->rows[selection.pos];
            }
        } else {
            currId = 0;
        }
    }
    byte* saveRecord = record;
    record = NULL;
    db->remove(table, removedId);
    record = saveRecord;
    removed = true;
    if (currId != 0 && prefetch) {
        fetch();
    }
}


void dbAnyCursor::removeAllSelected()
{
    if (type != dbCursorForUpdate) {
        db->handleError(dbDatabase::CursorError, "Readonly cursor");
    }
    byte* saveRecord = record;
    record = NULL;
    if (allRecords) {
        removeAll();
    } else if (selection.nRows != 0) {
        dbSelection::segment* curr = &selection.first;
        currId = 0;
        do {
            for (int i = 0, n = (int)curr->nRows; i < n; i++) {
                db->remove(table, curr->rows[i]);
            }
        } while ((curr = curr->next) != &selection.first);
        reset();
    } else if (currId != 0) {
        db->remove(table, currId);
        currId = 0;
    }
    record = saveRecord;
}

bool dbAnyCursor::isLast() const
{ 
    if (iterator != NULL) { 
        if (currId != 0) { 
            if (!iterator->next()) {
                return true;
            }
            iterator->prev();
        }
    } else if (allRecords) { 
        if (currId != 0) { 
            dbRecord rec;
            db->getHeader(rec, currId);
            return rec.next == 0;
        }
    } else if (selection.curr != NULL) { 
        return selection.pos+1 == selection.curr->nRows 
            && selection.curr->next == &selection.first; 
    }  
    return false;
}


bool dbAnyCursor::isFirst() const
{ 
    if (iterator != NULL) { 
        if (currId != 0) { 
            if (!iterator->prev()) {
                return true;
            }
            iterator->next();
        }
    } else if (allRecords) { 
        if (currId != 0) { 
            dbRecord rec;
            db->getHeader(rec, currId);
            return rec.prev == 0;
        }
    } else if (selection.curr != NULL) { 
        return selection.pos == 0 && selection.curr == &selection.first; 
    }  
    return false;
}


bool dbAnyCursor::hasNext() const
{ 
    if (iterator != NULL) { 
        if (currId != 0 && iterator->next()) {
            iterator->prev();
            return true;
        }
    } else if (allRecords) {
        if (currId != 0) {
            dbRecord rec;
            db->getHeader(rec, currId);
            return rec.next != 0;
        }
    } else if (selection.curr != NULL 
               && (selection.pos+1 < selection.curr->nRows || selection.curr->next != &selection.first))
    {
        return true;
    }
    return false;
}

byte* dbAnyCursor::fetchNext()
{
    byte* result = NULL;
    if (type == dbCursorDetached) { 
        db->beginTransaction(dbSharedLock);
        db->threadContext.get()->cursors.link(this);
        assert(!removed);
        while (gotoNext()) {
            if (db->isValidOid(currId)) { 
                fetch();
                result = record;
                break;
            }
        }
        unlink();    
        db->commit();
    } else { 
        if (!removed) {
            if (gotoNext()) {
                fetch();
                result = record;
            }
        } else { 
            removed = false; 
            if (currId != 0 && !lastRecordWasDeleted) { 
                if (!prefetch) { 
                    fetch();
                }
                result = record;
            }
        }
    }
    return result;  
}

byte* dbAnyCursor::fetchPrev()
{
    if (type == dbCursorDetached) { 
        db->beginTransaction(dbSharedLock);
        db->threadContext.get()->cursors.link(this);
        assert(!removed);
        byte* result = NULL;
        while (gotoPrev()) {            
            if (db->isValidOid(currId)) { 
                fetch();
                result = record;
                break;
            }
        }
        unlink();    
        db->commit();
        return result;
    } else { 
        if (removed) {
            removed = false; 
            if (lastRecordWasDeleted) { 
                if (currId != 0) { 
                    if (!prefetch) { 
                        fetch();
                    }
                    return record;
                }
                return NULL;
            }
        }
        if (gotoPrev()) {
            fetch();
            return record;
        }
        return NULL;
    }
}

byte* dbAnyCursor::fetchFirst()
{
    byte* result = NULL;

    if (type == dbCursorDetached) { 
        db->beginTransaction(dbSharedLock);
        db->threadContext.get()->cursors.link(this);

        if (gotoFirst()) {
            do { 
                if (db->isValidOid(currId)) {                 
                    fetch();
                    result = record;
                    break;
                }
            } while (gotoNext());
        }
        unlink();    
        db->commit();
    } else { 
        if (gotoFirst()) {
            fetch();
            result = record;
        }
    }
    return result;
}

byte* dbAnyCursor::fetchLast()
{
    byte* result = NULL;

    if (type == dbCursorDetached) { 
        db->beginTransaction(dbSharedLock);
        db->threadContext.get()->cursors.link(this);

        if (gotoLast()) {
            do { 
                if (db->isValidOid(currId)) {                 
                    fetch();
                    result = record;
                    break;
                }
            } while (gotoPrev());
        }
        unlink();    
        db->commit();
    } else { 
        if (gotoLast()) {
            fetch();
            result = record;
        }
    }
    return result;
}


bool dbAnyCursor::gotoNext()
{
    if (iterator != NULL) { 
        oid_t next = iterator->next();
        if (next != 0) { 
            currId = next;
            return true;
        }
        return false;
    } 
    removed = false;
    if (allRecords) {
        if (currId != 0) {
            dbRecord rec;
            db->getHeader(rec, currId);
            if (rec.next != 0) {
                currId = rec.next;
                return true;
            }
        }
    } else if (selection.curr != NULL) {
        if (++selection.pos == selection.curr->nRows) {
            if (selection.curr->next == &selection.first) {
                selection.pos -= 1;
                return false;
            }
            selection.pos = 0;
            selection.curr = selection.curr->next;
        }
        currId = selection.curr->rows[selection.pos];
        return true;
    }
    return false;
}

bool dbAnyCursor::gotoPrev()
{
    if (iterator != NULL) { 
        oid_t prev = iterator->prev();
        if (prev != 0) { 
            currId = prev;
            return true;
        }
        return false;
    } 
    removed = false;
    if (allRecords) {
        if (currId != 0) {
            dbRecord rec;
            db->getHeader(rec, currId);
            if (rec.prev != 0) {
                currId = rec.prev;
                return true;
            }
        }
    } else if (selection.curr != NULL) {
        if (selection.pos == 0) {
            if (selection.curr == &selection.first || selection.curr->prev->nRows == 0) {
                return false;
            }
            selection.curr = selection.curr->prev;
            selection.pos = selection.curr->nRows;
        }
        currId = selection.curr->rows[--selection.pos];
        return true;
    }
    return false;
}


bool dbAnyCursor::moveNext() 
{
    if (!removed) { 
        return gotoNext(); 
    } else {
        removed = false;
        return !lastRecordWasDeleted;
    }
}
    
bool dbAnyCursor::movePrev() 
{
    if (!removed) { 
        return gotoPrev(); 
    } else {
        removed = false;
        return lastRecordWasDeleted ? (currId != 0) : gotoPrev();
    }
}
 
bool dbAnyCursor::gotoFirst()
{
    if (iterator != NULL) { 
        if ((currId = iterator->first()) != 0) { 
            selection.nRows = 1;
            return true;
        }
        return false;
    } 
    removed = false;
    if (allRecords) {
        currId = firstId;
        return (currId != 0);
    } else {
        selection.curr = selection.first.nRows == 0 ? selection.first.next : &selection.first;
        selection.pos = 0;
        if (selection.curr->nRows == 0) {
            return (currId != 0);
        } else {
            currId = selection.curr->rows[0];
            return true;
        }
    }
}

bool dbAnyCursor::gotoLast()
{
    if (iterator != NULL) { 
        return (currId = iterator->last()) != 0;
    } 
    removed = false;
    if (allRecords) {
        currId = lastId;
        return (currId != 0);
    } else {
        selection.curr = selection.first.prev;
        if (selection.curr->nRows == 0) {
            return (currId != 0);
        } else {
            selection.pos = selection.curr->nRows-1;
            currId = selection.curr->rows[selection.pos];
            return true;
        }
    }
}

bool dbAnyCursor::isInSelection(oid_t oid) 
{
    assert(iterator == NULL);
    if (eliminateDuplicates) { 
        return isMarked(oid);
    } else if (selection.nRows > dbSelection::buildSelectionBitmapThreshold) {
        checkForDuplicates();
        dbSelection::segment* curr = &selection.first;
        do {
            for (int i = 0, n = (int)curr->nRows; i < n; i++) {
                oid_t o = curr->rows[i];
                selection.bitmap[(size_t)o >> 5] |= 1 << ((int)o & 31);
            }
        } while ((curr = curr->next) != &selection.first);
        return isMarked(oid);
    } else { 
        dbSelection::segment* curr = &selection.first;
        do {
            for (int i = 0, n = (int)curr->nRows; i < n; i++) {
                if (curr->rows[i] == oid) { 
                    return true;
                }
            }
        } while ((curr = curr->next) != &selection.first);
        return false;
    }
}

void dbAnyCursor::reset()
{
    if (db == NULL) { 
        db = table->db;
        assert(((void)"cursor associated with online database table",
                table->tableId != 0));
    } else if (table->db != db) { 
        table = db->lookupTable(table);
    }
    unlink();
    selection.reset();
    eliminateDuplicates = false;
    allRecords = false;
    removed = false;
    currId = 0;
    tie.reset();
    stmtLimitLen = dbDefaultSelectionLimit;
    stmtLimitStart = 0;
    nSkipped = 0;
    iterator = NULL;
}

void dbAnyCursor::freeze()
{
    assert(type != dbCursorIncremental && type != dbCursorDetached);
    unlink();    
    tie.reset();
}

void dbAnyCursor::unfreeze()
{
    db->beginTransaction(type == dbCursorForUpdate ? dbUpdateLock : dbSharedLock);
    db->threadContext.get()->cursors.link(this);
    if (currId != 0 && prefetch) {
        fetch();
    }
}

dbAnyCursor::~dbAnyCursor()
{
    selection.reset();
}

void dbParallelQueryContext::search(int i)
{
    int nThreads = db->parThreads;
    oid_t oid = firstRow;
    dbRecord rec;
    int j;
    for (j = i; --j >= 0;) {
        db->getHeader(rec, oid);
        oid = rec.next;
    }
    while (oid != 0) {
        if (db->evaluateBoolean(query->tree, oid, table, cursor)) {
            selection[i].add(oid);
        }
        db->getHeader(rec, oid);
        oid = rec.next;
        for (j = nThreads; --j > 0 && oid != 0;) {
            db->getHeader(rec, oid);
            oid = rec.next;
        }
    }
    if (query->order != NULL) {
        selection[i].sort(db, query->order);
    }
}

END_GIGABASE_NAMESPACE







