//-< ZIPFILE.H >----------------------------------------------------*--------*
// GigaBASE                  Version 1.0         (c) 1999  GARRET    *     ?  *
// (Post Relational Database Management System)                      *   /\|  *
//                                                                   *  /  \  *
//                          Created:     07-Jul-2005  K.A. Knizhnik  * / [] \ *
//                          Last update: 07-Jul-2005  K.A. Knizhnik  * GARRET *
//-------------------------------------------------------------------*--------*
// Compressed file
//-------------------------------------------------------------------*--------*

#ifndef __ZIPFILE_H__
#define __ZIPFILE_H__

#include "file.h"

BEGIN_GIGABASE_NAMESPACE

/**
 * Compressed file
 */
class GIGABASE_DLL_ENTRY dbZipFile : public dbOSFile {    
    int*   segments;
    size_t segmentSize;
    int    nSegments;
    int    currSeg;
    char*  compressedBuffer;
    char*  decompressedBuffer;
    size_t decompressedSize;

  public:
    dbZipFile();
    ~dbZipFile();
    
    virtual int open(char_t const* fileName, int attr);

    virtual char_t* errorText(int code, char_t* buf, size_t bufSize);
    virtual int close();

    virtual int write(offs_t pos, void const* ptr, size_t size);
    virtual int read(offs_t pos, void* ptr, size_t size);
};

END_GIGABASE_NAMESPACE

#endif
