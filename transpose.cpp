#include "aplga.h"

int equal_apls = 0;
int repeated_apls = 0;

/* Vectorized DJBHash. */
uint32_t strhash(const char* str, __m128i& fullhash){
    __m128i xmm0 = _mm_setzero_si128();
    __m128i xmm2 = _mm_set1_epi32(5381);
    __m128i xmm3 = _mm_set1_epi32(0xff);
    __m128i* p = (__m128i*)str;
    __m128i mask;
    int exitflag = 1;
    __m128i xmm1;
    while (exitflag){
        xmm1 = _mm_loadu_si128(p++);
        mask = _mm_cmpeq_epi8(xmm1, xmm0);
        if (_mm_movemask_epi8(mask) & 0xffff){
            mask = _mm_and_si128(mask, _mm_slli_si128(mask, 1));
            mask = _mm_and_si128(mask, _mm_slli_si128(mask, 2));
            mask = _mm_and_si128(mask, _mm_slli_si128(mask, 4));
            mask = _mm_and_si128(mask, _mm_slli_si128(mask, 8));
            xmm1 = _mm_andnot_si128(mask, xmm1);
            exitflag = 0;
        }
        xmm2 = _mm_add_epi32(xmm2, _mm_slli_epi32(xmm2, 5));
        xmm2 = _mm_add_epi32(xmm2, _mm_and_si128(_mm_srli_epi32(xmm1, 24), xmm3));
        xmm2 = _mm_add_epi32(xmm2, _mm_slli_epi32(xmm2, 5));
        xmm2 = _mm_add_epi32(xmm2, _mm_and_si128(_mm_srli_epi32(xmm1, 16), xmm3));
        xmm2 = _mm_add_epi32(xmm2, _mm_slli_epi32(xmm2, 5));
        xmm2 = _mm_add_epi32(xmm2, _mm_and_si128(_mm_srli_epi32(xmm1, 8), xmm3));
        xmm2 = _mm_add_epi32(xmm2, _mm_slli_epi32(xmm2, 5));
        xmm2 = _mm_add_epi32(xmm2, _mm_and_si128(xmm1, xmm3));
    }
    uint32_t u[4];
    _mm_storeu_si128((__m128i*)u, xmm2);
    fullhash = xmm2;
    u[0] ^= u[1];
    u[0] ^= u[2];
    u[0] ^= u[3];
    return u[0];
}

const size_t HASH_SIZE = 0x02000000;

__m128i fullhash[HASH_SIZE];
//float   hashedps[HASH_SIZE];

int checktt(__m128i& longhash, uint32_t shorthash){
    __m128i recordhash;
    shorthash &= (HASH_SIZE - 1);
    recordhash = fullhash[shorthash];
    if (0xffff == _mm_movemask_epi8(_mm_cmpeq_epi8(recordhash, longhash)))
        return 1;//hashedps[shorthash];
    else
        return 0;//-1.0f;
}

void recordtt(__m128i& longhash, uint32_t shorthash){
    shorthash &= (HASH_SIZE - 1);
    fullhash[shorthash] = longhash;
    //hashedps[shorthash] = dps;
}


struct hashdict_t{
	unsigned r[32][256];
	hashdict_t(){
		for (int i = 0; i < 32; i++){
			for (int j = 0; j < 256; j++){
				r[i][j] = rng()();
			}
		}
	}
};

hashdict_t& dict(){
	static hashdict_t _dict;
	return _dict;
}

unsigned char transpose[1U << 30] = {0};

bool hash_apl(lelem_t& elem){
	float result[8];
	unsigned char* p = (unsigned char*)(void*)&result[0];
	std::string str;
	elem.apl->print(str);
	__m128i longhash;
	unsigned shorthash;
	shorthash = strhash(str.c_str(), longhash);
	if (checktt(longhash, shorthash)){
		repeated_apls++;
		return false;
	}
	recordtt(longhash, shorthash);

	irecore_hash(str, result);
	if (elem.saved_kernel)
		clReleaseProgram(elem.saved_kernel);
	elem.saved_kernel = ocl().program;
	unsigned elemhash = 0;
	for (int i = 0; i < 32; i++){
		elemhash ^= dict().r[i][p[i]];
	}
	union{
		unsigned char ch;
		struct{
			unsigned hi : 4;
			unsigned lo : 4;
		} half;
	}tt;
	p = &transpose[(0x7fffffff & elemhash) >> 1];
	tt.ch = *p;

	if (elemhash & 1){
		if (tt.half.hi){
			equal_apls++;
			if (elem.apl->complexity() < tt.half.hi){
				tt.half.hi = elem.apl->complexity();
				*p = tt.ch;
			}
			else return false;
		}
		else{
			tt.half.hi = elem.apl->complexity();
			*p = tt.ch;
		}
		
	}
	else{
		if (tt.half.lo){
			equal_apls++;
			if (elem.apl->complexity() < tt.half.lo){
				tt.half.lo = elem.apl->complexity();
				*p = tt.ch;
			}
			else return false;
		}
		else{
			tt.half.lo = elem.apl->complexity();
			*p = tt.ch;
		}
	}
	return true;
}