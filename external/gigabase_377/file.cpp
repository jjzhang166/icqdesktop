//-< FILE.CPP >------------------------------------------------------*--------*
// GigaBASE                  Version 1.0         (c) 1999  GARRET    *     ?  *
// (Post Relational Database Management System)                      *   /\|  *
//                                                                   *  /  \  *
//                          Created:     20-Nov-98    K.A. Knizhnik  * / [] \ *
//                          Last update: 10-Dec-98    K.A. Knizhnik  * GARRET *
//-------------------------------------------------------------------*--------*
// System dependent implementation of mapped on memory file
//-------------------------------------------------------------------*--------*

#ifdef __SYMBIAN32__
#include <e32err.h>
#include <COEMAIN.H> 
#endif

#define INSIDE_GIGABASE

#define __EXTENSIONS__
#define _EXTENSIONS
#define _FILE_OFFSET_BITS 64
#if ! defined(HPUX11_NOT_ITANIUM) && ! defined(L64)
#define _LARGEFILE64_SOURCE 1 // access to files greater than 2Gb in Solaris
#define _LARGE_FILE_API     1 // access to files greater than 2Gb in AIX
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE           // for definition of O_DIRECT on Linux
#endif

#include "gigabase.h"

#if !defined(O_LARGEFILE) && !defined(aix64) && (!defined(SOLARIS) || defined(SOLARIS64))
#define O_LARGEFILE 0
#endif

BEGIN_GIGABASE_NAMESPACE

dbFile::~dbFile()
{
}

#if defined(_WIN32) && !defined(__SYMBIAN32__)
#if defined(SPARSE_FILE_OPTIMIZATION) && (_WIN32_WINNT >= 0x0500)
#include <Winioctl.h>
#endif

class OS_info : public OSVERSIONINFO {
  public:
    OS_info() {
        dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(this);
    }
};

static OS_info osinfo;

#define BAD_POS 0xFFFFFFFF // returned by SetFilePointer and GetFileSize


dbOSFile::dbOSFile()
{
    fh = INVALID_HANDLE_VALUE;
}

int dbOSFile::open(char_t const* fileName, int attr)
{
    noSync = (attr & no_sync) != 0;
    fh = CreateFile(fileName, (attr & read_only)
                    ? GENERIC_READ : GENERIC_READ|GENERIC_WRITE, 
                    (attr & shared) 
                     ? (FILE_SHARE_READ|FILE_SHARE_WRITE) 
                     : (attr & read_only) ? FILE_SHARE_READ : 0, 
                    NULL,
                    (attr & read_only) ? OPEN_EXISTING : (attr & truncate) ? CREATE_ALWAYS : OPEN_ALWAYS,
#ifdef _WINCE
                    FILE_ATTRIBUTE_NORMAL,
#else
                    ((attr & no_buffering) ? FILE_FLAG_NO_BUFFERING : 0) 
                    | ((attr & write_through) ? FILE_FLAG_WRITE_THROUGH : 0)
                    | ((attr & delete_on_close) ? FILE_FLAG_DELETE_ON_CLOSE : 0)
                    | ((attr & sequential) ? FILE_FLAG_SEQUENTIAL_SCAN : FILE_FLAG_RANDOM_ACCESS),
#endif
                    NULL);
    if (fh == INVALID_HANDLE_VALUE) {
        return GetLastError();
    }
#ifndef _WINCE
    if (attr & no_buffering) { 
         noSync = true;
    }
#endif
#if defined(SPARSE_FILE_OPTIMIZATION) && (_WIN32_WINNT >= 0x0500)
    if (!(attr & read_only)) {
        DWORD bytes_returned;
        if (!DeviceIoControl(fh, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &bytes_returned, NULL)) {
            return GetLastError();
        }
    }
#endif
    return ok;
}

int dbOSFile::lock(LockType lck)
{
#ifndef _WINCE
    OVERLAPPED ovl;
    ovl.Offset = 0;
    ovl.OffsetHigh = 0;
    ovl.hEvent = 0;
    if (!LockFileEx(fh, lck == lck_shared ? 0 : LOCKFILE_EXCLUSIVE_LOCK, 0, 1, 0, &ovl)) { 
        return GetLastError();
    }
#endif
    return ok;
}

int dbOSFile::unlock()
{
#ifndef _WINCE
    OVERLAPPED ovl;
    ovl.Offset = 0;
    ovl.OffsetHigh = 0;
    ovl.hEvent = 0;
    if (!UnlockFileEx(fh, 0, 1, 0, &ovl)) { 
        return GetLastError();
    }
#endif
    return ok;
}

int dbOSFile::read(offs_t pos, void* buf, size_t size)
{
    DWORD readBytes;
    if (osinfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        OVERLAPPED Overlapped;
        Overlapped.Offset = nat8_low_part(pos);
        Overlapped.OffsetHigh = nat8_high_part(pos);
        Overlapped.hEvent = NULL;
        if (ReadFile(fh, buf, (DWORD)size, &readBytes, &Overlapped)) {
            return readBytes == size ? ok : eof;
        } else {
            int rc = GetLastError();
            return (rc == ERROR_HANDLE_EOF) ? eof : rc;
        }
    } else {
        LONG high_pos = nat8_high_part(pos);
        LONG low_pos = nat8_low_part(pos);
        dbCriticalSection cs(mutex);
        if (SetFilePointer(fh, low_pos,
                           &high_pos, FILE_BEGIN) != BAD_POS
            && ReadFile(fh, buf, (DWORD)size, &readBytes, NULL))
        {
            return (readBytes == size) ? ok : eof;
        } else {
            int rc = GetLastError();
            return rc == ERROR_HANDLE_EOF ? eof : rc;
        }
    }
}

int dbOSFile::read(void* buf, size_t size)
{
    DWORD readBytes;
    if (ReadFile(fh, buf, (DWORD)size, &readBytes, NULL)) {
        return (readBytes == size) ? ok : eof;
    } else {
        int rc = GetLastError();
        return rc == ERROR_HANDLE_EOF ? eof : rc;
    }
}

int dbOSFile::setSize(offs_t size)
{
    LONG low_pos = nat8_low_part(size);
    LONG high_pos = nat8_high_part(size);
    if (SetFilePointer(fh, low_pos,
                       &high_pos, FILE_BEGIN) == BAD_POS
        || !SetEndOfFile(fh))
    {
        return GetLastError();
    }
    return ok;    
}


int dbOSFile::write(void const* buf, size_t size)
{
    DWORD writtenBytes;
    return !WriteFile(fh, buf, (DWORD)size, &writtenBytes, NULL)
        ? GetLastError() : (writtenBytes == size) ? ok : eof;
}

int dbOSFile::write(offs_t pos, void const* buf, size_t size)
{
    DWORD writtenBytes;
    if (osinfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        OVERLAPPED Overlapped;
        Overlapped.Offset = nat8_low_part(pos);
        Overlapped.OffsetHigh = nat8_high_part(pos);
        Overlapped.hEvent = NULL;
        return WriteFile(fh, buf, (DWORD)size, &writtenBytes, &Overlapped)
            ? writtenBytes == size ? ok : eof
            : GetLastError();
    } else {
        LONG high_pos = nat8_high_part(pos);
        LONG low_pos = nat8_low_part(pos);
        dbCriticalSection cs(mutex);
        return SetFilePointer(fh, low_pos, &high_pos, FILE_BEGIN)
            == BAD_POS ||
            !WriteFile(fh, buf, (DWORD)size, &writtenBytes, NULL)
            ? GetLastError()
            : (writtenBytes == size) ? ok : eof;
    }
}


int dbOSFile::flush()
{
    return noSync || FlushFileBuffers(fh) ? ok : GetLastError();
}

int dbOSFile::close()
{
    if (fh != INVALID_HANDLE_VALUE) {
        if (CloseHandle(fh)) {
            fh = INVALID_HANDLE_VALUE;
            return ok;
        } else {
            return GetLastError();
        }
    } else {
        return ok;
    }
}

void* dbOSFile::allocateBuffer(size_t size, bool lock)
{
    void* buf = VirtualAlloc(NULL, size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    if (buf != NULL) {
#ifndef _WINCE 
        if (lock) { 
            SIZE_T minimumWorkingSetSize, maximumWorkingSetSize;
            GetProcessWorkingSetSize(GetCurrentProcess(),
                                     &minimumWorkingSetSize, 
                                     &maximumWorkingSetSize);
            
            if (size > minimumWorkingSetSize) {
                minimumWorkingSetSize = (DWORD)size + 4*1024*1024;
            }
            if (maximumWorkingSetSize < minimumWorkingSetSize) { 
                maximumWorkingSetSize = minimumWorkingSetSize;
            }
            if (!SetProcessWorkingSetSize(GetCurrentProcess(),
                                          minimumWorkingSetSize,
                                          maximumWorkingSetSize)) 
            {
                printf("Failed to extend process working set: %d\n", (int)GetLastError());
            }
            if (!VirtualLock(buf, size)) { 
                printf("Virtuanl lock failed with status: %d\n", (int)GetLastError());
            }        
        }
#endif
    }
    return buf;
}

void dbOSFile::protectBuffer(void* buf, size_t size, bool readonly)
{
    DWORD oldProt;
    VirtualProtect(buf, size, readonly ? PAGE_READONLY : PAGE_READWRITE, &oldProt);
}



void  dbOSFile::deallocateBuffer(void* buffer, size_t size, bool unlock)
{
#ifndef _WINCE
    if (unlock) {
        VirtualUnlock(buffer, size);
    }
#endif
    VirtualFree(buffer, 0, MEM_RELEASE);
}

size_t dbOSFile::ramSize()
{
    MEMORYSTATUS memStat;
    GlobalMemoryStatus(&memStat);
    return memStat.dwTotalPhys;
}


char_t* dbOSFile::errorText(int code, char_t* buf, size_t bufSize)
{
    int len;

    switch (code) {
      case ok:
        STRNCPY(buf, STRLITERAL("No error"), bufSize-1);
        break;
      case eof:
        STRNCPY(buf, STRLITERAL("Transfer less bytes than specified"), bufSize-1);
        break;
      default:
        len = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                            NULL,
                            code,
                            0,
                            buf,
                            (DWORD)bufSize-1,
                            NULL);
        if (len == 0) {
            char_t errcode[64];
            SPRINTF(SPRINTF_BUFFER(errcode), STRLITERAL("unknown error %u"), code);
            STRNCPY(buf, errcode, bufSize-1);
        }
    }
    buf[bufSize-1] = '\0';
    return buf;
}

#else // Unix

END_GIGABASE_NAMESPACE

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

BEGIN_GIGABASE_NAMESPACE

#ifdef __linux__
#define lseek(fd, offs, whence) lseek64(fd, offs, whence)
#endif

dbOSFile::dbOSFile()
{
#if !defined(__SYMBIAN32__)
    fd = -1;
#endif
}

#if defined(__linux__)
bool directIoSupported(char const* fileName)
{
    int dummy = 0;
    char* buf = new char[strlen(fileName) + 8];
    sprintf(buf, "%s.direct", fileName);
    int fd = open(buf, O_WRONLY|O_CREAT|O_DIRECT, 0666);
    if (fd < 0) {         
        delete[] buf;
        return false;
    }
    int rc = write(fd, (char*)&dummy+1, 1);
    int error = errno;
    close(fd);
    unlink(buf);
    delete[] buf;
    return rc < 0 && error == EINVAL;
}
#endif

int dbOSFile::open(char_t const* fileName, int attr)
{
#if defined(__SYMBIAN32__)
    wchar_t buf[1024];
    int fileAttr = (attr & read_only) ? EFileRead : EFileWrite;
    fileAttr |= (attr & shared) ? EFileShareReadersOrWriters : (attr & read_only) ? EFileShareReadersOnly : EFileShareExclusive;
		
    int len = mbstowcs(buf, fileName, sizeof buf);
    int rc = file.Open(CCoeEnv::Static()->FsSession(), TPtrC((TUint16*)buf, len), fileAttr);
    if (rc == KErrNotFound) {
        rc = file.Create(CCoeEnv::Static()->FsSession(), TPtrC((TUint16*)buf, len), fileAttr);
    }
    return rc;
#else    	
    char* name;
#ifdef UNICODE 
    char buf[1024];
    wcstombs(buf, fileName, sizeof buf);
    name = buf;
#else
    name = (char*)fileName;
#endif
    noSync = (attr & no_sync) != 0;
#if defined(__sun)
    fd = ::open64(name, ((attr & read_only) ? O_RDONLY : O_CREAT|O_RDWR)
                  | ((attr & truncate) ? O_TRUNC : 0), 0666);
    if (fd >= 0 && (attr & no_buffering) != 0) { 
        if (directio(fd, DIRECTIO_ON) == 0) { 
            noSync = true;
        }
    }
#elif defined(_AIX)
#if defined(_AIX43)
    fd = ::open64(name, ((attr & read_only) ? O_RDONLY : O_CREAT|O_RDWR|O_LARGEFILE|O_DSYNC|
                         ((attr & no_buffering) ? O_DIRECT : 0)), 0666);
    if (attr & no_buffering) {
        noSync = true;
    }                  
#else
     fd = ::open64(name, ((attr & read_only) ? O_RDONLY : O_CREAT|O_RDWR|O_LARGEFILE
                   | ((attr & truncate) ? O_TRUNC : 0), 0666);
#endif /* _AIX43 */
#elif defined(__linux__)
    fd = ::open(name, O_LARGEFILE
                | ((attr & read_only) ? O_RDONLY : O_CREAT|O_RDWR)
                | ((attr & truncate) ? O_TRUNC : 0)
                | ((attr & no_buffering) ? O_DIRECT : 0), 0666);
    if (fd < 0) { 
        fd = ::open(name, O_LARGEFILE
                    | ((attr & read_only) ? O_RDONLY : O_CREAT|O_RDWR)
                    | ((attr & truncate) ? O_TRUNC : 0), 0666);        
    } else if ((attr & no_buffering) && directIoSupported(name)) {
        noSync = true;
    }       
#else
    fd = ::open(name, O_LARGEFILE | ((attr & read_only) ? O_RDONLY : O_CREAT|O_RDWR)
                | ((attr & truncate) ? O_TRUNC : 0), 0666);
#endif
    if (fd < 0) {
        return errno;
    }
    if (attr & delete_on_close) {
        ::unlink(name);
    }
    return ok;
#endif
}

int dbOSFile::setSize(offs_t size)
{
#ifdef __SYMBIAN32__
    return file.SetSize(size);
#else
    return ftruncate(fd, size) == 0 ? ok : errno;
#endif
}

int dbOSFile::lock(LockType lck)
{
#ifdef __SYMBIAN32__
    return file.Lock(0, 1);
#else
    struct flock flc;
    flc.l_type = (lck == lck_shared) ? F_RDLCK : F_WRLCK;
    flc.l_whence = SEEK_SET;
    flc.l_start = 0;    
    flc.l_len = 1; 
    return fcntl(fd, F_SETLKW, &flc) == 0 ? ok : errno;
#endif
}

int dbOSFile::unlock()
{
#ifdef __SYMBIAN32__
    return file.UnLock(0, 1);
#else
    struct flock flc;
    flc.l_type = F_UNLCK;
    flc.l_whence = SEEK_SET;
    flc.l_start = 0;    
    flc.l_len = 1; 
    return fcntl(fd, F_SETLKW, &flc) == 0 ? ok : errno;
#endif
}

int dbOSFile::read(offs_t pos, void* buf, size_t size)
{
#ifdef __SYMBIAN32__
    TPtr8 dst((TUint8*)buf, size);
    int rc = file.Read(pos, dst, size);
    size_t read = dst.Length();
    return rc == KErrNone ? read == size ? ok : eof : rc;
#else
    ssize_t rc;
#if defined(__sun) || defined(_AIX43) || defined(__linux__)
#if !defined(SOLARIS) || defined(L64)
    rc = pread64(fd, buf, size, pos);
#else
    rc = pread(fd, buf, size, pos);
#endif
#else
    {
        dbCriticalSection cs(mutex);
        if (offs_t(lseek(fd, pos, SEEK_SET)) != pos) {
            return errno;
        }
        rc = ::read(fd, (char*)buf, size);
    }
#endif
    if (rc == -1) {
        return errno;
    } else if (size_t(rc) != size) {
        return eof;
    } else {
        return ok;
    }
#endif
}

int dbOSFile::read(void* buf, size_t size)
{
#ifdef __SYMBIAN32__
    TPtr8 dst((TUint8*)buf, size);
    int rc = file.Read(dst, size);
    size_t read = dst.Length();
    return rc == KErrNone ? read == size ? ok : eof : rc;
#else
	ssize_t rc = ::read(fd, (char*)buf, size);
    if (rc == -1) {
        return errno;
    } else if (size_t(rc) != size) {
        return eof;
    } else {
        return ok;
    }
#endif
}

int dbOSFile::write(void const* buf, size_t size)
{
#ifdef __SYMBIAN32__
    return file.Write(TPtr8((TUint8*)buf, size, size));
#else
    ssize_t rc = ::write(fd, (char const*)buf, size);
    if (rc == -1) {
        return errno;
    } else if (size_t(rc) != size) {
        return eof;
    } else {
        return ok;
    }
#endif
}

int dbOSFile::write(offs_t pos, void const* buf, size_t size)
{
#ifdef __SYMBIAN32__
    TInt fileSize;
    int rc = file.Size(fileSize);
    if (rc != KErrNone) {
        return rc;
    }
    if ((offs_t)fileSize < pos) {
        file.SetSize(pos);
    }
    return file.Write(pos, TPtr8((TUint8*)buf, size, size));
#else
    ssize_t rc;
#if defined(__sun) || defined(_AIX43) || defined(__linux__)
#if !defined(SOLARIS) || defined(L64)
    rc = pwrite64(fd, buf, size, pos);
#else 
    rc = pwrite(fd, buf, size, pos);
#endif
#else
    {
        dbCriticalSection cs(mutex);
        rc = lseek(fd, pos, SEEK_SET);
        if (rc != pos) {
            perror("lseek");
            return errno;
        }
        rc = ::write(fd, (char const*)buf, size);
    }
#endif
    if (rc == -1) {
        return errno;
    } else if (size_t(rc) != size) {
        return eof;
    } else {
        return ok;
    }
#endif
}

int dbOSFile::flush()
{
#ifdef __SYMBIAN32__
    return noSync ? ok : file.Flush();
#else
    return noSync || fsync(fd) == ok ? ok : errno;
#endif
}

int dbOSFile::close()
{
#ifdef __SYMBIAN32__
   file.Close();
   return ok;
#else
   if (fd != -1) {
        if (::close(fd) == ok) {
            fd = -1;
            return ok;
        } else {
            return errno;
        }
    } else {
        return ok;
    }
#endif
}

void* dbOSFile::allocateBuffer(size_t size, bool)
{
#if defined(__SYMBIAN32__) || defined(__MINGW32__) || defined(__CYGWIN__) || (defined(__QNX__) && !defined(__QNXNTO__))
    return malloc(size);
#else
    return valloc(size);
#endif
}

void  dbOSFile::deallocateBuffer(void* buffer, size_t, bool)
{
    free(buffer);
}

#ifdef __linux__
END_GIGABASE_NAMESPACE
#include <sys/sysinfo.h>
BEGIN_GIGABASE_NAMESPACE

size_t dbOSFile::ramSize()
{
    struct sysinfo info;
    sysinfo(&info);
#ifdef SYSINFO_HAS_NO_MEM_UNIT
    return info.totalram;
#else
    return info.totalram*info.mem_unit;
#endif
}
#else
size_t dbOSFile::ramSize()
{
#if defined(__sun)
    return sysconf(_SC_PHYS_PAGES)*sysconf(_SC_PAGE_SIZE);
#elif defined(__SYMBIAN32__)
    return 128 * 1024;
#else
    return 64 * 1024 * 1024;
#endif
}
#endif

char_t* dbOSFile::errorText(int code, char_t* buf, size_t bufSize)
{
    switch (code) {
      case ok:
        STRNCPY(buf, STRLITERAL("No error"), bufSize-1);
        break;
      case eof:
        STRNCPY(buf, STRLITERAL("Transfer less bytes than specified"), bufSize-1);
        break;
      default:
        STRNCPY(buf, strerror(code), bufSize-1);
    }
    buf[bufSize-1] = '\0';
    return buf;
}

#endif

int dbMultiFile::open(int n, dbSegment* seg, int attr)
{
    segment = new dbFileSegment[n];
    nSegments = n;
    while (--n >= 0) {
        segment[n].size = seg[n].size*dbPageSize;
        segment[n].offs = seg[n].offs;
        int rc = segment[n].open(seg[n].name, attr);
        if (rc != ok) {
            while (++n < nSegments) {
                segment[n].close();
            }
            return rc;
        }
    }
    return ok;
}

int dbMultiFile::close()
{
    if (segment != NULL) {
        for (int i = nSegments; --i >= 0;) {
            int rc = segment[i].close();
            if (rc != ok) {
                return rc;
            }
        }
        delete[] segment;
        segment = NULL;
    }
    return ok;
}

int dbMultiFile::setSize(offs_t)
{
    return ok;
}

int dbMultiFile::flush()
{
    for (int i = nSegments; --i >= 0;) {
        int rc = segment[i].flush();
        if (rc != ok) {
            return rc;
        }
    }
    return ok;
}


int dbMultiFile::write(offs_t pos, void const* ptr, size_t size)
{
    int n = nSegments-1;
    char const* src = (char const*)ptr;
    for (int i = 0; i < n; i++) {
        if (pos < segment[i].size) {
            if (pos + size > segment[i].size) {
                int rc = segment[i].write(segment[i].offs + pos, src, size_t(segment[i].size - pos));
                if (rc != ok) {
                    return rc;
                }
                size -= size_t(segment[i].size - pos);
                src += size_t(segment[i].size - pos);
                pos = 0;
            } else {
                return segment[i].write(segment[i].offs + pos, src, size);
            }
        } else {
            pos -= segment[i].size;
        }
    }
    return segment[n].write(segment[n].offs + pos, src, size);
}

int dbMultiFile::read(offs_t pos, void* ptr, size_t size)
{
    int n = nSegments-1;
    char* dst = (char*)ptr;
    for (int i = 0; i < n; i++) {
        if (pos < segment[i].size) {
            if (pos + size > segment[i].size) {
                int rc = segment[i].read(segment[i].offs + pos, dst, size_t(segment[i].size - pos));
                if (rc != ok) {
                    return rc;
                }
                size -= size_t(segment[i].size - pos);
                dst += size_t(segment[i].size - pos);
                pos = 0;
            } else {
                return segment[i].read(segment[i].offs + pos, dst, size);
            }
        } else {
            pos -= segment[i].size;
        }
    }
    return segment[n].read(segment[n].offs + pos, dst, size);
}


int dbRaidFile::setSize(offs_t)
{
    return ok;
}

int dbRaidFile::write(offs_t pos, void const* ptr, size_t size)
{
    char const* src = (char const*)ptr;
    while (true) { 
        int i = (int)(pos / raidBlockSize % nSegments);
        int offs = (int)((unsigned)pos % raidBlockSize);
        size_t available = raidBlockSize - offs;
        if (available >= size) { 
            return segment[i].write((offs_t)(segment[i].offs + pos / (raidBlockSize*nSegments) * raidBlockSize + offs), src, size);
        }
        int rc = segment[i].write((offs_t)(segment[i].offs + pos / (raidBlockSize*nSegments) * raidBlockSize + offs), src, available);
        if (rc != ok) {
            return rc;
        }
        src += available;
        pos += (offs_t)available;
        size -= available;
    }
}
            

int dbRaidFile::read(offs_t pos, void* ptr, size_t size)
{
    char* dst = (char*)ptr;
    while (true) { 
        int i = (int)(pos / raidBlockSize % nSegments);
        int offs = (int)((unsigned)pos % raidBlockSize);
        size_t available = raidBlockSize - offs;
        if (available >= size) { 
            return segment[i].read((offs_t)(segment[i].offs + pos / (raidBlockSize*nSegments) * raidBlockSize + offs), dst, size);
        }
        int rc = segment[i].read((offs_t)(segment[i].offs + pos / (raidBlockSize*nSegments) * raidBlockSize + offs), dst, available);
        if (rc != ok) {
            return rc;
        }
        dst += available;
        pos += (offs_t)available;
        size -= available;
    }
}
            
END_GIGABASE_NAMESPACE






