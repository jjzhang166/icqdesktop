//-< SERVER.CPP >----------------------------------------------------*--------*
// GigaBASE                  Version 1.0         (c) 1999  GARRET    *     ?  *
// (Post Relational Database Management System)                      *   /\|  *
//                                                                   *  /  \  *
//                          Created:     13-Jan-2000 K.A. Knizhnik   * / [] \ *
//                          Last update: 13-Jan-2000 K.A. Knizhnik   * GARRET *
//-------------------------------------------------------------------*--------*
// CLI multithreaded server implementation
//-------------------------------------------------------------------*--------*

#define INSIDE_GIGABASE

#include "gigabase.h"
#include "compiler.h"
#include "wwwapi.h"
#include "subsql.h"
#include "symtab.h"
#include "hashtab.h"
#include "btree.h"
#include "rtree.h"
#include "cli.h"
#include "cliproto.h"
#include "server.h"
#include "localcli.h"

#if !defined(_WIN32) && defined(NO_PTHREADS)
#error Server requires multithreading support
#endif

BEGIN_GIGABASE_NAMESPACE

#ifdef SECURE_SERVER
REGISTER(UserInfo);
#endif

int dbColumnBinding::unpackArray(char* dst, size_t& offs)
{
    int len = this->len;
    int i;
    switch (cliType) { 
      case cli_array_of_string:
        { 
            dbVarying* hdr = (dbVarying*)(dst + offs);
            char_t* body = (char_t*)(hdr + len);
            char* p = ptr + 4;
            int relOffs = sizeof(dbVarying)*len;
            offs += relOffs;
            for (i = 0; i < len; i++) { 
                char* p1 = unpack_str(body, p);         
                int strlen = (int)((p1 - p) / sizeof(char_t));
                hdr->size = strlen;
                hdr->offs = relOffs;
                body += strlen;
                relOffs += strlen*sizeof(char_t) - sizeof(dbVarying);
                p = p1;
                hdr += 1;
            }
            offs += relOffs;
        }
        break;
      case cli_array_of_decimal:
        {
            char* p = ptr + 4;
            for (i = 0; i < len; i++) { 
                double val = 0.0;
                sscanf(p, "%lf", &val);
                p += strlen(p) + 1;
                switch (fd->components->type) {
                  case dbField::tpInt1:
                    *(dst + offs) = (int1)val;
                    offs += sizeof(int1);
                    break;
                  case dbField::tpInt2:
                    *(int2*)(dst + offs) = (int2)val;
                    offs += sizeof(int2);
                    break;
                  case dbField::tpInt4:
                    *(int4*)(dst + offs) = (int4)val;
                    offs += sizeof(int4);
                    break;
                  case dbField::tpInt8:
                    *(db_int8*)(dst + offs) = (db_int8)val;
                    offs += sizeof(db_int8);
                    break;
                  case dbField::tpReal4:
                    *(real4*)(dst + offs) = (real4)val;
                    offs += sizeof(real4);
                    break;
                  case dbField::tpReal8:
                    *(real8*)(dst + offs) = val;
                    offs += sizeof(real8);
                    break;
                }
            }
        }
        break;
      case cli_cstring:
        unpack_str((char_t*)(dst + offs), ptr + 4, len-1);
        offs += len*sizeof(char_t);
        *((char_t*)(dst + offs - sizeof(char_t))) = '\0';
        break;
      case cli_asciiz:
      case cli_pasciiz:
        unpack_str((char_t*)(dst + offs), ptr + 4, len);
        offs += len*sizeof(char_t);
        break;
      default:
        switch (sizeof_type[cliType - cli_array_of_oid]) {
          case 1:
            memcpy(dst + offs, ptr + 4, len);
            break;
          case 2:
            for (i = 0; i < len; i++) {
                unpack2(dst + offs + i*2, ptr + 4 + i*2);
            }
            break;
          case 4:
            for (i = 0; i < len; i++) {
                unpack4(dst + offs + i*4, ptr + 4 + i*4);
            }
            break;
          case 8:
            for (i = 0; i < len; i++) {
                unpack8(dst + offs + i*8, ptr + 4 + i*8);
            }
            break;
          default:
            assert(false);
        }
        offs += len*sizeof_type[cliType - cli_array_of_oid];
    }
    return len;
}

void dbColumnBinding::unpackScalar(char* dst, bool insert)
{
    if (cliType == cli_decimal) { 
        double val;
        sscanf(ptr, "%lf", &val);
        switch (fd->type) {
          case dbField::tpInt1:
            *(dst + fd->dbsOffs) = (int1)val;
            break;
          case dbField::tpInt2:
            *(int2*)(dst + fd->dbsOffs) = (int2)val;
            break;
          case dbField::tpInt4:
            *(int4*)(dst + fd->dbsOffs) = (int4)val;
            break;
          case dbField::tpInt8:
            *(db_int8*)(dst + fd->dbsOffs) = (db_int8)val;
            break;
          case dbField::tpReal4:
            *(real4*)(dst + fd->dbsOffs) = (real4)val;
            break;
          case dbField::tpReal8:
            *(real8*)(dst + fd->dbsOffs) = val;
            break;
        }
        return;
    } else if (cliType == cli_autoincrement) { 
        assert(fd->type == dbField::tpInt4);
        if (insert) { 
#ifdef AUTOINCREMENT_SUPPORT
            *(int4*)(dst+fd->dbsOffs) = fd->defTable->autoincrementCount;
#else
            *(int4*)(dst+fd->dbsOffs) = fd->defTable->nRows;
#endif
        }
        return;
    } 
    switch (fd->type) {
      case dbField::tpReference:
        *(oid_t*)(dst + fd->dbsOffs) = unpack_oid(ptr);
         break;
      case dbField::tpRectangle:
        unpack_rectangle((cli_rectangle_t*)(dst + fd->dbsOffs), ptr);
        break;
      case dbField::tpBool:
      case dbField::tpInt1:
        switch (sizeof_type[cliType]) {
          case 1:
            *(dst + fd->dbsOffs) = *ptr;
            break;
          case 2:
            *(dst + fd->dbsOffs) = (char)unpack2(ptr);
            break;
          case 4:
            *(dst + fd->dbsOffs) = (char)unpack4(ptr);
            break;
          case 8:
            *(dst + fd->dbsOffs) = (char)unpack8(ptr);
            break;
          default:
            assert(false);
        }
        break;
      case dbField::tpInt2:
        switch (sizeof_type[cliType]) {
          case 1:
            *(int2*)(dst+fd->dbsOffs) = *ptr;
            break;
          case 2:
            unpack2(dst+fd->dbsOffs, ptr);
            break;
          case 4:
            *(int2*)(dst+fd->dbsOffs) = (int2)unpack4(ptr);
            break;
          case 8:
            *(int2*)(dst+fd->dbsOffs) = (int2)unpack8(ptr);
            break;
          default:
            assert(false);
        }
        break;
      case dbField::tpInt4:
        switch (sizeof_type[cliType]) {
          case 1:
            *(int4*)(dst+fd->dbsOffs) = *ptr;
            break;
          case 2:
            *(int4*)(dst+fd->dbsOffs) = unpack2(ptr);
            break;
          case 4:
            unpack4(dst+fd->dbsOffs, ptr);
            break;
          case 8:
            *(int4*)(dst+fd->dbsOffs) = (int4)unpack8(ptr);
            break;
          default:
            assert(false);
        }
        break;
      case dbField::tpInt8:
        switch (sizeof_type[cliType]) {
          case 1:
            *(db_int8*)(dst+fd->dbsOffs) = *ptr;
            break;
          case 2:
            *(db_int8*)(dst+fd->dbsOffs) = unpack2(ptr);
            break;
          case 4:
            *(db_int8*)(dst+fd->dbsOffs) = unpack4(ptr);
            break;
          case 8:
            unpack8(dst+fd->dbsOffs, ptr);
            break;
          default:
            assert(false);
        }
        break;
      case dbField::tpReal4:
        switch (cliType) {
          case cli_real4:
            unpack4(dst+fd->dbsOffs, ptr);
            break;
          case cli_real8:
            {
                real8 temp;
                unpack8((char*)&temp, ptr);
                *(real4*)(dst + fd->dbsOffs) = (real4)temp;
            }
            break;
          default:
            assert(false);
        }
        break;
      case dbField::tpReal8:
        switch (cliType) {
          case cli_real4:
            {
                real4 temp;
                unpack4((char*)&temp, ptr);
                *(real8*)(dst + fd->dbsOffs) = temp;
            }
            break;
          case cli_real8:
            unpack8(dst+fd->dbsOffs, ptr);
            break;
          default:
            assert(false);
        }
        break;
      case dbField::tpStructure:
        assert(cliType == cli_datetime);
        unpack4(dst+fd->dbsOffs, ptr);
        break;
      default:
        assert(false);
    }
}

void dbStatement::reset()
{
    dbColumnBinding *cb, *next;
    for (cb = columns; cb != NULL; cb = next) {
        next = cb->next;
        delete cb;
    }
    columns = NULL;
    delete[] params;
    params = NULL;
    delete cursor;
    cursor = NULL;
    query.reset();
    table = NULL;
}

dbServer* dbServer::chain;

inline dbStatement* dbServer::findStatement(dbClientSession* session, int stmt_id)
{
    for (dbStatement* stmt = session->stmts; stmt != NULL; stmt = stmt->next)
    {
        if (stmt->id == stmt_id) {
            return stmt;
        }
    }
    return NULL;
}

void thread_proc dbServer::serverThread(void* arg)
{
    ((dbServer*)arg)->serveClient();
}

void thread_proc dbServer::acceptLocalThread(void* arg)
{
    dbServer* server = (dbServer*)arg;
    server->acceptConnection(server->localAcceptSock);
}

void thread_proc dbServer::acceptGlobalThread(void* arg)
{
    dbServer* server = (dbServer*)arg;
    server->acceptConnection(server->globalAcceptSock);
}

dbServer::dbServer(dbDatabase* db,
                   char_t const* serverURL,
                   int optimalNumberOfThreads,
                   int connectionQueueLen)
{
    next = chain;
    chain = this;
    this->db = db;
    this->optimalNumberOfThreads = optimalNumberOfThreads;
    this->connectionQueueLen = connectionQueueLen;
#ifdef UNICODE 
    char buf[256];
    wcstombs(buf, serverURL, sizeof buf);
    URL = new char_t[STRLEN(serverURL) + 1];
    STRCPY(URL, serverURL);
    address = new char[strlen(buf)+1];
    strcpy(address, buf);
#else
    URL = new char[strlen(serverURL) + 1];
    strcpy(URL, serverURL);
#endif
    globalAcceptSock = NULL;
    localAcceptSock = NULL;

    freeList = activeList = waitList = NULL;
    waitListLength = 0;
}

dbServer* dbServer::find(char_t const* URL)
{
    for (dbServer* server = chain; server != NULL; server = server->next) {
        if (STRCMP(URL, server->URL) == 0) {
            return server;
        }
    }
    return NULL;
}

void dbServer::cleanup()
{
    dbServer *server, *next;
    for (server = chain; server != NULL; server = next) {
        next = server->next;
        delete server;
    }
}

void dbServer::start()
{
    nActiveThreads = nIdleThreads = 0;
    cancelWait = cancelSession = cancelAccept = false;
    go.open();
    done.open();
#ifdef UNICODE
    globalAcceptSock = socket_t::create_global(address, connectionQueueLen);
#else
    globalAcceptSock = socket_t::create_global(URL, connectionQueueLen);
#endif
    if (!globalAcceptSock->is_ok()) {
        char_t errbuf[64];
        globalAcceptSock->get_error_text(errbuf, itemsof(errbuf));
        dbTrace(STRLITERAL("Failed to create global socket: %s\n"), errbuf);
        delete globalAcceptSock;
        globalAcceptSock = NULL;
    } else { 
        globalAcceptThread.create(acceptGlobalThread, this);
    }
#ifdef UNICODE
    localAcceptSock = socket_t::create_local(address, connectionQueueLen);
#else
    localAcceptSock = socket_t::create_local(URL, connectionQueueLen);
#endif
    if (!localAcceptSock->is_ok()) {
        char_t errbuf[64];
        localAcceptSock->get_error_text(errbuf, itemsof(errbuf));
        dbTrace(STRLITERAL("Failed to create local socket: %s\n"), errbuf);
        delete localAcceptSock;
        localAcceptSock = NULL;
    } else { 
        localAcceptThread.create(acceptLocalThread, this);
    }
}

void dbServer::stop()
{
    cancelAccept = true;
    if (globalAcceptSock != NULL) {
        globalAcceptSock->cancel_accept();
        globalAcceptThread.join();
        delete globalAcceptSock;
        globalAcceptSock = NULL;
    }
    if (localAcceptSock != NULL) {
        localAcceptSock->cancel_accept();
        localAcceptThread.join();
        delete localAcceptSock;
        localAcceptSock = NULL;
    }
    dbCriticalSection cs(mutex);
    cancelSession = true;
    while (activeList != NULL) {
        activeList->sock->shutdown();
        done.wait(mutex);
    }

    cancelWait = true;
    while (nIdleThreads != 0) {
        go.signal();
        done.wait(mutex);
    }

    while (waitList != NULL) {
        dbClientSession* next = waitList->next;
        delete waitList->sock;
        waitList->next = freeList;
        freeList = waitList;
        waitList = next;
    }
    waitListLength = 0;
    assert(nActiveThreads == 0);
    done.close();
    go.close();
}

bool dbServer::freeze(dbClientSession* session, int stmt_id)
{
    dbStatement* stmt = findStatement(session, stmt_id);
    int4 response = cli_ok;
    if (stmt == NULL || stmt->cursor == NULL) { 
        response = cli_bad_descriptor;
    } else { 
        stmt->cursor->freeze();
    }
    pack4(response);
    return session->sock->write(&response, sizeof response);
}
    
bool dbServer::unfreeze(dbClientSession* session, int stmt_id)
{
    dbStatement* stmt = findStatement(session, stmt_id);
    int4 response = cli_ok;
    if (stmt == NULL || stmt->cursor == NULL) { 
        response = cli_bad_descriptor;
    } else { 
        stmt->cursor->unfreeze();
    }
    pack4(response);
    return session->sock->write(&response, sizeof response);
}
    
bool dbServer::get_first(dbClientSession* session, int stmt_id)
{
    dbStatement* stmt = findStatement(session, stmt_id);
    int4 response;
    if (stmt == NULL || stmt->cursor == NULL) {
        response = cli_bad_descriptor;
    } else if (!stmt->cursor->gotoFirst()) {
        response = cli_not_found;
    } else {
        return fetch(session, stmt);
    }
    pack4(response);
    return session->sock->write(&response, sizeof response);
}

bool dbServer::get_last(dbClientSession* session, int stmt_id)
{
    dbStatement* stmt = findStatement(session, stmt_id);
    int4 response;
    if (stmt == NULL || stmt->cursor == NULL) {
        response = cli_bad_descriptor;
    } else if (!stmt->cursor->gotoLast()) {
        response = cli_not_found;
    } else {
        return fetch(session, stmt);
    }
    pack4(response);
    return session->sock->write(&response, sizeof response);
}

bool dbServer::get_next(dbClientSession* session, int stmt_id)
{
    dbStatement* stmt = findStatement(session, stmt_id);
    int4 response;
    if (stmt == NULL || stmt->cursor == NULL) {
        response = cli_bad_descriptor;
    }
    else if (!((stmt->firstFetch && stmt->cursor->gotoFirst()) ||
               (!stmt->firstFetch && stmt->cursor->gotoNext())))
    {
        response = cli_not_found;
    } else {
        return fetch(session, stmt);
    }
    pack4(response);
    return session->sock->write(&response, sizeof response);
}

bool dbServer::get_prev(dbClientSession* session, int stmt_id)
{
    dbStatement* stmt = findStatement(session, stmt_id);
    int4 response;
    if (stmt == NULL || stmt->cursor == NULL) {
        response = cli_bad_descriptor;
    }
    else if (!((stmt->firstFetch && stmt->cursor->gotoLast()) ||
               (!stmt->firstFetch && stmt->cursor->gotoPrev())))
    {
        response = cli_not_found;
    } else {
        return fetch(session, stmt);
    }
    pack4(response);
    return session->sock->write(&response, sizeof response);
}

bool dbServer::skip(dbClientSession* session, int stmt_id, char* buf)
{
    dbStatement* stmt = findStatement(session, stmt_id);
    int4 response;
    if (stmt == NULL || stmt->cursor == NULL) {
        response = cli_bad_descriptor;
    } else { 
        int n = unpack4(buf);
        if ((n > 0 && !((stmt->firstFetch && stmt->cursor->gotoFirst() && stmt->cursor->skip(n-1)
                         || (!stmt->firstFetch && stmt->cursor->skip(n)))))
            || (n < 0 && !((stmt->firstFetch && stmt->cursor->gotoLast() && stmt->cursor->skip(n+1)
                            || (!stmt->firstFetch && stmt->cursor->skip(n))))))
        {
            response = cli_not_found;
        } else {
            return fetch(session, stmt);
        }
    }
    pack4(response);
    return session->sock->write(&response, sizeof response);
}

bool dbServer::seek(dbClientSession* session, int stmt_id, char* buf)
{
    dbStatement* stmt = findStatement(session, stmt_id);
    int4 response;
    if (stmt == NULL || stmt->cursor == NULL) {
        response = cli_bad_descriptor;
    } else { 
        oid_t oid = unpack_oid(buf);
        int pos = stmt->cursor->seek(oid); 
        if (pos < 0) { 
            response = cli_not_found;
        } else { 
            return fetch(session, stmt, pos);
        }
    }
    pack4(response);
    return session->sock->write(&response, sizeof response);
}

bool dbServer::fetch(dbClientSession* session, dbStatement* stmt, oid_t result)
{
    int4 response;
    char buf[64];
    dbColumnBinding* cb;

    stmt->firstFetch = false;
    if (stmt->cursor->isEmpty()) {
        response = cli_not_found;
        pack4(response);
        return session->sock->write(&response, sizeof response);
    }
    int msg_size = sizeof(cli_oid_t) + 4;
    dbGetTie tie;
    char* data = (char*)db->getRow(tie, stmt->cursor->currId);
    char* src;
    for (cb = stmt->columns; cb != NULL; cb = cb->next) {
        src = data + cb->fd->dbsOffs;
        if (cb->cliType == cli_array_of_string) {
            int len = ((dbVarying*)src)->size;
            dbVarying* p = (dbVarying*)(data + ((dbVarying*)src)->offs);
            msg_size += 4;
            while (--len >= 0) { 
                msg_size += p->size*sizeof(char_t);
                p += 1;
            }
        } else if (cb->cliType == cli_array_of_decimal) { 
            int len = ((dbVarying*)src)->size;
            msg_size += 4;
            char* p = data + ((dbVarying*)src)->offs;
            while (--len >= 0) { 
                switch (cb->fd->components->type) {
                  case dbField::tpInt1:
                    sprintf(buf, "%d", *(int1*)p);
                    p += sizeof(int1);
                    break;
                  case dbField::tpInt2:
                    sprintf(buf, "%d", *(int2*)p);
                    p += sizeof(int2);
                    break;
                  case dbField::tpInt4:
                    sprintf(buf, "%d", *(int4*)p);
                    p += sizeof(int4);
                    break;
                  case dbField::tpInt8:
                    sprintf(buf, INT8_FORMAT, *(db_int8*)p);
                    p += sizeof(db_int8);
                    break;
                  case dbField::tpReal4:
                    sprintf(buf, "%.14g", *(real4*)p);
                    p += sizeof(real4);
                    break;
                  case dbField::tpReal8:
                    sprintf(buf, "%.14g", *(real8*)p);
                    p += sizeof(real8);
                    break;
                }
                msg_size += (int)(strlen(buf) + 1);
            }
        } else if (cb->cliType == cli_datetime || cb->cliType == cli_autoincrement) {
            msg_size += 4;
        } else if (cb->cliType >= cli_array_of_oid && cb->cliType <= cli_array_of_string) {
            msg_size += 4 + ((dbVarying*)src)->size
                            * sizeof_type[cb->cliType - cli_array_of_oid];
        } else if (cb->cliType == cli_decimal) {
            switch (cb->fd->type) {
              case dbField::tpInt1:
                sprintf(buf, "%d", *(int1*)src);
                break;
              case dbField::tpInt2:
                sprintf(buf, "%d", *(int2*)src);
                break;
              case dbField::tpInt4:
                sprintf(buf, "%d", *(int4*)src);
                break;
              case dbField::tpInt8:
                sprintf(buf, INT8_FORMAT, *(db_int8*)src);
                break;
              case dbField::tpReal4:
                sprintf(buf, "%.14g", *(real4*)src);
                break;
              case dbField::tpReal8:
                sprintf(buf, "%.14g", *(real8*)src);
                break;
            }
            msg_size += (int)(strlen(buf) + 1);
        } else if (cb->cliType >= cli_asciiz && cb->cliType <= cli_cstring) {
            int size = ((dbVarying*)src)->size;
            if (cb->cliType == cli_cstring && size != 0) { 
                size -= 1; // omit '\0'
            }
            msg_size += 4 + size*sizeof(char_t);
        } else {
            msg_size += sizeof_type[cb->cliType];
        }
        msg_size += 1; // column type
    }
    if (stmt->buf_size < msg_size) {
        delete[] stmt->buf;
        stmt->buf = new char[msg_size+1];
        stmt->buf_size = msg_size;
    }
    char* p = stmt->buf;
    p = pack4(p, msg_size);
    p = pack_oid(p, result);

    for (cb = stmt->columns; cb != NULL; cb = cb->next) {
        src = data + cb->fd->dbsOffs;
        *p++ = cb->cliType;
        if (cb->cliType == cli_decimal) {
            switch (cb->fd->type) {
              case dbField::tpInt1:
                p += sprintf(p, "%d", *(int1*)src);
                break;
              case dbField::tpInt2:
                p += sprintf(p, "%d", *(int2*)src);
                break;
              case dbField::tpInt4:
                p += sprintf(p, "%d", *(int4*)src);
                break;
              case dbField::tpInt8:
                p += sprintf(p, INT8_FORMAT, *(db_int8*)src);
                break;
              case dbField::tpReal4:
                p += sprintf(p, "%.14g", *(real4*)src);
                break;
              case dbField::tpReal8:
                p += sprintf(p, "%.14g", *(real8*)src);
                break;
            }
            p += 1;
        } else { 
            switch (cb->fd->type) {
              case dbField::tpBool:
              case dbField::tpInt1:
                switch (sizeof_type[cb->cliType]) {
                  case 1:
                    *p++ = *src;
                    break;
                  case 2:
                    p = pack2(p, (int2)*(char*)src);
                    break;
                  case 4:
                    p = pack4(p, (int4)*(char*)src);
                    break;
                  case 8:
                    p = pack8(p, (db_int8)*(char*)src);
                    break;
                  default:
                    assert(false);
                }
                break;
              case dbField::tpInt2:
                switch (sizeof_type[cb->cliType]) {
                  case 1:
                    *p++ = (char)*(int2*)src;
                    break;
                  case 2:
                    p = pack2(p, src);
                    break;
                  case 4:
                    p = pack4(p, (int4)*(int2*)src);
                    break;
                  case 8:
                    p = pack8(p, (db_int8)*(int2*)src);
                    break;
                  default:
                    assert(false);
                }
                break;
              case dbField::tpInt4:
                switch (sizeof_type[cb->cliType]) {
                  case 1:
                    *p++ = (char)*(int4*)src;
                    break;
                  case 2:
                    p = pack2(p, (int2)*(int4*)src);
                    break;
                  case 4:
                    p = pack4(p, src);
                    break;
                  case 8:
                    p = pack8(p, (db_int8)*(int4*)src);
                    break;
                  default:
                    assert(false);
                }
                break;
              case dbField::tpInt8:
                switch (sizeof_type[cb->cliType]) {
                  case 1:
                    *p++ = (char)*(db_int8*)src;
                    break;
                  case 2:
                    p = pack2(p, (int2)*(db_int8*)src);
                    break;
                  case 4:
                    p = pack4(p, (int4)*(db_int8*)src);
                    break;
                  case 8:
                    p = pack8(p, src);
                    break;
                  default:
                    assert(false);
                }
                break;
              case dbField::tpReal4:
                switch (cb->cliType) {
                  case cli_real4:
                    p = pack4(p, src);
                    break;
                  case cli_real8:
                  {
                    real8 temp = *(real4*)src;
                    p = pack8(p, (char*)&temp);
                    break;
                  }
                  default:
                    assert(false);
                }
                break;
              case dbField::tpReal8:
                switch (cb->cliType) {
                  case cli_real4:
                  {
                    real4 temp = (real4)*(real8*)src;
                    p = pack4(p, (char*)&temp);
                    break;
                  }
                  case cli_real8:
                    p = pack8(p, src);
                    break;
                  default:
                    assert(false);
                }
                break;
              case dbField::tpString:
              {
                dbVarying* v = (dbVarying*)src;
                int size = v->size;
                if (cb->cliType == cli_cstring && size != 0) { 
                    size -= 1;
                }
                p = pack4(p, size);
                p = pack_str(p, (char_t*)(data + v->offs), size);
                break;
              }
              case dbField::tpReference:
                p = pack_oid(p, *(oid_t*)src);
                break;
              case dbField::tpRectangle:
                p = pack_rectangle(p, (cli_rectangle_t*)src);
                break;
              case dbField::tpArray:
              {
                dbVarying* v = (dbVarying*)src;
                int n = v->size;
                p = pack4(p, n);
                src = data + v->offs;
                if (cb->cliType == cli_array_of_string) {
                    while (--n >= 0) {
                        p = pack_str(p, (char_t*)(src + ((dbVarying*)src)->offs));
                        src += sizeof(dbVarying);
                    }
                } else if (cb->cliType == cli_array_of_decimal) { 
                    while (--n >= 0) {
                        switch (cb->fd->components->type) {
                          case dbField::tpInt1:
                            p += sprintf(p, "%d", *(int1*)src) + 1;
                            src += sizeof(int1);
                            break;
                          case dbField::tpInt2:
                            p += sprintf(p, "%d", *(int2*)src) + 1;
                            src += sizeof(int2);
                            break;
                          case dbField::tpInt4:
                            p += sprintf(p, "%d", *(int4*)src) + 1;
                            src += sizeof(int4);
                            break;
                          case dbField::tpInt8:
                            p += sprintf(p, INT8_FORMAT, *(db_int8*)src) + 1;
                            src += sizeof(db_int8);
                            break;
                          case dbField::tpReal4:
                            p += sprintf(buf, "%.14g", *(real4*)src) + 1;
                            src += sizeof(real4);
                            break;
                          case dbField::tpReal8:
                            p += sprintf(buf, "%.14g", *(real8*)src) + 1;
                            src += sizeof(real8);
                            break;
                        }
                    }   
                } else { 
                    switch (sizeof_type[cb->cliType-cli_array_of_oid]) {
                      case 2:
                        while (--n >= 0) {
                            p = pack2(p, src);
                            src += 2;
                        }
                        break;
                      case 4:
                        while (--n >= 0) {
                            p = pack4(p, src);
                            src += 4;
                        }
                        break;
                      case 8:
                        while (--n >= 0) {
                            p = pack8(p, src);
                            src += 8;
                        }
                        break;
                      default:
                        memcpy(p, src, n);
                        p += n;
                    }
                }
                break;
              }
              case dbField::tpStructure:
                assert(cb->cliType == cli_datetime);
                p = pack4(p, src);
                break;
              default:
                assert(false);
            }
        }
    }
    assert(p - stmt->buf == msg_size);
    return session->sock->write(stmt->buf, msg_size);
}

bool dbServer::remove(dbClientSession* session, int stmt_id)
{
    dbStatement* stmt = findStatement(session, stmt_id);
    int4 response;
    if (stmt == NULL) {
        response = cli_bad_descriptor;
    } else {
        if (stmt->cursor->isEmpty()) {
            response = cli_not_found;
        } else {
            stmt->cursor->removeAllSelected();
            response = cli_ok;
        }
    }
    pack4(response);
    return session->sock->write(&response, sizeof response);
}

bool dbServer::remove_current(dbClientSession* session, int stmt_id)
{
    dbStatement* stmt = findStatement(session, stmt_id);
    int4 response;
    if (stmt == NULL) { 
        response = cli_bad_descriptor;
    } else { 
        if (stmt->cursor->isEmpty()) { 
            response = cli_not_found;
        } else { 
            stmt->cursor->remove();
            response = cli_ok;
        }
    }
    pack4(response);
    return session->sock->write(&response, sizeof response);
}

bool dbServer::update(dbClientSession* session, int stmt_id, char* new_data)
{
    dbStatement* stmt = findStatement(session, stmt_id);
    dbColumnBinding* cb;
    int4 response;
    if (stmt == NULL) {
        response = cli_bad_descriptor;
        pack4(response);
        return session->sock->write(&response, sizeof response);
    }
    if (stmt->cursor->isEmpty()) {
        response = cli_not_found;
        pack4(response);
        return session->sock->write(&response, sizeof response);
    }
    char* old_data = stmt->buf + sizeof(cli_oid_t) + 4;
    for (cb = stmt->columns; cb != NULL; cb = cb->next) {
        cb->ptr = new_data;
        old_data += 1; // skip column type
        if (cb->cliType == cli_decimal) {       
            if (cb->fd->indexType & (HASHED|INDEXED)
                && strcmp(new_data, old_data) != 0)
            {
                cb->fd->attr |= dbFieldDescriptor::Updated;
            }
            new_data += strlen(new_data) + 1;
            old_data += strlen(old_data) + 1;
        } else if (cb->cliType >= cli_asciiz && cb->cliType <= cli_array_of_string) {
            int new_len = unpack4(new_data);
            int old_len = unpack4(old_data);
            cb->len = new_len;
            if (cb->cliType == cli_cstring) { 
                cb->len += 1; // add '\0'
            }
            if (cb->fd->indexType & (HASHED|INDEXED)
                && memcmp(new_data, old_data, new_len*sizeof(char_t)+4) != 0)
            {
                cb->fd->attr |= dbFieldDescriptor::Updated;
            }
            new_data += 4;
            old_data += 4;
            if (cb->cliType == cli_array_of_string) {
                while (--new_len >= 0) { 
                    new_data = skip_str(new_data);
                }
                while (--old_len >= 0) { 
                    old_data = skip_str(old_data);
                }
            } else if (cb->cliType == cli_array_of_decimal) {
                while (--new_len >= 0) { 
                    new_data += strlen(new_data) + 1;
                }
                while (--old_len >= 0) {
                    old_data += strlen(old_data) + 1;
                }
            } else if (cb->cliType >= cli_array_of_oid) {
                new_data += new_len * sizeof_type[cb->cliType - cli_array_of_oid];
                old_data += old_len * sizeof_type[cb->cliType - cli_array_of_oid];
            } else {
                new_data += new_len * sizeof(char_t);
                old_data += old_len * sizeof(char_t);
            }
        } else {
            int size = sizeof_type[cb->cliType];
            if (cb->fd->indexType & (HASHED|INDEXED)
                && memcmp(new_data, old_data, size) != 0)
            {
                cb->fd->attr |= dbFieldDescriptor::Updated;
            }
            new_data += size;
            old_data += size;
        }
    }
    db->beginTransaction(dbExclusiveLock);

    dbGetTie tie;
    dbRecord* rec = db->getRow(tie, stmt->cursor->currId);
    dbTableDescriptor* table = stmt->query.table;
    dbFieldDescriptor *first = table->columns, *fd = first;
    size_t offs = table->fixedSize;
    oid_t oid = stmt->cursor->currId;

    do {
        if (fd->type == dbField::tpArray || fd->type == dbField::tpString)
        {
            int len = ((dbVarying*)((char*)rec + fd->dbsOffs))->size;
            for (cb = stmt->columns; cb != NULL; cb = cb->next) {
                if (cb->fd == fd) {
                    len = cb->len;
                    break;
                }
            }
            offs = DOALIGN(offs, fd->components->alignment)
                + len*fd->components->dbsSize;
            if (fd->components->type == dbField::tpString) { 
                if (cb != NULL) { 
                    char* src = cb->ptr + 4;
                    while (--len >= 0) { 
                        char* p = skip_str(src);
                        offs += p - src;
                        src = p;
                    }
                } else { 
                    dbVarying* v = (dbVarying*)((char*)rec + ((dbVarying*)((char*)rec + fd->dbsOffs))->offs);
                    while (--len >= 0) { 
                        offs += v->size*sizeof(char_t);
                        v += 1;
                    }
                }
            }
        }
    } while ((fd = fd->next) != first);

    old_data = (char*)rec;

    db->modified = true;
    size_t new_size = offs;
    new_data = new char[new_size];

    fd = first;
    offs = table->fixedSize;
    do {
        if (fd->type == dbField::tpArray || fd->type == dbField::tpString)
        {
            int len = ((dbVarying*)(old_data + fd->dbsOffs))->size;
            offs = DOALIGN(offs, fd->components->alignment);
            ((dbVarying*)(new_data + fd->dbsOffs))->offs = (int4)offs;
            for (cb = stmt->columns; cb != NULL; cb = cb->next) {
                if (cb->fd == fd) {
                    len = cb->unpackArray(new_data, offs);
                    break;
                }
            }
            ((dbVarying*)(new_data + fd->dbsOffs))->size = len;
            if (cb == NULL) {
                memcpy(new_data + offs,
                       old_data + ((dbVarying*)(old_data + fd->dbsOffs))->offs,
                       len*fd->components->dbsSize);
                if (fd->components->type == dbField::tpString) { 
                    dbVarying* new_str = (dbVarying*)(new_data + offs);
                    dbVarying* old_str = (dbVarying*)(old_data + ((dbVarying*)(old_data + fd->dbsOffs))->offs);
                    int relOffs = len*sizeof(dbVarying);
                    offs += relOffs;
                    while (--len >= 0) { 
                        int strlen = old_str->size*sizeof(char_t);
                        new_str->offs = relOffs;
                        memcpy(new_data + offs, (char*)old_str + old_str->offs, strlen);
                        relOffs += strlen - sizeof(dbVarying);
                        offs += strlen;
                        old_str += 1;
                        new_str += 1;
                    }
                } else {
                    offs += len*fd->components->dbsSize;
                }                   
            }
        } else {
            for (cb = stmt->columns; cb != NULL; cb = cb->next) {
                if (cb->fd == fd) {
                    if (cb->cliType == cli_autoincrement) { // autoincrement column is ignored
                        cb = NULL;
                        break;
                    }
                    cb->unpackScalar(new_data, false);
                    break;
                }
            }
            if (cb == NULL) {
                memcpy(new_data + fd->dbsOffs, old_data + fd->dbsOffs,
                       fd->dbsSize);
            }
        }
    } while ((fd = fd->next) != first);

    for (cb = stmt->columns; cb != NULL; cb = cb->next) {
        dbFieldDescriptor* fd = cb->fd; 
        if ((fd->attr & dbFieldDescriptor::Updated) != 0
            && (fd->indexType & UNIQUE) != 0
            && fd->type != dbField::tpRectangle)
        { 
            dbBtree::remove(db, fd->bTree, oid, (byte*)old_data, fd->dbsOffs, fd->comparator);
            if (!dbBtree::insert(db, fd->bTree, oid, (byte*)new_data, fd->dbsOffs, fd->comparator)) { 
                dbBtree::insert(db, fd->bTree, oid, (byte*)old_data, fd->dbsOffs, fd->comparator);
                for (dbColumnBinding* cbu = stmt->columns; cbu != cb; cbu = cbu->next) {
                    fd = cbu->fd; 
                    if ((fd->attr & dbFieldDescriptor::Updated) != 0
                        && (fd->indexType & UNIQUE) != 0
                        && fd->type != dbField::tpRectangle)
                    {
                        dbBtree::remove(db, fd->bTree, oid, (byte*)new_data, fd->dbsOffs, fd->comparator);
                        dbBtree::insert(db, fd->bTree, oid, (byte*)old_data, fd->dbsOffs, fd->comparator);
                    }
                }
                for (cb = stmt->columns; cb != NULL; cb = cb->next) {
                    cb->fd->attr &= ~dbFieldDescriptor::Updated;
                }
                delete[] new_data;
                response = cli_not_unique;
                pack4(response);
                return session->sock->write(&response, sizeof response);
            }
        }
    }
    for (cb = stmt->columns; cb != NULL; cb = cb->next) {
        dbFieldDescriptor* fd = cb->fd; 
        if (fd->attr & dbFieldDescriptor::Updated) { 
            if (fd->type == dbField::tpRectangle) { 
                dbRtree::remove(db, fd->bTree, oid, fd->dbsOffs);
            } else if ((fd->indexType & UNIQUE) == 0) { 
                dbBtree::remove(db, fd->bTree, oid, (byte*)old_data, fd->dbsOffs, fd->comparator);
            }
        }
    }
    {
        dbPutTie putTie(true);
        byte* dst = (byte*)db->putRow(putTie, oid, new_size);
        memcpy(dst+sizeof(dbRecord), new_data+sizeof(dbRecord), new_size-sizeof(dbRecord));
    }
    for (cb = stmt->columns; cb != NULL; cb = cb->next) {
        dbFieldDescriptor* fd = cb->fd; 
        if (fd->attr & dbFieldDescriptor::Updated) { 
            fd->attr &= ~dbFieldDescriptor::Updated;
            if (fd->type == dbField::tpRectangle) { 
                dbRtree::insert(db, fd->bTree, oid, fd->dbsOffs);
            } else if ((fd->indexType & UNIQUE) == 0) { 
                dbBtree::insert(db, fd->bTree, oid, (byte*)new_data, fd->dbsOffs, fd->comparator);
            }
        }
    }
    delete[] new_data;
    response = cli_ok;
    pack4(response);
    return session->sock->write(&response, sizeof response);
}



char* dbServer::checkColumns(dbStatement* stmt, int n_columns,
                             dbTableDescriptor* desc, char* data,
                             int4& response, bool select)
{
    dbColumnBinding** cpp = &stmt->columns;
    response = cli_ok;
    while (--n_columns >= 0) {
        int cliType = *data++;
#ifdef UNICODE
        char_t columnName[256];
        data = unpack_str(columnName, data);
#else
        char* columnName = data;
        data += strlen(data) + 1;
#endif
        dbFieldDescriptor* fd = desc->find(columnName);
        if (fd != NULL) {
            if ((cliType == cli_any && select
                 && (fd->type <= dbField::tpReference 
                     || (fd->type == dbField::tpArray 
                         && fd->components->type <= dbField::tpReference)))
                || (cliType == cli_oid
                    && fd->type == dbField::tpReference)
                || (cliType == cli_rectangle
                    && fd->type == dbField::tpRectangle)
                || (((cliType >= cli_bool && cliType <= cli_int8) || cliType == cli_autoincrement)
                    && fd->type >= dbField::tpBool
                    && fd->type <= dbField::tpInt8)
                || (cliType >= cli_real4 && cliType <= cli_real8
                    && fd->type >= dbField::tpReal4
                    && fd->type <= dbField::tpReal8)
                || (cliType == cli_decimal 
                    && fd->type >= dbField::tpInt1 
                    && fd->type <= dbField::tpReal8)
                || (cliType == cli_datetime
                    && ((fd->type == dbField::tpStructure
                         && fd->components->type == dbField::tpInt4) 
                        || fd->type ==  dbField::tpInt4))                   
                || ((cliType == cli_asciiz || cliType == cli_pasciiz || cliType == cli_cstring)
                    && fd->type == dbField::tpString)
                || (cliType == cli_array_of_oid &&
                    fd->type == dbField::tpArray &&
                    fd->components->type == dbField::tpReference)
                || (cliType == cli_autoincrement && fd->type == dbField::tpInt4)
                || (cliType >= cli_array_of_bool
                    && fd->type == dbField::tpArray
                    && fd->components->type <= dbField::tpReference
                    && (cliType-cli_array_of_oid == gb2cli_type_mapping[fd->components->type]
                        || (cliType == cli_array_of_decimal 
                            && fd->components->type >= dbField::tpInt1 
                            && fd->components->type <= dbField::tpReal8))))
            {
                if (cliType == cli_any) { 
                    cliType = map_type(fd);
                }
                dbColumnBinding* cb = new dbColumnBinding(fd, cliType);
                *cpp = cb;
                cpp = &cb->next;
            } else {
                response = cli_incompatible_type;
                break;
            }
        } else {
            TRACE_MSG((STRLITERAL("Field '%s' not found\n"), columnName));
            response = cli_column_not_found;
            break;
        }
    }
    return data;
}


bool dbServer::insert(dbClientSession* session, int stmt_id, char* data, bool prepare)
{
    dbStatement* stmt = findStatement(session, stmt_id);
    dbTableDescriptor* desc = NULL;
    dbColumnBinding* cb;
    int4   response;
    char   reply_buf[sizeof(cli_oid_t) + 8];
    oid_t  oid = 0;
    size_t offs;
    size_t size;
    int    n_columns;

    if (stmt == NULL) {
        if (!prepare) {
            response = cli_bad_statement;
            goto return_response;
        }
        stmt = new dbStatement(stmt_id);
        stmt->next = session->stmts;
        session->stmts = stmt;
    } else {
        if (prepare) {
            stmt->reset();
        } else if ((desc = stmt->table) == NULL) {
            response = cli_bad_descriptor;
            goto return_response;
        }
    }
    if (prepare) {
        session->scanner.reset(data);
        if (session->scanner.get() != tkn_insert
            || session->scanner.get() != tkn_into
            || session->scanner.get() != tkn_ident)
        {
            response = cli_bad_statement;
            goto return_response;
        }
        desc = db->findTable(session->scanner.ident);
        if (desc == NULL) {
            response = cli_table_not_found;
            goto return_response;
        }
        while (unpack_char(data) != '\0') { 
            data += sizeof(char_t);
        }
        data += sizeof(char_t);
        n_columns = *data++ & 0xFF;
        data = checkColumns(stmt, n_columns, desc, data, response, false);
        if (response != cli_ok) {
            goto return_response;
        }
        stmt->table = desc;
    }

    offs = desc->fixedSize;
    for (cb = stmt->columns; cb != NULL; cb = cb->next) {
        cb->ptr = data;
        if (cb->cliType == cli_decimal) {
            data += strlen(data) + 1;
        } else if (cb->cliType == cli_datetime) {
            data += 4;
        } else if (cb->cliType == cli_autoincrement) {
            ;
        } else if (cb->cliType >= cli_asciiz && cb->cliType  <= cli_array_of_string) {
            int len = unpack4(data);
            cb->len = len;
            if (cb->cliType == cli_cstring) { 
                cb->len += 1; // add '\0'
            }
            offs = DOALIGN(offs, cb->fd->components->alignment)
                 + cb->len*cb->fd->components->dbsSize;
            data += 4;
            if (cb->cliType == cli_array_of_string) { 
                while (--len >= 0) { 
                    char* p = skip_str(data); 
                    offs += p - data;
                    data = p;
                }
            } else if (cb->cliType == cli_array_of_decimal) { 
                while (--len >= 0) { 
                    data += strlen(data) + 1; 
                }
            } else {
                data += len*cb->fd->components->dbsSize;
            }
        } else {
            data += sizeof_type[cb->cliType];
        }
    }
    db->beginTransaction(dbExclusiveLock);
    db->modified = true;
    size = offs + sizeof(char_t);
    oid = db->allocateRow(desc->tableId, size, stmt->table);
    { 
        dbPutTie tie;
        char* dst = (char*)db->putRow(tie, oid);
        memset(dst + sizeof(dbRecord), 0, size - sizeof(dbRecord));
        offs = desc->fixedSize;
        for (cb = stmt->columns; cb != NULL; cb = cb->next) {
            dbFieldDescriptor* fd = cb->fd;
            if (fd->type == dbField::tpArray || fd->type == dbField::tpString) {
                offs = DOALIGN(offs, fd->components->alignment);
                ((dbVarying*)(dst + fd->dbsOffs))->offs = (int4)offs;
                ((dbVarying*)(dst + fd->dbsOffs))->size = cb->len;
                cb->unpackArray(dst, offs);
            } else {
                cb->unpackScalar(dst, true);
            }
        }
        dbFieldDescriptor* fd = desc->columns; 
        for (int i = (int)desc->nColumns; --i >= 0; fd = fd->next) { 
            if (fd->type == dbField::tpString && ((dbVarying*)(dst + fd->dbsOffs))->offs == 0) {
                ((dbVarying*)(dst + fd->dbsOffs))->size = 1;
                ((dbVarying*)(dst + fd->dbsOffs))->offs = (int4)(size - sizeof(char_t));
            }
        }
    }
    for (cb = stmt->columns; cb != NULL; cb = cb->next) {
        dbFieldDescriptor* fd = cb->fd;
        if ((fd->indexType & UNIQUE) != 0 && fd->type != dbField::tpRectangle) { 
            if (!dbBtree::insert(db, fd->bTree, oid, fd->dbsOffs, fd->comparator)) { 
                for (dbColumnBinding* cbu = stmt->columns; cbu != cb; cbu = cbu->next) {
                    fd = cbu->fd;
                    if ((fd->indexType & UNIQUE) != 0 && fd->type != dbField::tpRectangle) { 
                        dbBtree::remove(db, fd->bTree, oid, fd->dbsOffs, fd->comparator);
                    }
                }
                db->freeRow(desc->tableId, oid, desc);
                response = cli_not_unique;
                oid = 0;
                goto return_response;
            }
        }
    }
    for (cb = stmt->columns; cb != NULL; cb = cb->next) {
        dbFieldDescriptor* fd = cb->fd;
        if (fd->indexType & INDEXED) {
            if (fd->type == dbField::tpRectangle) { 
                dbRtree::insert(db, cb->fd->bTree, oid, cb->fd->dbsOffs);
            } else if ((fd->indexType & UNIQUE) == 0) { 
                dbBtree::insert(db, cb->fd->bTree, oid, cb->fd->dbsOffs, cb->fd->comparator);
            }
        }
    }
    response = cli_ok;
  return_response:
    pack4(reply_buf, response);
    if (desc == NULL) { 
        pack4(reply_buf+4, 0);
    } else { 
#ifdef AUTOINCREMENT_SUPPORT
        pack4(reply_buf+4, desc->autoincrementCount);
#else
        pack4(reply_buf+4, desc->nRows);
#endif
    }
    pack_oid(reply_buf+8, oid);
    if (stmt_id == 0) { 
        session->stmts = stmt->next;
        delete stmt;
    }
    return session->sock->write(reply_buf, sizeof reply_buf);    
}

bool dbServer::describe_table(dbClientSession* session, char* data)
{
    char_t* table = (char_t*)data;
    unpack_str(table, data);
    dbTableDescriptor* desc = db->findTableByName(table);
    if (desc == NULL) {
        char response[8];
        pack4(response, 0);
        pack4(response+4, -1);
        return session->sock->write(response, sizeof response);
    } else { 
        int i, length = 0;
        dbFieldDescriptor* fd = desc->columns; 
        for (i = (int)desc->nColumns; --i >= 0;) { 
            length += 2;
            length += (int)((STRLEN(fd->name)+3)*sizeof(char_t));
            if (fd->refTableName != NULL) {  
                length += (int)(STRLEN(fd->refTableName)*sizeof(char_t));
            } else if (fd->type == dbField::tpArray && fd->components->refTableName != NULL) {
                length += (int)(STRLEN(fd->components->refTableName)*sizeof(char_t));
            }
            if (fd->inverseRefName != NULL) { 
                length += (int)(STRLEN(fd->inverseRefName)*sizeof(char_t));
            }
            fd = fd->next;
        }
        dbSmallBuffer<char> response(length+8);
        char* p = (char*)response;
        pack4(p, length);
        pack4(p+4, (int4)desc->nColumns);
        p += 8;
        for (i = (int)desc->nColumns, fd = desc->columns; --i >= 0;) { 
            int flags = 0;
            *p++ = (char)map_type(fd);
            if (fd->bTree != 0) { 
                flags |= cli_indexed;
                dbGetTie tie;
                dbBtree* tree = (dbBtree*)db->getRow(tie, fd->bTree);
                if (tree->isCaseInsensitive()) { 
                    flags |= cli_case_insensitive;
                }
            }
            if (fd->hashTable != 0) {
                flags |= cli_hashed;
            }
            *p++ = (char)flags;            
            p = pack_str(p, fd->name);
            if (fd->refTableName != NULL) {
                p = pack_str(p, fd->refTableName);
            } else if (fd->type == dbField::tpArray && fd->components->refTableName != NULL) {
                p = pack_str(p, fd->components->refTableName);
            } else { 
                *(char_t*)p = 0; // ref table is NULL
                p += sizeof(char_t);
            }
            if (fd->inverseRefName != NULL) { 
                p = pack_str(p, fd->inverseRefName);
            } else { 
                *(char_t*)p = 0;
                p += sizeof(char_t);
            }
            fd = fd->next;
        }
        return session->sock->write(response, length+8);
    }
}

bool dbServer::select(dbClientSession* session, int stmt_id, char* msg, bool prepare)
{
    int4 response;
    int i, n_params, tkn, n_columns;
    dbStatement* stmt = findStatement(session, stmt_id);
    dbCursorType cursorType;
    dbTableDescriptor* desc;

    if (prepare) {
        if (stmt == NULL) {
            stmt = new dbStatement(stmt_id);
            stmt->next = session->stmts;
            session->stmts = stmt;
        } else {
            stmt->reset();
        }
        stmt->n_params = *msg++ & 0xFF;
        stmt->n_columns = n_columns = *msg++ & 0xFF;
        stmt->params = new dbParameterBinding[stmt->n_params];
        int len = unpack2(msg);
        msg += 2;
        session->scanner.reset(msg);
        char *p, *end = msg + len;
        if (session->scanner.get() != tkn_select) {
            TRACE_MSG((STRLITERAL("Bad select statement: %s\n"), msg));
            response = cli_bad_statement;
            goto return_response;
        }
        if ((tkn = session->scanner.get()) == tkn_all) {
            tkn = session->scanner.get();
        }
        if (tkn == tkn_from && session->scanner.get() == tkn_ident) {
            if ((desc = db->findTable(session->scanner.ident)) != NULL) {
                msg = checkColumns(stmt, n_columns, desc, end, response, true);
                if (response != cli_ok) {
                    goto return_response;
                }
                stmt->cursor = new dbAnyCursor(*desc, dbCursorViewOnly, NULL);
                stmt->cursor->setPrefetchMode(false);
            } else {
                response = cli_table_not_found;
                goto return_response;
            }
        } else {
            TRACE_MSG((STRLITERAL("Bad select statement: %s\n"), msg));
            response = cli_bad_statement;
            goto return_response;
        }
        p = session->scanner.p;
        for (i = 0; p < end; i++) {
#ifdef UNICODE
            char_t* dst = (char_t*)((size_t)p & ~(sizeof(char_t)-1)); 
            p = unpack_str(dst, p);
            stmt->query.append(dbQueryElement::qExpression, dst);
#else
            stmt->query.append(dbQueryElement::qExpression, p);
            p += strlen(p) + 1;
#endif
            if (p < end) {
                int cliType = *p++;
                static const dbQueryElement::ElementType type_map[] = {
                    dbQueryElement::qVarReference, // cli_oid
                    dbQueryElement::qVarBool,      // cli_bool
                    dbQueryElement::qVarInt1,      // cli_int1
                    dbQueryElement::qVarInt2,      // cli_int2
                    dbQueryElement::qVarInt4,      // cli_int4
                    dbQueryElement::qVarInt8,      // cli_int8
                    dbQueryElement::qVarReal4,     // cli_real4
                    dbQueryElement::qVarReal8,     // cli_real8
                    dbQueryElement::qVarReal8,     // cli_decimal
                    dbQueryElement::qVarStringPtr, // cli_asciiz
                    dbQueryElement::qVarStringPtr, // cli_pasciiz
                    dbQueryElement::qVarUnknown,   // cli_cstring
                    dbQueryElement::qVarUnknown,   // cli_array_of_oid,
                    dbQueryElement::qVarUnknown,   // cli_array_of_bool
                    dbQueryElement::qVarUnknown,   // cli_array_of_int1
                    dbQueryElement::qVarUnknown,   // cli_array_of_int2
                    dbQueryElement::qVarUnknown,   // cli_array_of_int4
                    dbQueryElement::qVarUnknown,   // cli_array_of_db_int8
                    dbQueryElement::qVarUnknown,   // cli_array_of_real4
                    dbQueryElement::qVarUnknown,   // cli_array_of_real8
                    dbQueryElement::qVarUnknown,   // cli_array_of_decimal
                    dbQueryElement::qVarUnknown,   // cli_array_of_string
                    dbQueryElement::qVarUnknown,   // cli_any
                    dbQueryElement::qVarInt4,      // cli_datetime
                    dbQueryElement::qVarUnknown,   // cli_autoincrement
                    dbQueryElement::qVarRectangle, 
                    dbQueryElement::qVarUnknown,   // cli_unknown
                };
                stmt->params[i].type = cliType;
                stmt->query.append(type_map[cliType], &stmt->params[i].u);
            }
        }
    } else {
        if (stmt == NULL) {
            response = cli_bad_descriptor;
            goto return_response;
        }
    }
    stmt->firstFetch = true;
    cursorType = (dbCursorType)*msg++;

    for (i = 0, n_params = stmt->n_params; i < n_params; i++) {
        switch (stmt->params[i].type) {
          case cli_oid:
            stmt->params[i].u.oid = unpack_oid(msg);
            msg += sizeof(cli_oid_t);
            break;
          case cli_int1:
            stmt->params[i].u.i1 = *msg++;
            break;
          case cli_int2:
            msg = unpack2((char*)&stmt->params[i].u.i2, msg);
            break;
          case cli_int4:
            msg = unpack4((char*)&stmt->params[i].u.i4, msg);
            break;
          case cli_int8:
            msg = unpack8((char*)&stmt->params[i].u.i8, msg);
            break;
          case cli_real4:
            msg = unpack4((char*)&stmt->params[i].u.r4, msg);
            break;
          case cli_real8:
            msg = unpack8((char*)&stmt->params[i].u.r8, msg);
            break;
          case cli_bool:
            stmt->params[i].u.b = *msg++;
            break;
          case cli_decimal:
          {
            sscanf(msg, "%lf", &stmt->params[i].u.r8);
            msg += strlen(msg) + 1;
            break;
          }
          case cli_asciiz:
          case cli_pasciiz:
#ifdef UNICODE
            msg += -(int)msg & (sizeof(char_t)-1);
            stmt->params[i].u.str = (char_t*)msg;
            msg = unpack_str((char_t*)msg, msg);
#else
            stmt->params[i].u.str = msg;
            msg += strlen(msg) + 1;
#endif
            break;
          case cli_cstring:
            { 
                char_t* dst = (char_t*)(msg + (-(size_t)msg & (sizeof(char_t)-1)));                
                stmt->params[i].u.str = dst;
                int len = unpack4(msg);
                msg = unpack_str(dst, msg + 4, len);
                dst[len] = '\0';
            }
            break;
          case cli_rectangle:
            assert(sizeof(cli_rectangle_t) == sizeof(rectangle));
            msg = unpack_rectangle((cli_rectangle_t*)&stmt->params[i].u.rect, msg);
            break;
          default:
            TRACE_MSG((STRLITERAL("Usupported type: %d\n"), stmt->params[i].type));
            response = cli_bad_statement;
            goto return_response;
        }
    }
#ifdef THROW_EXCEPTION_ON_ERROR
    try {
        response = stmt->cursor->select(stmt->query, cursorType);
    } catch (dbException const& x) {
        response = (x.getErrCode() == dbDatabase::QueryError)
            ? cli_bad_statement : cli_runtime_error;
    }
#else
    {
        dbDatabaseThreadContext* ctx = db->threadContext.get();
        ctx->catched = true;
        int errorCode = setjmp(ctx->unwind);
        if (errorCode == 0) {
            response = stmt->cursor->select(stmt->query, cursorType);
        } else {
            TRACE_MSG((STRLITERAL("Select failed with %d error code\n"), errorCode));
            response = (errorCode == dbDatabase::QueryError)
                ? cli_bad_statement : cli_runtime_error;
        }
        ctx->catched = false;
    }
#endif
  return_response:
    pack4(response);
    return session->sock->write(&response, sizeof response);
}



bool dbServer::show_tables(dbClientSession* session)
{
    dbTableDescriptor* desc=db->tables;
    if (desc == NULL) {
        char response[8];
        pack4(response, 0);
        pack4(response+4, -1);
        return session->sock->write(response, sizeof response);
    } else {
        int length = 0, n = 0;
        for (desc=db->tables; desc != NULL; desc=desc->nextDbTable) {
            if (STRCMP(desc->name, STRLITERAL("Metatable"))) {
                length += (int)((STRLEN(desc->name)+1)*sizeof(char_t));
                n++;
            }
        }
        dbSmallBuffer<char> response(length+8);
        char* p = (char*)response;
        pack4(p, length);
        pack4(p+4, n);
        p += 8;
        for (desc=db->tables; desc != NULL; desc=desc->nextDbTable) {
            if (STRCMP(desc->name, STRLITERAL("Metatable")) != 0) {
                p = pack_str(p, desc->name);
            }
        }
        return session->sock->write(response, length+8);
    }
}

bool dbServer::update_table(dbClientSession* session, char* data, bool create)
{
    db->beginTransaction(dbUpdateLock);
    db->modified = true;
    char_t* tableName = (char_t*)data;
    data = unpack_str(tableName, data);
    int nColumns = *data++ & 0xFF;
    dbSmallBuffer<cli_field_descriptor> columnsBuf(nColumns);    
    cli_field_descriptor* columns = (cli_field_descriptor*)columnsBuf;
    for (int i = 0; i < nColumns; i++) {
        columns[i].type = (cli_var_type)*data++;
        columns[i].flags = *data++ & 0xFF;
        columns[i].name = (char_t*)data;
        data = unpack_str((char_t*)columns[i].name, data);
        if (*(char_t*)data != 0) { 
            columns[i].refTableName = (char_t*)data;
            data = unpack_str((char_t*)columns[i].refTableName, data);
        } else { 
            columns[i].refTableName = NULL;
            data += sizeof(char_t);
        }
        if (*(char_t*)data != 0) { 
            columns[i].inverseRefFieldName = (char_t*)data;
            data = unpack_str((char_t*)columns[i].inverseRefFieldName, data);
        } else { 
            columns[i].inverseRefFieldName = NULL;
            data += sizeof(char_t);
        }
    }
    int4 response;
    if (create) {
        if (session->existed_tables == NULL) { 
            session->existed_tables = db->tables;
        }
        response = dbCLI::create_table(db, tableName, nColumns, columns);
    } else { 
        response = dbCLI::alter_table(db, tableName, nColumns, columns);
    }
    pack4(response);
    return session->sock->write(&response, sizeof response);
}

bool dbServer::drop_table(dbClientSession* session, char* data)
{
    char_t* tableName = (char_t*)data;
    unpack_str(tableName, data);
    db->beginTransaction(dbUpdateLock);
    dbTableDescriptor* desc = db->findTableByName(tableName);
    int4 response = cli_ok;
    if (desc != NULL) {
        db->dropTable(desc);
        if (desc == session->existed_tables) { 
            session->existed_tables = desc->nextDbTable;
        }
        db->unlinkTable(desc);
        desc->nextDbTable = session->dropped_tables;
        session->dropped_tables = desc;
    } else { 
        response = cli_table_not_found;
    }
    pack4(response);
    return session->sock->write(&response, sizeof response);
}

bool dbServer::alter_index(dbClientSession* session, char* data)
{
    char_t* tableName = (char_t*)data;
    data = unpack_str(tableName, data);
    char_t* fieldName = (char_t*)data;
    data = unpack_str(fieldName, data);
    int newFlags = *data++ & 0xFF;
    int4 response = dbCLI::alter_index(db, tableName, fieldName, newFlags);
    pack4(response);
    return session->sock->write(&response, sizeof response);
}


bool dbServer::authenticate(char* buf)
{
#ifdef SECURE_SERVER
    dbCursor<UserInfo> cursor;
    dbQuery q;
    char_t* user = (char_t*)buf;
    buf = unpack_str(user, buf);
    char_t* password = (char_t*)buf;
    unpack_str(password, buf);
    q = "user=",&user,"and password=",&password;
    bool succeed = cursor.select(q) != 0;
    if (!succeed) {
        dbTrace(STRLITERAL("Login failure for user %s password %s\n"),
                           user, password);
    }
    db->commit();
    return succeed;
#else
    return true;
#endif
}


bool dbServer::insert_cpp(dbClientSession* session, char* data)
{
    db->beginTransaction(dbExclusiveLock);
    dbTableDescriptor* desc = *(dbTableDescriptor**)data;
    data += sizeof(dbTableDescriptor*);
    bool batch = *data++ != 0;
    dbRecord* record = (dbRecord*)data;
    db->refreshTable(desc);
    db->modified = true;
    dbFieldDescriptor* fd;

    oid_t oid = db->allocateRow(desc->tableId, record->size, desc);
    {
        dbPutTie tie;
        byte* dst = (byte*)db->putRow(tie, oid);
        memcpy(dst + sizeof(dbRecord), record+1, record->size - sizeof(dbRecord));
    }

    if (batch) { 
        if (!desc->isInBatch) { 
            desc->isInBatch = true;
            desc->nextBatch = db->batchList;
            db->batchList = desc;
            desc->batch.reset();
        }        
        desc->batch.add(oid);
    } else { 
        for (fd = desc->indexedFields; fd != NULL; fd = fd->nextIndexedField) {
            if ((fd->indexType & UNIQUE) != 0 && fd->type != dbField::tpRectangle) { 
                if (!dbBtree::insert(db, fd->bTree, oid, fd->dbsOffs, fd->comparator)) { 
                    for (dbFieldDescriptor* fdu = desc->indexedFields; fdu != fd; fdu = fdu->nextIndexedField) {
                        if ((fdu->indexType & UNIQUE) != 0 && fdu->type != dbField::tpRectangle) { 
                            dbBtree::remove(db, fdu->bTree, oid, fdu->dbsOffs, fdu->comparator);
                        }
                    }
                    db->freeRow(desc->tableId, oid, desc);
                    oid = 0;
                    break;
                }
            }
        }
        if (oid != 0) { 
            size_t nRows = desc->nRows;
            for (fd = desc->hashedFields; fd != NULL; fd = fd->nextHashedField) {
                dbHashTable::insert(db, fd->hashTable, oid, fd->type, fd->dbsOffs, nRows);
            }
            for (fd = desc->indexedFields; fd != NULL; fd = fd->nextIndexedField) {
                if (fd->type == dbField::tpRectangle) { 
                    dbRtree::insert(db, fd->bTree, oid, fd->dbsOffs);
                } else if ((fd->indexType & UNIQUE) == 0) { 
                    dbBtree::insert(db, fd->bTree, oid, fd->dbsOffs, fd->comparator);
                }
            }
        }
    }
    if (oid != 0) {     
        for (fd = desc->inverseFields; fd != NULL; fd = fd->nextInverseField) {
            if (fd->type == dbField::tpArray) {
                dbVarying* arr = (dbVarying*)(data + fd->dbsOffs);
                int n = arr->size;
                oid_t* refs = (oid_t*)(data + arr->offs);
                while (--n >= 0) {
                    if (refs[n] != 0) {
                        db->insertInverseReference(fd, oid, refs[n]);
                    }
                }
            } else if (!(fd->indexType & DB_BLOB_CASCADE_DELETE)) {
                oid_t ref = *(oid_t*)(data + fd->dbsOffs);
                if (ref != 0) {
                    db->insertInverseReference(fd, oid, ref);
                }
            }
        }
    }
    return session->sock->write(&oid, sizeof oid);    
}

bool dbServer::update_cpp(dbClientSession* session, char* data)
{
    db->beginTransaction(dbExclusiveLock);
    db->modified = true;
    dbTableDescriptor* desc = *(dbTableDescriptor**)data;
    data += sizeof(dbTableDescriptor*);
    oid_t oid = *(oid_t*)data;
    data += sizeof(oid_t);
    dbRecord* record = (dbRecord*)data;
    bool response = false;
    dbGetTie getTie;
    byte* old = (byte*)db->getRow(getTie, oid);
    desc->columns->markUpdatedFields2(old, (byte*)data);

    dbFieldDescriptor* fd;

    for (fd = desc->indexedFields; fd != NULL; fd = fd->nextIndexedField) {
        if (fd->attr & (dbFieldDescriptor::Updated) 
            && (fd->indexType & UNIQUE) != 0
            && fd->type != dbField::tpRectangle)
        { 
            dbBtree::remove(db, fd->bTree, oid, fd->dbsOffs, fd->comparator);
            if (!dbBtree::insert(db, fd->bTree, oid, (byte*)data, fd->dbsOffs, fd->comparator)) { 
                dbBtree::insert(db, fd->bTree, oid, fd->dbsOffs, fd->comparator);
                for (dbFieldDescriptor* fdu = desc->indexedFields; fdu != fd; fdu = fdu->nextIndexedField) {                    
                    if ((fdu->attr & dbFieldDescriptor::Updated) != 0
                        && (fdu->indexType & UNIQUE) != 0
                        && fdu->type != dbField::tpRectangle)
                    {
                        dbBtree::remove(db, fdu->bTree, oid, (byte*)data, fdu->dbsOffs, fdu->comparator);
                        dbBtree::insert(db, fdu->bTree, oid, fdu->dbsOffs, fdu->comparator);
                    }
                }
                for (fd = desc->indexedFields; fd != NULL; fd = fd->nextIndexedField) {
                    fd->attr &= ~dbFieldDescriptor::Updated;
                }
                for (fd = desc->hashedFields; fd != NULL; fd = fd->nextHashedField) {
                    fd->attr &= ~dbFieldDescriptor::Updated;
                }
                return session->sock->write(&response, sizeof response);    
            }
        }
    }
       
    for (fd = desc->hashedFields; fd != NULL; fd = fd->nextHashedField) {
        if (fd->attr & dbFieldDescriptor::Updated) {
            dbHashTable::remove(db, fd->hashTable, oid, fd->type,fd->dbsOffs);
        }
    }
    for (fd = desc->indexedFields; fd != NULL; fd = fd->nextIndexedField) {
        if (fd->attr & dbFieldDescriptor::Updated) {
            if (fd->type == dbField::tpRectangle) { 
                dbRtree::remove(db, fd->bTree, oid, fd->dbsOffs);
            } else if ((fd->indexType & UNIQUE) == 0) { 
                dbBtree::remove(db, fd->bTree, oid, fd->dbsOffs, fd->comparator);
            }
        }
    }

    db->updatedRecordId = oid;
    for (fd = desc->inverseFields; fd != NULL; fd = fd->nextInverseField) {
        if (fd->type == dbField::tpArray) {
            dbVarying* arr = (dbVarying*)(data + fd->dbsOffs);
            int n = arr->size;
            oid_t* newrefs = (oid_t*)(data + arr->offs);

            int m = ((dbVarying*)(old + fd->dbsOffs))->size;
            int offs =  ((dbVarying*)(old + fd->dbsOffs))->offs;
            int i, j, k;

            if (fd->indexType & DB_FIELD_CASCADE_DELETE) {
                for (i = 0, k = 0; i < m; i++) {
                    oid_t oldref = *(oid_t*)(old + offs);
                    offs += sizeof(oid_t);
                    for (j = i; j < n && newrefs[j] != oldref; j++);
                    if (j >= n) {
                        j = i < n ? i : n;
                        while (--j >= 0 && newrefs[j] != oldref);
                        if (j < 0) {
                            k += 1;
                            db->removeInverseReference(fd, oid, oldref);
                        }
                    }
                }
                if (n != m - k) {
                    oid_t* oldrefs = (oid_t*)(old + offs) - m;
                    for (i = 0; i < n; i++) {
                        for (j = 0; j < m && newrefs[i] != oldrefs[j]; j++);
                        if (j == m) {
                            db->insertInverseReference(fd, oid, newrefs[i]);
                        }
                    }
                }
            } else {
                k = n < m ? n : m;
                for (i = 0; i < k; i++) {
                    oid_t oldref = *(oid_t*)(old + offs);
                    offs += sizeof(oid_t);
                    if (newrefs[i] != oldref) {
                        if (oldref != 0) {
                            db->removeInverseReference(fd, oid, oldref);
                        }
                        if (newrefs[i] != 0) {
                            db->insertInverseReference(fd, oid, newrefs[i]);
                        }
                    }
                }
                while (i < m) {
                    oid_t oldref = *(oid_t*)(old + offs);
                    offs += sizeof(oid_t);
                    if (oldref != 0) {
                        db->removeInverseReference(fd, oid, oldref);
                    }
                    i += 1;
                }
                while (i < n) {
                    if (newrefs[i] != 0) {
                        db->insertInverseReference(fd, oid, newrefs[i]);
                    }
                    i += 1;
                }
            }
        } else {
            oid_t newref = *(oid_t*)(data + fd->dbsOffs);
            oid_t oldref = *(oid_t*)(old + fd->dbsOffs);
            if (newref != oldref) {
                if (oldref != 0) {
                    db->removeInverseReference(fd, oid, oldref);
                }
                if (newref != 0 && !(fd->indexType & DB_BLOB_CASCADE_DELETE)) {
                    db->insertInverseReference(fd, oid, newref);
                }
            }
        }
    }
    db->updatedRecordId = 0;

    {
        dbPutTie putTie(true);
        byte* dst = (byte*)db->putRow(putTie, oid, record->size);
        memcpy(dst+sizeof(dbRecord), data+sizeof(dbRecord), record->size-sizeof(dbRecord));
    }

    for (fd = desc->hashedFields; fd != NULL; fd = fd->nextHashedField) {
        if (fd->attr & dbFieldDescriptor::Updated) {
            dbHashTable::insert(db, fd->hashTable, oid, fd->type, fd->dbsOffs, 0);
        }
    }
    for (fd = desc->indexedFields; fd != NULL; fd = fd->nextIndexedField) {
        if (fd->attr & dbFieldDescriptor::Updated) {
            fd->attr &= ~dbFieldDescriptor::Updated;
            if (fd->type == dbField::tpRectangle) { 
                dbRtree::insert(db, fd->bTree, oid, fd->dbsOffs);
            } else if ((fd->indexType & UNIQUE) == 0) { 
                dbBtree::insert(db, fd->bTree, oid, fd->dbsOffs, fd->comparator);
            }
        }
    }
    for (fd = desc->hashedFields; fd != NULL; fd = fd->nextHashedField) {
        fd->attr &= ~dbFieldDescriptor::Updated;
    }
    db->updateCursors(oid); 
    response = true;
    return session->sock->write(&response, sizeof response);    
}

bool dbServer::remove_cpp(dbClientSession* session, char* data)
{
    dbTableDescriptor* desc = *(dbTableDescriptor**)data;
    data += sizeof(dbTableDescriptor*);
    oid_t oid = *(oid_t*)data;
    data += sizeof(oid_t);
    db->remove(desc, oid);
    return true;
}

const size_t SOCKET_WRITE_BUFFER = 64*1024;

bool dbServer::select_cpp(dbClientSession* session, char* data)
{
    dbQuery query;
    dbAnyCursor cursor(dbCursorIncremental);
    char buf[SOCKET_WRITE_BUFFER];
    size_t bufUsed = 0;
    if (execute_query(data, query, cursor)) {
        dbGetTie tie;
        do { 
            if (sizeof(buf) - bufUsed < sizeof(oid_t)) {        
                if (!session->sock->write(buf, bufUsed)) { 
                    return false;
                }
                bufUsed = 0;
            }
            oid_t oid = cursor.getOid();
            *(oid_t*)&buf[bufUsed] = oid;
            bufUsed += sizeof(oid_t);
            dbRecord* record = db->getRow(tie, oid);
            size_t size = record->size;
            char* src = (char*)record;
            size_t available = sizeof(buf) - bufUsed;
            if (size >= available) {
                memcpy(buf + bufUsed, src, available);
                if (!session->sock->write(buf, sizeof(buf))) { 
                    return false;
                }
                size -= available;
                src += available;
                bufUsed = 0;
            }
            if (size >= sizeof(buf)) { 
                if (!session->sock->write(src, size)) { 
                    return false;
                }
            } else {
                memcpy(buf + bufUsed, src, size);
                bufUsed += size;
            }
        } while (cursor.gotoNext());

        if (sizeof(buf) - bufUsed < sizeof(oid_t)) {        
            if (!session->sock->write(buf, bufUsed)) { 
                return false;
            }
            bufUsed = 0;
        }
    }
    *(oid_t*)&buf[bufUsed] = 0;
    return session->sock->write(buf, bufUsed + sizeof(oid_t));
}

bool dbServer::remove_cond(dbClientSession* session, char* data)
{
    dbQuery query;
    dbAnyCursor cursor(dbCursorForUpdate);
    size_t n = execute_query(data, query, cursor);
    if (n != 0) {
        cursor.removeAllSelected();
    }
    return session->sock->write(&n, sizeof n);
}
    
int dbServer::execute_query(char* data, dbQuery& q, dbAnyCursor& cursor)
{
    cursor.setTable(*(dbTableDescriptor**)data);
    data += sizeof(dbTableDescriptor*);
    size_t limit = *(size_t*)data;
    data += sizeof(size_t);
    while (true) { 
        switch (*data++) { 
          case dbQueryElement::qExpression:
            q.append(dbQueryElement::qExpression, data);
            data += (STRLEN((char_t*)data) + 1)*sizeof(char_t);
            continue;
          case dbQueryElement::qVarBool:
            q.append(dbQueryElement::qVarBool, data++);
            continue;
          case dbQueryElement::qVarInt1:
            q.append(dbQueryElement::qVarInt1, data++);
            continue;
          case dbQueryElement::qVarInt2:
            q.append(dbQueryElement::qVarInt2, data);
            data += sizeof(int2);
            continue;
          case dbQueryElement::qVarInt4:
            q.append(dbQueryElement::qVarInt4, data);
            data += sizeof(int4);
            continue;
          case dbQueryElement::qVarInt8:
            q.append(dbQueryElement::qVarInt8, data);
            data += sizeof(db_int8);
            continue;
          case dbQueryElement::qVarReal4:
            q.append(dbQueryElement::qVarReal4, data);
            data += sizeof(real4);
            continue;
          case dbQueryElement::qVarReal8:
            q.append(dbQueryElement::qVarReal8, data);
            data += sizeof(real8);
            continue;
          case dbQueryElement::qVarString:
          case dbQueryElement::qVarStringPtr:
            q.append(dbQueryElement::qVarString, data);
            data += (STRLEN((char_t*)data) + 1)*sizeof(char_t);
            continue;
          case dbQueryElement::qVarReference:
            q.append(dbQueryElement::qVarReference, data);
            data += sizeof(oid_t);
            continue;
          case dbQueryElement::qVarArrayOfRef:
          case dbQueryElement::qVarArrayOfRefPtr:
          {
              dbAnyArray* arr = (dbAnyArray*)data;
              q.append(dbQueryElement::qVarArrayOfRef, arr);
              data += sizeof(dbAnyArray) + sizeof(oid_t*);
              *((oid_t**)data-1) = (oid_t*)data;
              data += arr->length()*sizeof(oid_t);
              continue;
          }
          case dbQueryElement::qVarRectangle:
            q.append(dbQueryElement::qVarRectangle, data);
            data += sizeof(rectangle);
            continue;
          default:
            break;
        }
        break;
    }
    cursor.setPrefetchMode(false);
    if (limit != 0) { 
        cursor.setSelectionLimit(limit);
    }
    return cursor.select(q);
}

        
bool dbServer::reload_schema(dbClientSession* session, char* data)
{
    db->beginTransaction(dbExclusiveLock);

    int nClientTables = *(int*)data;
    data += sizeof(int);
    dbTableDescriptor** descriptors = new dbTableDescriptor*[nClientTables];
    memset(descriptors, 0, nClientTables*sizeof(dbTableDescriptor*));

    dbGetTie tie;
    dbTable* metaTable = (dbTable*)db->get(dbMetaTableId);
    oid_t first = metaTable->firstRow;
    oid_t last = metaTable->lastRow;
    int nServerTables = metaTable->nRows;
    oid_t tableId = first;
    db->pool.unfix(metaTable);

    dbTableDescriptor* desc;

    for (int i = 0; i < nClientTables; i++) { 
        dbTable* clientTable = (dbTable*)data;
        for (desc = db->tables; desc != NULL; desc = desc->nextDbTable) { 
            if (STRCMP(desc->name, (char_t*)((byte*)clientTable + clientTable->name.offs)) == 0) {
                if (!desc->equal(clientTable, true)) {
                    goto Return;
                }
                descriptors[i] = desc;        
                break;
            }
        }
        if (desc == NULL) { 
            desc = new dbTableDescriptor(clientTable);
            descriptors[i] = desc;
            int n = nServerTables;
            while (--n >= 0) {
                dbTable* table = (dbTable*)db->getRow(tie, tableId);
                oid_t next = table->next;
                if (STRCMP(desc->name, (char_t*)((byte*)table + table->name.offs)) == 0) {
                    if (!desc->equal(table, db->preserveExistedIndices)) {
                        db->modified = true;
                        if (table->nRows == 0) {
                            TRACE_MSG((STRLITERAL("Replace definition of table '%s'\n"), desc->name));
                            desc->match(table, true, db->preserveExistedIndices, true);
                            db->updateTableDescriptor(desc, tableId, table);
                        } else {
                            db->reformatTable(tableId, desc);
                        }
                    } else {
                        db->linkTable(desc, tableId);
                    }
                    desc->setFlags();
                    break;
                }
                if (tableId == last) {
                    tableId = first;
                } else { 
                    tableId = next;
                }
            }
            if (n < 0) { // no match found
                if (db->accessType == dbDatabase::dbReadOnly || db->accessType == dbDatabase::dbMulticlientReadOnly ) {
                    db->handleError(dbDatabase::DatabaseOpenError, "New table definition can not be added to read only database");
                    return false;
                } else {
                    TRACE_MSG((STRLITERAL("Create new table '%s' in database\n"), desc->name));
                    db->addNewTable(desc);
                    db->modified = true;
                }
            }
            if (db->accessType != dbDatabase::dbReadOnly && db->accessType != dbDatabase::dbMulticlientReadOnly) {
                db->addIndices(desc);
            }
        } 
        data += clientTable->size;
    }
    for (desc = db->tables; desc != NULL; desc = desc->nextDbTable) { 
        for (dbFieldDescriptor *fd = desc->firstField; fd != NULL; fd = fd->nextField) 
        {
            if (fd->refTable != NULL) { 
                fd->refTable = db->lookupTable(fd->refTable);
            }
        }
        desc->checkRelationship();
    }
    db->commit();
  Return:
    bool rc = session->sock->write(descriptors, nClientTables*sizeof(dbTableDescriptor*));
    delete[] descriptors;
    return rc;
}

void dbServer::serveClient()
{
    dbStatement *sp, **spp;
    db->attach();
    while (true) {
        dbClientSession* session;
        {
            dbCriticalSection cs(mutex);
            do {
                go.wait(mutex);
                if (cancelWait) {
                    nIdleThreads -= 1;
                    done.signal();
                    db->detach();
                    return;
                }
            } while (waitList == NULL);

            session = waitList;
            waitList = waitList->next;
            session->next = activeList;
            activeList = session;
            nIdleThreads -= 1;
            nActiveThreads += 1;
            waitListLength -= 1;
        }
        cli_request req;
        int4 response = cli_ok;
        bool online = true;
        bool authenticated = false;
        while (online && session->sock->read(&req, sizeof req)) {
            req.unpack();
            
#ifdef SECURE_SERVER
//------------------------------------------------------------------------------------------------------------
// validations added to give more robustness to server.cpp (GigaBASE 2.46)
// step 1 check cmd

            if ((unsigned)req.cmd >= (unsigned)cli_cmd_last) break;

// step 2 check msg lengths depending on cmd

            if (req.cmd == cli_cmd_login || req.cmd == cli_cmd_describe_table ||
                req.cmd == cli_cmd_prepare_and_execute || req.cmd == cli_cmd_execute ||
                req.cmd == cli_cmd_prepare_and_insert || req.cmd == cli_cmd_insert ||
                req.cmd == cli_cmd_update || req.cmd == cli_cmd_create_table || req.cmd == cli_cmd_alter_table || 
                req.cmd == cli_cmd_seek || req.cmd == cli_cmd_skip ||
                req.cmd == cli_cmd_drop_table || req.cmd == cli_cmd_alter_index ||
                req.cmd == cli_cmd_insert_cpp || req.cmd == cli_cmd_update_cpp ||
                req.cmd == cli_cmd_remove_cpp || req.cmd == cli_cmd_select_cpp ||
                req.cmd == cli_cmd_remove_cond || req.cmd == cli_cmd_reload_schema)
            {
                 if (req.length == sizeof(cli_request)) break;
            } else if (req.length != sizeof(cli_request)) break;
            
// step 3 check spurious login

            if (req.cmd == cli_cmd_login && authenticated) break;
//------------------------------------------------------------------------------------------------------------
#endif
            int length = req.length - sizeof(req);
            dbSmallBuffer<char> msg(length);
            if (length > 0) {
                if (!session->sock->read(msg, length)) { //added to catch read problems
                    response = cli_network_error;
                    pack4(response);
                    session->sock->write(&response, sizeof response);
                    break;
                }

#ifdef SECURE_SERVER
//------------------------------------------------------------------------------------------------------------
// step 4 msg signature checking. Offers some degree of protection against naive crackers or worms
// I agree with you that security layer should be the correct answer. priority devel?

                int i, s = req.length + req.cmd + req.stmt_id;
                char *p = (char *)msg;
                for (i = 0; i < length; i++, p++) {
                   s += (*p << 7) + (*p << 3) + i;
//                 s += *p + i;  alternative more simple checking if above can't be
//                               implemented on other languages
                }
                if (s != req.sig) break;
//------------------------------------------------------------------------------------------------------------
#endif
            }
            
#ifdef SECURE_SERVER
//------------------------------------------------------------------------------------------------------------
// step 5 check in advance stmt_id's, if aren't required cli.cpp sets them to 0
// see cli.cpp changes in code about pack() position just before sock->write ....

            if (req.cmd == cli_cmd_login || req.cmd == cli_cmd_close_session ||
                req.cmd == cli_cmd_describe_table || req.cmd == cli_cmd_show_tables ||
                req.cmd == cli_cmd_commit || req.cmd == cli_cmd_precommit ||
                req.cmd == cli_cmd_abort || req.cmd == cli_cmd_create_table || req.cmd == cli_cmd_alter_table ||  
                req.cmd == cli_cmd_drop_table || req.cmd == cli_cmd_alter_index) 
            {
                if (req.stmt_id != 0) break;

// otherwise search specific stmt_id to verify existence
// I am not sure if this is really necessary (or if an ilegall stm potentially threats the server)

            } else {
                for (spp = &session->stmts; (sp = *spp) != NULL; spp = &sp->next)
                {
                    if (sp->id == req.stmt_id) break;
                }
                if (req.cmd != cli_cmd_prepare_and_execute &&
                    req.cmd != cli_cmd_prepare_and_insert) {  // check others than prepares
                    if (spp != NULL && *spp == NULL) { // not found -> spurious stmt_id
                        response = cli_network_error;  // maybe a programming bug, so give error code
                        pack4(response);               // disconnect anyway.
                        session->sock->write(&response, sizeof response);
                        break;
                    }
                }
            }

// step 6 check login state

            if (req.cmd != cli_cmd_login && !authenticated)
            {
                response = cli_login_failed;
                pack4(response);
                online = session->sock->write(&response, sizeof response);
                break;
            }
//------------------------------------------------------------------------------------------------------------
#endif
            switch(req.cmd) {
              case cli_cmd_insert_cpp:
                online = insert_cpp(session, msg);
                break;
              case cli_cmd_update_cpp:
                online = update_cpp(session, msg);
                break;
              case cli_cmd_remove_cpp:
                online = remove_cpp(session, msg);
                break;
              case cli_cmd_remove_cond:
                online = remove_cond(session, msg);
                break;
              case cli_cmd_select_cpp:
                online = select_cpp(session, msg);
                break;
              case cli_cmd_reload_schema:
                online = reload_schema(session, msg);
                break;         
              case cli_cmd_lock:
                db->lock();
                break;
               case cli_cmd_unlock:
                db->precommit();
                break;
               case cli_cmd_commit_async:
                db->commit();
                break;
               case cli_cmd_rollback_async:
                db->rollback();
                break;
              case cli_cmd_login:
                if (authenticate(msg)) {
                    authenticated = true;
                    response = cli_ok;
                } else {
                    online = false;
                    response = cli_login_failed;
                }
                pack4(response);
                online = session->sock->write(&response, sizeof response);
                break;
              case cli_cmd_close_session:
                while (session->dropped_tables != NULL) {
                    dbTableDescriptor* next = session->dropped_tables->nextDbTable;
                    delete session->dropped_tables;
                    session->dropped_tables = next;
                }
                db->commit();
                session->in_transaction = false;
                online = false;
                break;
              case cli_cmd_prepare_and_execute:
                online = select(session, req.stmt_id, msg, true);
                session->in_transaction = true;
                break;
              case cli_cmd_execute:
                online = select(session, req.stmt_id, msg, false);
                break;
              case cli_cmd_get_first:
                online = get_first(session, req.stmt_id);
                break;
              case cli_cmd_get_last:
                online = get_last(session, req.stmt_id);
                break;
              case cli_cmd_get_next:
                online = get_next(session, req.stmt_id);
                break;
              case cli_cmd_get_prev:
                online = get_prev(session, req.stmt_id);
                break;
              case cli_cmd_skip:
                online = skip(session, req.stmt_id, msg);
                break;
              case cli_cmd_seek:
                online = seek(session, req.stmt_id, msg);
                break;
              case cli_cmd_freeze:
                online = freeze(session, req.stmt_id);
                break;
              case cli_cmd_unfreeze:
                online = unfreeze(session, req.stmt_id);
                break;
              case cli_cmd_free_statement:
                for (spp = &session->stmts; (sp = *spp) != NULL; spp = &sp->next)
                {
                    if (sp->id == req.stmt_id) {
                        *spp = sp->next;
                        delete sp;
                        break;
                    }
                }
                break;
              case cli_cmd_abort:
                while (session->dropped_tables != NULL) {
                    dbTableDescriptor* next = session->dropped_tables->nextDbTable;
                    db->linkTable(session->dropped_tables, session->dropped_tables->tableId);
                    session->dropped_tables = next;
                }
                if (session->existed_tables != NULL) { 
                    while (db->tables != session->existed_tables) { 
                        dbTableDescriptor* table = db->tables;
                        db->unlinkTable(table);
                        delete table;
                    }
                    session->existed_tables = NULL;
                }
                db->rollback();
                session->in_transaction = false;
                online = session->sock->write(&response, sizeof response);
                break;
              case cli_cmd_commit:
                while (session->dropped_tables != NULL) {
                    dbTableDescriptor* next = session->dropped_tables->nextDbTable;
                    delete session->dropped_tables;
                    session->dropped_tables = next;
                }
                session->existed_tables = NULL;
                db->commit();
                session->in_transaction = false;
                online = session->sock->write(&response, sizeof response);
                break;
              case cli_cmd_precommit:
                db->precommit();
                online = session->sock->write(&response, sizeof response);
                break;
              case cli_cmd_update:
                online = update(session, req.stmt_id, msg);
                break;
              case cli_cmd_remove:
                online = remove(session, req.stmt_id);
                break;
              case cli_cmd_remove_current:
                online = remove_current(session, req.stmt_id);
                break;          
              case cli_cmd_prepare_and_insert:
                online = insert(session, req.stmt_id, msg, true);
                session->in_transaction = true;
                break;
              case cli_cmd_insert:
                online = insert(session, req.stmt_id, msg, false);
                break;
              case cli_cmd_describe_table:
                online = describe_table(session, msg);
                break;
              case cli_cmd_show_tables:
                online = show_tables(session);
                break;
              case cli_cmd_create_table:
                online = update_table(session, msg, true);
                break;
              case cli_cmd_alter_table:
                online = update_table(session, msg, false);
                break;
              case cli_cmd_drop_table:
                online = drop_table(session, msg);
                break;
              case cli_cmd_alter_index:
                online = alter_index(session, msg);
                break;
            }
        }
        if (session->in_transaction) {
            while (session->dropped_tables != NULL) {
                dbTableDescriptor* next = session->dropped_tables->nextDbTable;
                db->linkTable(session->dropped_tables, session->dropped_tables->tableId);
                session->dropped_tables = next;
            }
            if (session->existed_tables != NULL) { 
                while (db->tables != session->existed_tables) { 
                    dbTableDescriptor* table = db->tables;
                    db->unlinkTable(table);
                    delete table;
                }
                session->existed_tables = NULL;
            }
            db->rollback();
        }
        // Finish session
        {
            dbCriticalSection cs(mutex);
            dbClientSession** spp;
            delete session->sock;
            for (spp = &activeList; *spp != session; spp = &(*spp)->next);
            *spp = session->next;
            session->next = freeList;
            freeList = session;
            nActiveThreads -= 1;
            if (cancelSession) {
                done.signal();
                break;
            }
            if (nActiveThreads + nIdleThreads >= optimalNumberOfThreads) {
                break;
            }
            nIdleThreads += 1;
        }
    }
    db->detach();
}

void dbServer::acceptConnection(socket_t* acceptSock)
{
    while (true) {
        socket_t* sock = acceptSock->accept();
        dbCriticalSection cs(mutex);
        if (cancelAccept) {
            return;
        }
        if (sock != NULL) {
            if (freeList == NULL) {
                freeList = new dbClientSession;
                freeList->next = NULL;
            }
            dbClientSession* session = freeList;
            freeList = session->next;
            session->sock = sock;
            session->stmts = NULL;
            session->next = waitList;
            session->in_transaction = false;
            session->existed_tables = NULL;
            session->dropped_tables = NULL;
            waitList = session;
            waitListLength += 1;
            if (nIdleThreads < waitListLength) {
                dbThread thread;
                nIdleThreads += 1;
                thread.create(serverThread, this);
                thread.detach();
            }
            go.signal();
        }
    }
}

dbServer::~dbServer()
{
    dbServer** spp;
    for (spp = &chain; *spp != this; spp = &(*spp)->next);
    *spp = next;
    delete globalAcceptSock;
    delete localAcceptSock;
    delete[] URL;
#ifdef UNICODE
    delete[] address;
#endif
}



int dbQueryScanner::get()
{
    int i = 0, ch, digits;
    char numbuf[64];

    do {
        ch = unpack_char(p);
        p += sizeof(char_t);
        if (ch == '\0') {
            return tkn_eof;
        }
    } while (ch > 0 && ch <= 32);

    if (ch == '*') {
        return tkn_all;
    } else if ((ch >= '0' && ch <= '9') || ch == '+' || ch == '-') {
        do {
            numbuf[i++] = (char)ch;
            if ((size_t)i == sizeof numbuf) {
                // Numeric constant too long
                return tkn_error;
            }
            ch = unpack_char(p);
            p += sizeof(char_t);
        } while (ch != '\0'
                 && ((ch >= '0' && ch <= '9') || ch == '+' || ch == '-' || ch == 'e' ||
                     ch == 'E' || ch == '.'));
        p -= sizeof(char_t);
        numbuf[i] = '\0';
        if (sscanf(numbuf, INT8_FORMAT "%n", &ival, &digits) != 1) {
            // Bad integer constant
            return tkn_error;
        }
        if (digits != i) {
            if (sscanf(numbuf, "%lf%n", &fval, &digits) != 1 || digits != i) {
                // Bad float constant
                return tkn_error;
            }
            return tkn_fconst;
        }
        return tkn_iconst;
    } else if (ISALNUM(ch) || ch == '$' || ch == '_') {
        do {
            buf[i++] = ch;
            if (i == dbQueryMaxIdLength) {
                // Identifier too long
                return tkn_error;
            }
            ch = unpack_char(p);
            p += sizeof(char_t);
        } while (ch != T_EOF && (ISALNUM(ch) || ch == '$' || ch == '_'));
        p -= sizeof(char_t);
        buf[i] = '\0';
        ident = buf;
        return dbSymbolTable::add(ident, tkn_ident);
    } else {
        // Invalid symbol
        return tkn_error;
    }
}



END_GIGABASE_NAMESPACE


