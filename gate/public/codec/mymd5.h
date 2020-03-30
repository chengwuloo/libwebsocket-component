#ifndef MYMD5_INCLUDE_H  
#define MYMD5_INCLUDE_H  

// #ifdef __cplusplus
// extern "C"
// {
// #endif

typedef struct {
	unsigned int count[2];
	unsigned int state[4];
	unsigned char buffer[64];
}MY_MD5_CTX;

#define F__(x,y,z) ((x & y) | (~x & z))  
#define G__(x,y,z) ((x & z) | (y & ~z))  
#define H__(x,y,z) (x^y^z)  
#define I__(x,y,z) (y ^ (x | ~z))  
#define ROTATE_LEFT(x,n) ((x << n) | (x >> (32-n)))  
#define FF__(a,b,c,d,x,s,ac) \
	{ \
		a += F__(b, c, d) + x + ac; \
		a = ROTATE_LEFT(a, s); \
		a += b; \
	}

#define GG__(a,b,c,d,x,s,ac) \
	{ \
		a += G__(b, c, d) + x + ac; \
		a = ROTATE_LEFT(a, s); \
		a += b; \
	}

#define HH__(a,b,c,d,x,s,ac) \
	{ \
		a += H__(b, c, d) + x + ac; \
		a = ROTATE_LEFT(a, s); \
		a += b; \
	}
#define II__(a,b,c,d,x,s,ac) \
	{ \
		a += I__(b, c, d) + x + ac; \
		a = ROTATE_LEFT(a, s); \
		a += b; \
	}

void MD5Init(MY_MD5_CTX *context);
void MD5Update(MY_MD5_CTX *context, unsigned char const* input, unsigned int inputlen);
void MD5Final(MY_MD5_CTX *context, unsigned char digest[16]);
void MD5Transform(unsigned int state[4], unsigned char const block[64]);
void MD5Encode(unsigned char *output, unsigned int const* input, unsigned int len);
void MD5Decode(unsigned int *output, unsigned char const* input, unsigned int len);

char const* MD5Encode32(char const *src, unsigned len, char dst[], int upper);

// #ifdef __cplusplus
// }
// #endif

#endif