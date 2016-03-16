#define INSIDE_GIGABASE

#include "gigabase.h"
#include "session.h"
#include "cli.h"
#include "cliproto.h"

BEGIN_GIGABASE_NAMESPACE

ConnectionException::ConnectionException(char const* op, socket_t* s) : dbException(dbDatabase::SocketError, op, s != NULL ? s->errcode : 0) {}

CursorException::CursorException(char const* msg) : dbException(dbDatabase::CursorError, msg) {}

void dbSession::login(char_t const* user_name, char_t const* password)
{
    size_t msg_size = sizeof(cli_request) + (STRLEN(user_name) + STRLEN(password) + 2)*sizeof(char_t);    
    dbSmallBuffer<char> buf(msg_size);
    char* p = buf;
    cli_request* req = (cli_request*)p;
    req->length  = (int)msg_size;
    req->cmd     = cli_cmd_login;
    p += sizeof(cli_request);
    p = pack_str(p, user_name);
    p = pack_str(p, password);
    req->pack();
    if (!socket->write(req, msg_size)) {
        throw ConnectionException("socket::write", socket);
    }
    int4 response;
    if (!socket->read(&response, sizeof response)) {
        throw ConnectionException("socket::read", socket);
    }
    unpack4(response);
    if (response != cli_ok) {
        throw ConnectionException("connection rejected by server", NULL);
    }
}

void dbSession::reloadSchema() 
{ 
    dbTableDescriptor* desc;
    size_t schemaSize = sizeof(cli_request) + sizeof(int);
    for (desc = dbTableDescriptor::chain; desc != NULL; desc = desc->next) {
        schemaSize += sizeof(dbTable) + desc->nFields*sizeof(dbField) + desc->totalNamesLength()*sizeof(char_t);
    }
    dbSmallBuffer<char> buf(schemaSize);
    char* p = buf.base();
    cli_request* req = (cli_request*)p;
    req->length  = (int)schemaSize;
    req->cmd     = cli_cmd_reload_schema;
    req->pack();
    p += sizeof(cli_request);
    int* nTablesPtr = (int*)p;
    p += sizeof(int);
    int nTables = 0;
    for (desc = dbTableDescriptor::chain; desc != NULL; desc = desc->next) {
        size_t tableSize = sizeof(dbTable) + desc->nFields*sizeof(dbField) + desc->totalNamesLength()*sizeof(char_t);
        dbTable* table = (dbTable*)p;
        desc->storeInDatabase(table);
        table->size = (nat4)tableSize;        
        p += tableSize;
        desc->tableId = nTables++;
    }
    *nTablesPtr = nTables;
    if (!socket->write(req, schemaSize)) { 
        throw ConnectionException("socket::write", socket);
    }
    tables = new dbTableDescriptor*[nTables];
    if (!socket->read(tables, nTables*sizeof(dbTableDescriptor*))) { 
        throw ConnectionException("socket::read", socket);
    }    
    for (int i = 0; i < nTables; i++) { 
        if (tables[i] == NULL) { 
            throw ConnectionException("Database schema can not be updated", NULL);
        }
    }
}

dbSession::dbSession(const char* address, 
                     int max_connect_attempts,
                     int reconnect_timeout_sec, 
                     char_t const* user_name,
                     char_t const* password)
{
    socket = socket_t::connect(address,
                               socket_t::sock_any_domain,
                               max_connect_attempts,
                               reconnect_timeout_sec);
    if (!socket->is_ok()) {
        throw ConnectionException("socket::connect", socket);
    }
    login(user_name, password);
    reloadSchema();    
    selected = false;
    currObj = NULL;

}
 
dbSession::~dbSession()
{
    sendCommand(cli_cmd_close_session);
    delete socket;
    delete[] currObj;
    delete[] tables;
}

void dbSession::fillBuffer(size_t size) 
{
    if ((size_t)(bufUsed - bufPos) < size) { 
        memmove(sockBuf, sockBuf + bufPos, bufUsed - bufPos);
        bufUsed -= bufPos;
        bufPos = 0;
        int rc = socket->read(sockBuf + bufUsed, size-bufUsed, SOCKET_BUFFER_SIZE-bufUsed);
        if (rc < (int)(size - bufUsed)) {
            throw ConnectionException("socket::read", socket);
        }
        bufUsed += rc;
    }
}

void dbSession::reset()
{
    if (selected) {
        while (next(NULL, NULL) != 0);
    }
}

oid_t dbSession::next(dbTableDescriptor* table, void* record)
{
    if (!selected) {
        throw CursorException("Cursor is not opened");
    }
    fillBuffer(sizeof(oid_t));
    oid_t currOid = *(oid_t*)&sockBuf[bufPos];
    bufPos += sizeof(oid_t);
    if (currObj != NULL) { 
        delete[] currObj;
        currObj = NULL;
    }
    if (currOid != 0) { 
        fillBuffer(sizeof(int));
        size_t size = *(int*)&sockBuf[bufPos];
        if (size <= SOCKET_BUFFER_SIZE) { 
            fillBuffer(size);
            if (record != NULL) { 
                table->columns->fetchRecordFields((byte*)record, (byte*)&sockBuf[bufPos]);
            }
            bufPos += size;
        } else { 
            currObj = new byte[size];
            size_t available = bufUsed-bufPos;
            memcpy(currObj, &sockBuf[bufPos], available);
            bufUsed = bufPos = 0;
            if (!socket->read(currObj + available, size-available)) { 
                throw ConnectionException("socket::read", socket);
            }
            if (record != NULL) { 
                table->columns->fetchRecordFields((byte*)record, currObj);
            }
        }
    } else {
        selected = false;
    }
    return currOid;
}

oid_t dbSession::insert(dbTableDescriptor* table, void const* record, bool batch)
{
    reset();
    size_t recordSize = table->columns->calculateRecordSize((byte*)record, table->fixedSize);
    size_t reqSize = recordSize + sizeof(cli_request) + 1 + sizeof(dbTableDescriptor*);
    dbSmallBuffer<char> buf(reqSize);
    byte* data = (byte*)buf.base();    
    cli_request* req = (cli_request*)data;
    req->length  = (int)reqSize;
    req->cmd     = cli_cmd_insert_cpp;
    req->pack();
    data += sizeof(cli_request);
    *(dbTableDescriptor**)data = tables[table->tableId];
    data += sizeof(dbTableDescriptor*);
    *data++ = (byte)batch;
    table->columns->storeRecordFields(data, (byte*)record, table->fixedSize, dbFieldDescriptor::Insert);
    ((dbRecord*)data)->size = (nat4)recordSize;
    if (!socket->write(req, (int)reqSize)) {
        throw ConnectionException("socket::write", socket);
    }
    oid_t oid;
    if (!socket->read(&oid, sizeof oid)) {
        throw ConnectionException("socket::read", socket);
    }
    return oid;
}

bool dbSession::update(dbTableDescriptor* table, void const* record, oid_t oid)
{
    reset();
    size_t recordSize = table->columns->calculateRecordSize((byte*)record, table->fixedSize);
    size_t reqSize = recordSize + sizeof(dbTableDescriptor*) + sizeof(oid) + sizeof(cli_request);
    dbSmallBuffer<char> buf(reqSize);
    byte* data = (byte*)buf.base();
    cli_request* req = (cli_request*)data;
    req->length  = (int)reqSize;
    req->cmd     = cli_cmd_update_cpp;
    data += sizeof(cli_request);
    *(dbTableDescriptor**)data = tables[table->tableId];
    data += sizeof(dbTableDescriptor*);
    *(oid_t*)data = oid;    
    data += sizeof(oid_t);
    table->columns->storeRecordFields(data, (byte*)record, table->fixedSize, dbFieldDescriptor::Update);
    ((dbRecord*)data)->size = (nat4)recordSize;
    req->pack();
    if (!socket->write(req, reqSize)) {
        throw ConnectionException("socket::write", socket);
    }
    bool response;
    if (!socket->read(&response, sizeof response)) {
        throw ConnectionException("socket::read", socket);
    }
    return response;
}

void dbSession::sendCommand(int command)
{
    reset();
    cli_request req;
    req.length = sizeof(req);
    req.cmd    = command;
    req.pack();
    if (!socket->write(&req, sizeof(req))) {
        throw ConnectionException("socket::write", socket);
    }    
}

void dbSession::remove(dbTableDescriptor* table, oid_t oid)
{
    reset();
    struct cli_remove_request : cli_request { 
        dbTableDescriptor* desc;
        oid_t oid;
    } req;
    req.length = sizeof(req);
    req.cmd    = cli_cmd_remove_cpp;
    req.desc = table;
    req.oid = oid;
    if (!socket->write(&req, sizeof(req))) {
        throw ConnectionException("socket::write", socket);
    }
}

void dbSession::commit()
{
    sendCommand(cli_cmd_commit_async);
}

void dbSession::rollback()
{
    sendCommand(cli_cmd_rollback_async);
}


void dbSession::unlock()
{
    sendCommand(cli_cmd_unlock);
}

void dbSession::lock()
{
    sendCommand(cli_cmd_lock);
}


void dbSession::sendQuery(dbTableDescriptor* table, dbQuery const& query, size_t limit, int cmd)
{
    dbQueryElement* elem;
    reset();
    size_t size = sizeof(cli_request) + 1 + sizeof(size_t) + sizeof(dbTableDescriptor*);
    size_t len;

    for (elem = query.elements; elem != NULL; elem = elem->next) { 
        size += 1;
        switch (elem->type) { 
          case dbQueryElement::qExpression:
            size += (STRLEN((char_t*)elem->ptr) + 1)*sizeof(char_t);

            break;
          case dbQueryElement::qVarBool:
            size += sizeof(bool);
            break;
          case dbQueryElement::qVarInt1:
            size += sizeof(int1);
            break;
          case dbQueryElement::qVarInt2:
            size += sizeof(int2);
            break;
          case dbQueryElement::qVarInt4:
            size += sizeof(int4);
            break;
          case dbQueryElement::qVarInt8:
            size += sizeof(db_int8);
            break;
          case dbQueryElement::qVarReal4:
            size += sizeof(real4);
            break;
          case dbQueryElement::qVarReal8:
            size += sizeof(real8);
            break;
          case dbQueryElement::qVarString:
            size += (STRLEN((char_t*)elem->ptr) + 1)*sizeof(char_t);
            break;
          case dbQueryElement::qVarStringPtr:
            size += (STRLEN(*(char_t**)elem->ptr) + 1)*sizeof(char_t);
            break;
          case dbQueryElement::qVarReference:
            size += sizeof(oid_t);
            break;
          case dbQueryElement::qVarArrayOfRef:
            size += ((dbAnyArray*)elem->ptr)->length() * sizeof(oid_t) + sizeof(size_t);
            break;
          case dbQueryElement::qVarArrayOfRefPtr:
            size += (*(dbAnyArray**)elem->ptr)->length() * sizeof(oid_t) + sizeof(size_t);
            break;
          case dbQueryElement::qVarRectangle:
            size += sizeof(rectangle);
            break;
          default:
            assert(false);
        }
    }
    dbSmallBuffer<char> buf(size);
    byte* data = (byte*)buf.base();
    cli_request* req = (cli_request*)data;
    req->length  = (int)size;
    req->cmd     = cmd;
    req->pack();
    data += sizeof(cli_request);
    *(dbTableDescriptor**)data = tables[table->tableId];
    data += sizeof(dbTableDescriptor*);
    *(size_t*)data = limit;
    data += sizeof(size_t);

    for (elem = query.elements; elem != NULL; elem = elem->next) { 
        *data++ = elem->type;
        switch (elem->type) { 
          case dbQueryElement::qExpression:
            len = (STRLEN((char_t*)elem->ptr) + 1)*sizeof(char_t);
            memcpy(data, elem->ptr, len);
            data += len;
            break;
          case dbQueryElement::qVarBool:
            *data++ = (byte)*(bool*)elem->ptr;
            break;
          case dbQueryElement::qVarInt1:
            *data++ = *(byte*)elem->ptr;
            break;
          case dbQueryElement::qVarInt2:
            *(int2*)data = *(int2*)elem->ptr;
            data += sizeof(int2);
            break;
          case dbQueryElement::qVarInt4:
            *(int4*)data = *(int4*)elem->ptr;
            data += sizeof(int4);
            break;
          case dbQueryElement::qVarInt8:
            *(db_int8*)data = *(db_int8*)elem->ptr;
            data += sizeof(db_int8);
            break;
          case dbQueryElement::qVarReal4:
            *(real4*)data = *(real4*)elem->ptr;
            data += sizeof(real4);
            break;
          case dbQueryElement::qVarReal8:
            *(real8*)data = *(real8*)elem->ptr;
            data += sizeof(real8);
            break;
          case dbQueryElement::qVarString:
            len = (STRLEN((char_t*)elem->ptr) + 1)*sizeof(char_t);
            memcpy(data, elem->ptr, len);
            data += len;
            break;
          case dbQueryElement::qVarStringPtr:
            len = (STRLEN(*(char_t**)elem->ptr) + 1)*sizeof(char_t);
            memcpy(data, *(char_t**)elem->ptr, len);
            data += len;
            break;
          case dbQueryElement::qVarReference:
            *(oid_t*)data = *(oid_t*)elem->ptr;
            data += sizeof(oid_t);
            break;
          case dbQueryElement::qVarArrayOfRef:
            len = ((dbAnyArray*)elem->ptr)->length();
            dbAnyArray::arrayAllocator((dbAnyArray*)data, NULL, len);
            data += sizeof(dbAnyArray) + sizeof(void*);
            memcpy(data, ((dbAnyArray*)elem->ptr)->base(), len*sizeof(oid_t));
            data += len*sizeof(oid_t);
            break;
          case dbQueryElement::qVarArrayOfRefPtr:
            len = (*(dbAnyArray**)elem->ptr)->length();
            dbAnyArray::arrayAllocator((dbAnyArray*)data, NULL, len);
            data += sizeof(dbAnyArray) + sizeof(void*);
            memcpy(data, (*(dbAnyArray**)elem->ptr)->base(), len*sizeof(oid_t));
            data += len*sizeof(oid_t);
            break;
          case dbQueryElement::qVarRectangle:
            *(rectangle*)data = *(rectangle*)elem->ptr;
            data += sizeof(rectangle);
            break;
          default:
            assert(false);
        }
    }
    *data = -1;
    if (!socket->write(req, size)) {
        throw ConnectionException("socket::write", socket);
    }
}

void dbSession::select(dbTableDescriptor* table, dbQuery const& query, size_t limit)
{
    sendQuery(table, query, limit, cli_cmd_select_cpp);
    bufUsed = bufPos = 0;
    selected = true;
}

size_t dbSession::remove(dbTableDescriptor* table, dbQuery const& query, size_t limit)
{
    sendQuery(table, query, limit, cli_cmd_remove_cond);
    size_t n;
    if (!socket->read(&n, sizeof n)) {
        throw ConnectionException("socket::read", socket);
    }
    return n;
}
           
END_GIGABASE_NAMESPACE

