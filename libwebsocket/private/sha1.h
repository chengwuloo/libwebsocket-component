/* ================ sha1.h ================ */
#ifndef __SHA_H_
#define __SHA_H_
/*
SHA-1 in C
By Steve Reid <steve@edmweb.com>
100% Public Domain
*/
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
	/*uint32_t*/unsigned int count[2];
    /*uint32_t*/unsigned int state[5];
    unsigned char buffer[64];
} SHA1_CTX;

void SHA1Init(SHA1_CTX* context);
void SHA1Update(SHA1_CTX* context, const unsigned char* data, /*uint32_t*/unsigned int len);
void SHA1Final(SHA1_CTX* context, unsigned char digest[20]);
void SHA1Transform(/*uint32_t*/unsigned int state[5], const unsigned char buffer[64]);

#ifdef __cplusplus
}
#endif
#endif