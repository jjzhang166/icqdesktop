//-< TESTBLOB.CXX >--------------------------------------------------*--------*
// GigaBASE                  Version 1.0         (c) 1999  GARRET    *     ?  *
// (Post Relational Database Management System)                      *   /\|  *
//                                                                   *  /  \  *
//                          Created:     23-Jun-2000  K.A. Knizhnik  * / [] \ *
//                          Last update: 23-Jun-2000  K.A. Knizhnik  * GARRET *
//-------------------------------------------------------------------*--------*
// Test program for binary large objects. This program can be used as
// exmaple for creating your own multimedia objects.
//-------------------------------------------------------------------*--------*

#include "gigabase.h"
#include <stdio.h>

#ifdef USE_ZLIB
#include "zipfile.h"
#endif

USE_GIGABASE_NAMESPACE

class File {
  public:
    char const* name;
    dbBlob      blob;

    TYPE_DESCRIPTOR((KEY(name, INDEXED|HASHED),
                     FIELD(blob)));
};

int __cdecl main(int argc, char* argv[])
{
    const int maxStrLen = 256;
    dbDatabase db;
    char cmd[maxStrLen];
    char input_file[maxStrLen];
    char output_file[maxStrLen];
    char buf[32*1024];
    char buf2[32*1024];
    File dbf;
    FILE* f;
    dbCursor<File> cursor;

#ifdef USE_ZLIB
    dbZipFile file;
    if (file.open("testblob.dbz", 0) != dbFile::ok) { 
        fprintf(stderr, "Failed to open compressed file\n");
        return EXIT_FAILURE;
    }                                               
    if (db.open(&file)) {
#else
    if (db.open("testblob.dbs")) {
#endif
        dbQuery q;
        q = "name like",input_file;

        if (argc > 1) { 
            // batch mode
            for (int i = 1; i < argc; i++) { 
                strcpy(input_file, argv[i]);
                if ((f = fopen(input_file, "rb")) == NULL) {
                    fprintf(stderr, "Failed to open input file\n");
                    continue;
                }
                if (cursor.select(q) == 0) {
                    size_t len;
                    dbf.blob.create(db);
                    dbBlobWriteIterator iter = dbf.blob.getWriteIterator(db);
                    while ((len = fread(buf, 1, sizeof buf, f)) > 0) {
                        iter.write(buf, len);
                    }
                    iter.close();
                    fclose(f);
                    
                    dbf.name = input_file;
                    insert(dbf);
                    printf("Import file '%s'\n", input_file);
                } else { 
                    dbBlobReadIterator iter = cursor->blob.getReadIterator(db);
                    while (iter.getAvailableSize() != 0) {
                        size_t len = iter.read(buf, sizeof buf);
                        size_t len2 = fread(buf2, 1, len, f); 
                        assert(len == len2);
                        assert(memcmp(buf, buf2, len) == 0);
                    }
                    printf("Verification completed for file '%s'\n", input_file);
                    iter.close();
                    fclose(f);
                }
            }
            db.close();
            return EXIT_SUCCESS;
        }
        printf(">");
        while (fgets(buf, sizeof buf, stdin)) {
            *input_file = '\0';
            *output_file = '\0';
            if (sscanf(buf, "%s%s%s", cmd, input_file, output_file) >= 1) {
                if (strcmp(cmd, "put") == 0) {
                    if (!*input_file) {
                        fprintf(stderr, "File name was not specified\n");
                    } else if (cursor.select(q, dbCursorForUpdate)) {
                        fprintf(stderr, "File already exists\n");
                    } else {
                        if ((f = fopen(input_file, "rb")) == NULL) {
                            fprintf(stderr, "Failed to open input file\n");
                        } else {
                            size_t len;
                            dbf.blob.create(db);
                            dbBlobWriteIterator iter = dbf.blob.getWriteIterator(db);

                            //
                            // Direct access to the buffer
                            //
                            void* addr = iter.mapBuffer();
                            len = fread(addr, 1, iter.getAvailableBufferSize(), f);
                            iter.unmapBuffer(len);

                            //
                            // Buffered write
                            //
                            while ((len = fread(buf, 1, sizeof buf, f)) > 0) {
                                iter.write(buf, len);
                            }
                            iter.close();
                            fclose(f);

                            dbf.name = output_file;
                            insert(dbf);
                        }
                    }
                } else if (strcmp(cmd, "del") == 0) {
                    if (!*input_file) {
                        fprintf(stderr, "File name was not specified\n");
                    } else {
                        if (cursor.select(q, dbCursorForUpdate) == 0) {
                            fprintf(stderr, "File not found\n");
                        } else {
                            do {
                                cursor->blob.free(db);
                                cursor.remove();
                            } while (!cursor.isEmpty());
                        }
                    }
                } else if (strcmp(cmd, "get") == 0) {
                    if (!*input_file) {
                        fprintf(stderr, "Input file was not specified\n");
                    } else if (cursor.select(q) == 0) {
                        fprintf(stderr, "File not found\n");
                    } else {
                        f = *output_file ? fopen(output_file, "wb") : stdout;
                        if (f == NULL) {
                            fprintf(stderr, "Failed to open output file '%s'\n", output_file);
                        } else {
                            dbBlobReadIterator iter = cursor->blob.getReadIterator(db);
                            size_t len;
                            //
                            // Direct access to the buffer
                            //
                            void* addr = iter.mapBuffer();
                            len = fwrite(addr, 1, iter.getAvailableBufferSize(), f);
                            iter.unmapBuffer(len);

                            //
                            // Buffered read
                            //
                            while (iter.getAvailableSize() != 0) {
                                len = iter.read(buf, sizeof buf);
                                fwrite(buf, 1, len, f);
                            }
                            iter.close();
                            fclose(f);
                        }
                    }
                } else if (strcmp(cmd, "dir") == 0) {
                    if (cursor.select()) {
                        do {
                            printf("%s\t%ld\n", cursor->name, (long)cursor->blob.getSize(db));
                        } while (cursor.next());
                    }
                } else if (strcmp(cmd, "exit") == 0) {
                    break;
                } else {
                    printf("Commands:\n"
                           "  exit\n"
                           "  dir\n"
                           "  del file_name\n"
                           "  put source-file-name destination-file-name\n"
                           "  get source-file-name [destination-file-name]\n");
                }
            }
            db.commit();
            printf(">");
        }
        db.close();
        printf("Database session finished\n");
        return EXIT_SUCCESS;
    } else {
        printf("Failed to open database\n");
        return EXIT_FAILURE;
    }
}

REGISTER(File);
