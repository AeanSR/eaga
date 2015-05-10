#include "aplga.h"

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

unsigned transpose[1 << 27] = {0};

bool hash_apl(lelem_t& elem){
	float result[8];
	unsigned char* p = (unsigned char*)(void*)&result[0];
	std::string str;
	elem.apl->print(str);
	irecore_hash(str, result);
	unsigned elemhash = 0;
	for (int i = 0; i < 32; i++){
		elemhash ^= dict().r[i][p[i]];
	}
	unsigned* tt = &transpose[elemhash >> 5];
	if (*tt & (1 << (elemhash & 31))) return false;
	*tt &= (1 << (elemhash & 31));
	return true;
}