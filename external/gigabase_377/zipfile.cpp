//-< ZIPFILE.CPP >---------------------------------------------------*--------*
// GigaBASE                  Version 1.0         (c) 1999  GARRET    *     ?  *
// (Post Relational Database Management System)                      *   /\|  *
//                                                                   *  /  \  *
//                          Created:     07-Jul-2005  K.A. Knizhnik  * / [] \ *
//                          Last update: 07-Jul-2005  K.A. Knizhnik  * GARRET *
//-------------------------------------------------------------------*--------*
// Compressed file
//-------------------------------------------------------------------*--------*

#define INSIDE_GIGABASE

#include <zlib.h>
#include "gigabase.h"
#include "cli.h"
#include "cliproto.h"
#include "zipfile.h"

    
BEGIN_GIGABASE_NAMESPACE

#define Z_READ_ONLY (-7)

dbZipFile::dbZipFile()
{
    segments = NULL;
    compressedBuffer = NULL;
    decompressedBuffer = NULL;
    segmentSize = 0;
    currSeg = -1;
}

dbZipFile::~dbZipFile()
{
    delete[] segments;
    delete[] compressedBuffer;
    delete[] decompressedBuffer;
}


int dbZipFile::open(char_t const* fileName, int attr)
{
    int rc = dbOSFile::open(fileName, attr|read_only);
    if (rc == ok) { 
        char buf[8];
        rc = dbOSFile::read(0, buf, 4);
        if (rc != ok) {
            return rc;
        }
        int nSegments = unpack4(buf);
        rc = dbOSFile::read(4, buf, 4);
        if (rc != ok) {
            return rc;
        }
        segmentSize = unpack4(buf);
        segments = new db_int4[nSegments+1];
        rc = dbOSFile::read(8, segments, nSegments*sizeof(db_int4));
        if (rc != ok) {
            return rc;
        }
        size_t offs = (nSegments+2)*4;
        for (int i = 0; i < nSegments; i++) { 
            size_t size = unpack4((char*)&segments[i]);
            segments[i] = offs;
            offs += size;
        }
        segments[nSegments] = offs;
        currSeg = -1;
        compressedBuffer = new char[segmentSize]; 
        decompressedBuffer = new char[segmentSize]; 
    }
    return rc;
}

int dbZipFile::close()
{
    delete[] segments;
    segments = NULL;
    delete[] compressedBuffer;
    compressedBuffer = NULL;
    delete[] decompressedBuffer;
    decompressedBuffer = NULL;
    return dbOSFile::close();
}

int dbZipFile::write(offs_t pos, void const* ptr, size_t size)
{
    return Z_READ_ONLY;
}

int dbZipFile::read(offs_t pos, void* ptr, size_t size)
{
    int seg = (int)(pos / segmentSize);
    size_t offs = (size_t)(pos - (offs_t)seg*segmentSize);
    if (seg != currSeg) { 
        size_t compressedSize = (size_t)(segments[seg+1] - segments[seg]);
        int rc = dbOSFile::read(segments[seg], compressedBuffer, compressedSize);
        if (rc != ok) {
            return rc;
        }
        currSeg = seg;
        uLongf decompressedBufferSize = segmentSize;
        rc = uncompress((Bytef*)decompressedBuffer, &decompressedBufferSize, (Bytef*)compressedBuffer, compressedSize);
        if (rc != Z_OK) { 
            return rc;
        }
        decompressedSize = decompressedBufferSize;
    }
    if (offs + size > decompressedSize) { 
        return eof;
    }
    memcpy(ptr, decompressedBuffer + offs, size);
    return ok;    
}

char_t* dbZipFile::errorText(int code, char_t* buf, size_t bufSize)
{
    char* msg;
    switch (code) {
      case Z_STREAM_ERROR:
        msg = "Stream error";
        break;
      case Z_DATA_ERROR:
        msg = "Data error";
        break;
      case Z_MEM_ERROR:
        msg = "Memory error";
        break;
      case Z_BUF_ERROR:
        msg = "Buffer error";
        break;
      case Z_VERSION_ERROR:
        msg = "Version error";
        break;
      case Z_READ_ONLY:        
        msg = "File is read only";
        break;
      default:
        return dbOSFile::errorText(code, buf, bufSize);
    }
    STRNCPY(buf, STRLITERAL("No error"), bufSize-1);
    buf[bufSize-1] = '\0';
    return buf;
}

END_GIGABASE_NAMESPACE
