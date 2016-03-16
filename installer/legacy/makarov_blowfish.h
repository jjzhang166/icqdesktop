
#pragma once

class MAKFC_CReaderWriter;

#define MAXKEYBYTES 56          /* 448 bits */

typedef DWORD uInt32;

typedef struct {
    uInt32 P[16 + 2];
    uInt32 S[4][256];
} BLOWFISH_CTX;

void Blowfish_DecryptSimple(std::vector<unsigned char> &inOutData, const void *key, size_t keySize);
void Blowfish_Init(BLOWFISH_CTX *ctx, unsigned char *key, int keyLen);
void Blowfish_Encrypt(BLOWFISH_CTX *ctx, uInt32 *xl, uInt32 *xr);
void Blowfish_Decrypt(BLOWFISH_CTX *ctx, uInt32 *xl, uInt32 *xr);

struct BLOWFISHKEY
{
public:

    BYTE key[MAXKEYBYTES];
};



BLOWFISHKEY Blowfish_GenerateKey();
MAKFC_CReaderWriter* Blowfish_EncodeData(BYTE *lpData, int iSize, BLOWFISHKEY key);
MAKFC_CReaderWriter* Blowfish_DecodeData(BYTE *lpData, int iSize, BLOWFISHKEY key);
BOOL Blowfish_ReadKey(HANDLE hBFFile, BLOWFISHKEY *key, BOOL bRegenerate = FALSE);
