#include "stdtp.h"
#include "cli.h"
#include "cliproto.h"

#include <zlib.h>

#define DEFAULT_SEGMENT_SIZE 128*1024

int main(int argc, char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: compressdb DATABASE-FILE [SEGMENT-SIZE] [COMPRESSION-LEVEL]\n");
        return 1;
    }
    int segmentSize = (argc >= 3) ? atol(argv[2]) : DEFAULT_SEGMENT_SIZE;
    int compressionLevel = (argc >= 4) ? atol(argv[3]) : Z_DEFAULT_COMPRESSION;
    char* filePath = argv[1];
    FILE* src = fopen(filePath, "rb");
    filePath[strlen(filePath)-1] = 'z'; // replace ".dbs" + ".dbz"
    FILE* dst = fopen(filePath, "wb");
    fseek(src, 0, SEEK_END);
    off_t fileSize = ftell(src);
    int nSegments = (int)((fileSize + segmentSize - 1) / segmentSize);
    fseek(src, 0, SEEK_SET);
    char* srcBuf = new char[segmentSize];
    char* dstBuf = new char[segmentSize*2];
    size_t headerSize = (nSegments+2)*4;
    char* header = new char[headerSize];
    fseek(dst, headerSize, SEEK_SET);
    for (int i = 0; i < nSegments; i++) { 
        uLongf destLen = segmentSize*2;
        size_t size = fread(srcBuf, 1, segmentSize, src);
        int rc = compress2((Bytef*)dstBuf, &destLen, (Bytef*)srcBuf, size, compressionLevel);
        if (rc != Z_OK) {
            fprintf(stderr, "Compression error %d\n", rc);
            return 1;
        }
        fwrite(dstBuf, 1, destLen, dst);
        pack4(header + (i+2)*4, destLen);
    }
    pack4(header, nSegments);
    pack4(header + 4, segmentSize);
    fseek(dst, 0, SEEK_SET);
    fwrite(header, 1, headerSize, dst);
    fclose(src);
    fclose(dst);
    printf("Write file '%s'\n", filePath);
    return 0;
}
