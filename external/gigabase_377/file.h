//-< FILE.H >--------------------------------------------------------*--------*
// GigaBASE                  Version 1.0         (c) 1999  GARRET    *     ?  *
// (Post Relational Database Management System)                      *   /\|  *
//                                                                   *  /  \  *
//                          Created:     20-Nov-98    K.A. Knizhnik  * / [] \ *
//                          Last update: 30-Jan-99    K.A. Knizhnik  * GARRET *
//-------------------------------------------------------------------*--------*
// System independent intrface to operating system file
//-------------------------------------------------------------------*--------*

#ifndef __FILE_H__
#define __FILE_H__

BEGIN_GIGABASE_NAMESPACE

const size_t dbDefaultRaidBlockSize = 1024*1024;

/**
 * GigaBASE file interface
 */
class GIGABASE_DLL_ENTRY dbFile {
  public:
    enum ReturnStatus {
        ok  = 0,
        eof = -1, // number of read/written bytes is smaller than requested
        not_supported = -2
    };
    enum OpenAttributes {
        read_only       = 0x01, // open file in read-only mode
        truncate        = 0x02, // truncate file when opened
        sequential      = 0x04, // optimize for sequenial access 
        no_buffering    = 0x08, // do not use OS file cache
        no_sync         = 0x10, // do not flush data to the disk
        shared          = 0x20, // shared access to the file
        write_through   = 0x40, // write through 
        delete_on_close = 0x80  // delete file on close
    };
    virtual int open(char_t const* fileName, int attr) = 0;
    virtual ~dbFile();

    virtual int flush() = 0;
    virtual int close() = 0;
    
    enum LockType { 
        lck_shared,
        lck_exclusive
    };
        
    virtual int lock(LockType lck) = 0;
    virtual int unlock() = 0;

    virtual int setSize(offs_t offs) = 0;

    virtual int write(offs_t pos, void const* ptr, size_t size) = 0;
    virtual int read(offs_t pos, void* ptr, size_t size) = 0;

    virtual char_t* errorText(int code, char_t* buf, size_t bufSize) = 0;
};

class GIGABASE_DLL_ENTRY dbOSFile : public dbFile {
  protected:
#if defined(__SYMBIAN32__)
    RFile file;
#elif defined(_WIN32)
    HANDLE  fh;
#else
    int     fd;
#endif
    bool    noSync;
    dbMutex mutex;
  public:
    int open(char_t const* fileName, int attr);
    virtual int write(void const* ptr, size_t size);
    virtual int read(void* ptr, size_t size);

    virtual int lock(LockType lck);
    virtual int unlock();
   
    dbOSFile();

    int flush();
    int close();

    int setSize(offs_t offs);

    int write(offs_t pos, void const* ptr, size_t size);
    int read(offs_t pos, void* ptr, size_t size);

    static void* allocateBuffer(size_t bufferSize, bool lock = false);
    static void  deallocateBuffer(void* buffer, size_t size = 0, bool unlock = false);
    static void  protectBuffer(void* buf, size_t bufSize, bool readonly);

    static size_t ramSize();

    char_t* errorText(int code, char_t* buf, size_t bufSize);
};

/**
 * File consisting of multiple segments
 */
class GIGABASE_DLL_ENTRY dbMultiFile : public dbOSFile {
  public:
    struct dbSegment {
        char_t* name;
        offs_t  size;
        offs_t  offs;
    };

    int open(int nSegments, dbSegment* segments, int attr);

    virtual int setSize(offs_t offs);

    virtual int flush();
    virtual int close();

    virtual int write(offs_t pos, void const* ptr, size_t size);
    virtual int read(offs_t pos, void* ptr, size_t size);

    dbMultiFile() { segment = NULL; }
    ~dbMultiFile() {}

  protected:
    class dbFileSegment : public dbOSFile {
      public:
        offs_t size;
        offs_t offs;
    };
    int            nSegments;
    dbFileSegment* segment;
};

/*
 * RAID-1 file. Scattern file blocks between several physical segments
 */
class GIGABASE_DLL_ENTRY dbRaidFile : public dbMultiFile {
    size_t raidBlockSize;
  public:
    dbRaidFile(size_t blockSize) { 
        raidBlockSize = blockSize;
    }

    virtual int setSize(offs_t offs);

    virtual int write(offs_t pos, void const* ptr, size_t size);
    virtual int read(offs_t pos, void* ptr, size_t size);
};    

END_GIGABASE_NAMESPACE

#endif




