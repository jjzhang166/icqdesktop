#include "stdafx.h"
//#include "md5.h"

#include "openssl/md5.h"

namespace core
{
    namespace tools
    {
        std::string md5(const void* _data, int32_t _size)
        {
            std::stringstream ss_hash;

            MD5_CTX md5handler;
            unsigned char md5digest[MD5_DIGEST_LENGTH];

            MD5_Init(&md5handler);
            MD5_Update(&md5handler, _data, _size);
            MD5_Final(md5digest,&md5handler);

            char format_buffer[10];

            for (int32_t i = 0; i < MD5_DIGEST_LENGTH; i++)
            {
#ifdef _WIN32
                sprintf_s(format_buffer, 10, "%02x", md5digest[i]);
#else
                sprintf(format_buffer, "%02x", md5digest[i]);
#endif

                ss_hash << format_buffer;
            }

            return ss_hash.str();
        }
    }
}


