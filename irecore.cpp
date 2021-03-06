#include "irecore.h"

std::ostream* report_path;
stat_t stat;
std::vector<stat_t> stat_array;
int stat_not_pushed = 1;
raidbuff_t raidbuff;
cl_uint seed;
std::string apl;
std::string predef;
int iterations;
float vary_combat_length;
float max_length;
float initial_health_percentage;
float death_pct;
float power_max; // rage_max
int plate_specialization;
int single_minded;
int race;
float mh_speed;
float oh_speed;
int mh_low, mh_high;
int oh_low, oh_high;
int mh_type;
int oh_type;
int talent;
int rng_engine;
int default_actions;
const int base10[] = { 1000000, 100000, 10000, 1000, 100, 10, 1 };
#define TALENT_TIER(tier) ((talent / base10[tier - 1]) % 10)
int calculate_scale_factors;

int archmages_incandescence;
int archmages_greater_incandescence;
int t17_2pc;
int t17_4pc;
int t18_2pc;
int t18_4pc;
int thunderlord_mh;
int thunderlord_oh;
int bleeding_hollow_mh;
int bleeding_hollow_oh;
int shattered_hand_mh;
int shattered_hand_oh;

int developer_debug;
int list_available_devices;
std::string trinket1_name;
std::string trinket2_name;
int trinket1_value;
int trinket2_value;

const char* trinket_list[] = {
	"none",
	"vial_of_convulsive_shadows",
	"forgemasters_insignia",
	"horn_of_screaming_spirits",
	"scabbard_of_kyanos",
	"badge_of_victory",
	"insignia_of_victory",
	"tectus_beating_heart",
	"formidable_fang",
	"draenic_stone",
	"skull_of_war",
	"mote_of_the_mountain",
	NULL
};

const char* race_str_kernel[] = {
	"RACE_NONE",
	"RACE_HUMAN",
	"RACE_DWARF",
	"RACE_GNOME",
	"RACE_NIGHTELF_DAY",
	"RACE_NIGHTELF_NIGHT",
	"RACE_DRAENEI",
	"RACE_WORGEN",
	"RACE_ORC",
	"RACE_TROLL",
	"RACE_TAUREN",
	"RACE_UNDEAD",
	"RACE_BLOODELF",
	"RACE_GOBLIN",
	"RACE_PANDAREN",
	NULL
};
const char* race_str_param[] = {
	"none",
	"human",
	"dwarf",
	"gnome",
	"nightelf_day",
	"nightelf_night",
	"draenei",
	"worgen",
	"orc",
	"troll",
	"tauren",
	"undead",
	"bloodelf",
	"goblin",
	"pandaren",
	NULL
};
const char* weapon_type_str[] = {
	"WEAPON_2H",
	"WEAPON_1H",
	"WEAPON_DAGGER",
};

void err(const char* format, ...){
	va_list vl;
	va_start(vl, format);
	vfprintf(stderr, format, vl);
	va_end(vl);
	exit(-1);
}

void set_default_parameters(){
	developer_debug = 0;
	list_available_devices = 0;
	ocl().opencl_device_id = 1;
	stat = {
		"",
		4313, 2148, 751, 1504, 478, 0
	};
	stat_not_pushed = 1;
	raidbuff = {
		1,1,1,1,1,1,1,1,1,1,1,1,1
	};
	raidbuff.vers = 1;
	seed = 0;
	srand((unsigned int)time(NULL));
	rng_engine = 32;
	apl = "";
	default_actions=0;
	iterations = 10000;
	vary_combat_length = 20.0f;
	max_length = 450.0f;
	initial_health_percentage = 100.0f;
	death_pct = 0.0f;
	power_max = 100.0f;
	plate_specialization = 1;
	single_minded = 0;
	race = 0;
	talent = 1321321;
	mh_speed = 3.6f;
	oh_speed = 3.6f;
	mh_high = 2490;
	oh_high = 2490;
	mh_low = 1659;
	oh_low = 1659;
	mh_type = 0;
	oh_type = 0;

	archmages_incandescence = 0;
	archmages_greater_incandescence = 1;
	t17_2pc = 1;
	t17_4pc = 1;
	t18_2pc = 0;
	t18_4pc = 0;
	thunderlord_mh = 1;
	thunderlord_oh = 1;
	bleeding_hollow_mh = 0;
	bleeding_hollow_oh = 0;
	shattered_hand_mh = 0;
	shattered_hand_oh = 0;

	trinket1_name = "vial_of_convulsive_shadows";
	trinket1_value = 2033;
	trinket2_name = "horn_of_screaming_spirits";
	trinket2_value = 2652;

	report_path = &std::cout;
	calculate_scale_factors = 0;
}

typedef struct{
	std::string key;
	std::string value;
} kvpair_t;

void build_arglist(std::vector<kvpair_t>& arglist, int argc, char** argv){
	for (int i = 1; i < argc; i++){
		kvpair_t kv;
		char* p;
		for (p = argv[i]; *p; p++){
			if (*p == '='){
				p++;
				break;
			}
			else{
				kv.key.push_back(*p);
			}
		}
		if (*p){
			for (; *p; p++){
				kv.value.push_back(*p);
			}
			arglist.push_back(kv);
		}
		else{
			FILE* f = fopen(kv.key.c_str(), "rb");
			if (!f) { err("Parameter \"%s\" is neither a valid key-value pair, nor a exist configuration file.", kv.key.c_str()); }
			char ** rargv = (char**)calloc(65536, sizeof(char*));
			int rargc = 1;
			char buffer[4096];
			size_t buffer_i = 0;
			char ch;
			int comment = 0;
			do {
				ch = fgetc(f);
				if (!comment){
					if (ch == '\r' || ch == '\n' || ch == '\t' || ch == ' ' || ch == EOF){
						if (buffer_i){
							buffer[buffer_i++] = 0;
							rargv[rargc] = (char*)malloc(buffer_i);
							memcpy(rargv[rargc], buffer, buffer_i);
							rargc++;
							buffer_i = 0;
							if (rargc >= 65536) { err("Configuration file too long."); }
						}
					}
					else if (ch == '#' && buffer_i == 0){
						comment = 1;
					}
					else{
						buffer[buffer_i++] = ch;
						if (buffer_i >= 4096) { err("Configuration line too long."); }
					}
				}
				else{
					if (ch == '\r' || ch == '\n'){
						comment = 0;
						buffer_i = 0;
					}
				}
			} while (ch != EOF);
			build_arglist(arglist, rargc, rargv);
			for (int i = 1; i < rargc; i++){
				free(rargv[i]);
			}
			free(rargv);
		}
	}
}

void parse_parameters(std::vector<kvpair_t>& arglist){
	for (auto i = arglist.begin(); i != arglist.end(); i++){
		if (0 == i->key.compare("gear_str")){
			stat.gear_str = atoi(i->value.c_str());
			if (stat.gear_str < 0) stat.gear_str = 0;
			stat_not_pushed = 1;
		}
		else if (0 == i->key.compare("gear_crit")){
			stat.gear_crit = atoi(i->value.c_str());
			if (stat.gear_crit < 0) stat.gear_crit = 0;
			stat_not_pushed = 1;
		}
		else if (0 == i->key.compare("gear_mastery")){
			stat.gear_mastery = atoi(i->value.c_str());
			if (stat.gear_mastery < 0) stat.gear_mastery = 0;
			stat_not_pushed = 1;
		}
		else if (0 == i->key.compare("gear_haste")){
			stat.gear_haste = atoi(i->value.c_str());
			if (stat.gear_haste < 0) stat.gear_haste = 0;
			stat_not_pushed = 1;
		}
		else if (0 == i->key.compare("gear_mult")){
			stat.gear_mult = atoi(i->value.c_str());
			if (stat.gear_mult < 0) stat.gear_mult = 0;
			stat_not_pushed = 1;
		}
		else if (0 == i->key.compare("gear_vers")){
			stat.gear_vers = atoi(i->value.c_str());
			if (stat.gear_vers < 0) stat.gear_vers = 0;
			stat_not_pushed = 1;
		}
		else if (0 == i->key.compare("gear_str+")){
			stat.gear_str += atoi(i->value.c_str());
			if (stat.gear_str < 0) stat.gear_str = 0;
			stat_not_pushed = 1;
		}
		else if (0 == i->key.compare("gear_crit+")){
			stat.gear_crit += atoi(i->value.c_str());
			if (stat.gear_crit < 0) stat.gear_crit = 0;
			stat_not_pushed = 1;
		}
		else if (0 == i->key.compare("gear_mastery+")){
			stat.gear_mastery += atoi(i->value.c_str());
			if (stat.gear_mastery < 0) stat.gear_mastery = 0;
			stat_not_pushed = 1;
		}
		else if (0 == i->key.compare("gear_haste+")){
			stat.gear_haste += atoi(i->value.c_str());
			if (stat.gear_haste < 0) stat.gear_haste = 0;
			stat_not_pushed = 1;
		}
		else if (0 == i->key.compare("gear_mult+")){
			stat.gear_mult += atoi(i->value.c_str());
			if (stat.gear_mult < 0) stat.gear_mult = 0;
			stat_not_pushed = 1;
		}
		else if (0 == i->key.compare("gear_vers+")){
			stat.gear_vers += atoi(i->value.c_str());
			if (stat.gear_vers < 0) stat.gear_vers = 0;
			stat_not_pushed = 1;
		}
		else if (0 == i->key.compare("gear_str-")){
			stat.gear_str -= atoi(i->value.c_str());
			if (stat.gear_str < 0) stat.gear_str = 0;
			stat_not_pushed = 1;
		}
		else if (0 == i->key.compare("gear_crit-")){
			stat.gear_crit -= atoi(i->value.c_str());
			if (stat.gear_crit < 0) stat.gear_crit = 0;
			stat_not_pushed = 1;
		}
		else if (0 == i->key.compare("gear_mastery-")){
			stat.gear_mastery -= atoi(i->value.c_str());
			if (stat.gear_mastery < 0) stat.gear_mastery = 0;
			stat_not_pushed = 1;
		}
		else if (0 == i->key.compare("gear_haste-")){
			stat.gear_haste -= atoi(i->value.c_str());
			if (stat.gear_haste < 0) stat.gear_haste = 0;
			stat_not_pushed = 1;
		}
		else if (0 == i->key.compare("gear_mult-")){
			stat.gear_mult -= atoi(i->value.c_str());
			if (stat.gear_mult < 0) stat.gear_mult = 0;
			stat_not_pushed = 1;
		}
		else if (0 == i->key.compare("gear_vers-")){
			stat.gear_vers -= atoi(i->value.c_str());
			if (stat.gear_vers < 0) stat.gear_vers = 0;
			stat_not_pushed = 1;
		}
		else if (0 == i->key.compare("push_stats")){
			stat.name = i->value;
			if (stat.name.empty()) stat.name = "<unnamed stat set>";
			stat_array.push_back(stat);
			stat_not_pushed = 0;
		}
		else if (0 == i->key.compare("deterministic_seed")){
			seed = atoi(i->value.c_str());
		}
		else if (0 == i->key.compare("iterations")){
			iterations = atoi(i->value.c_str());
			if (iterations <= 0) iterations = 1;
		}
		else if (0 == i->key.compare("raidbuff_str")){
			raidbuff.str = atoi(i->value.c_str());
			raidbuff.str = !!raidbuff.str;
		}
		else if (0 == i->key.compare("raidbuff_ap")){
			raidbuff.ap = atoi(i->value.c_str());
			raidbuff.ap = !!raidbuff.ap;
		}
		else if (0 == i->key.compare("raidbuff_sp")){
			raidbuff.sp = atoi(i->value.c_str());
			raidbuff.sp = !!raidbuff.sp;
		}
		else if (0 == i->key.compare("raidbuff_crit")){
			raidbuff.crit = atoi(i->value.c_str());
			raidbuff.crit = !!raidbuff.crit;
		}
		else if (0 == i->key.compare("raidbuff_haste")){
			raidbuff.haste = atoi(i->value.c_str());
			raidbuff.haste = !!raidbuff.haste;
		}
		else if (0 == i->key.compare("raidbuff_mastery")){
			raidbuff.mastery = atoi(i->value.c_str());
			raidbuff.mastery = !!raidbuff.mastery;
		}
		else if (0 == i->key.compare("raidbuff_mult")){
			raidbuff.mult = atoi(i->value.c_str());
			raidbuff.mult = !!raidbuff.mult;
		}
		else if (0 == i->key.compare("raidbuff_vers")){
			raidbuff.vers = atoi(i->value.c_str());
			raidbuff.vers = !!raidbuff.vers;
		}
		else if (0 == i->key.compare("raidbuff_sta")){
			raidbuff.sta = atoi(i->value.c_str());
			raidbuff.sta = !!raidbuff.sta;
		}
		else if (0 == i->key.compare("raidbuff_flask")){
			raidbuff.flask = !!atoi(i->value.c_str());
		}
		else if (0 == i->key.compare("raidbuff_food")){
			raidbuff.food = !!atoi(i->value.c_str());
		}
		else if (0 == i->key.compare("raidbuff_potion")){
			raidbuff.potion = !!atoi(i->value.c_str());
		}
		else if (0 == i->key.compare("raidbuff_bloodlust")){
			raidbuff.bloodlust = !!atoi(i->value.c_str());
		}
		else if (0 == i->key.compare("raidbuff_all")){
			raidbuff.str = atoi(i->value.c_str());
			raidbuff.str = !!raidbuff.str;
			raidbuff.bloodlust = raidbuff.flask = raidbuff.food = raidbuff.potion = raidbuff.ap = raidbuff.crit = raidbuff.haste = raidbuff.mastery = raidbuff.mult = raidbuff.vers = raidbuff.sp = raidbuff.sta = raidbuff.str;
			raidbuff.vers = 1; /* This cannot be cancelled ingame. */
			
		}
		else if (0 == i->key.compare("actions")){
			apl = i->value;
			apl.append("\n");
		}
		else if (0 == i->key.compare("actions+")){
			apl.append(i->value);
			apl.append("\n");
		}
		else if (0 == i->key.compare("default_actions")){
			default_actions = !!atoi(i->value.c_str());
		}
		else if (0 == i->key.compare("vary_combat_length")){
			vary_combat_length = atof(i->value.c_str());
			if (vary_combat_length > 100.0f) vary_combat_length = 100.0f;
			if (vary_combat_length < 0.0f) vary_combat_length = 0.0f;
		}
		else if (0 == i->key.compare("max_length")){
			max_length = atof(i->value.c_str());
			if (max_length < 1.0f) max_length = 1.0f;
		}
		else if (0 == i->key.compare("initial_health_percentage")){
			initial_health_percentage = atof(i->value.c_str());
			if (initial_health_percentage > 100.0f) initial_health_percentage = 100.0f;
			if (initial_health_percentage < 0.0f) initial_health_percentage = 0.0f;
		}
		else if (0 == i->key.compare("death_pct")){
			death_pct = atof(i->value.c_str());
			if (death_pct > 100.0f) death_pct = 100.0f;
			if (death_pct < 0.0f) death_pct = 0.0f;
		}
		else if (0 == i->key.compare("rage_max")){
			power_max = atof(i->value.c_str());
			if (power_max < 0.0f) power_max = 0.0f;
		}
		else if (0 == i->key.compare("plate_specialization")){
			plate_specialization = atoi(i->value.c_str());
			plate_specialization = !!plate_specialization;
		}
		else if (0 == i->key.compare("race")){
			race = -1;
			for (int j = 0; race_str_param[j]; j++){
				if (0 == i->value.compare(race_str_param[j])){
					race = j;
					break;
				}
			}
			if (race == -1) { err("No such race \"%s\".", i->value.c_str()); }
		}
		else if (0 == i->key.compare("mh_speed")){
			mh_speed = atof(i->value.c_str());
			if (mh_speed <= 0.0f) mh_speed = 1.5f;
		}
		else if (0 == i->key.compare("oh_speed")){
			oh_speed = atof(i->value.c_str());
			if (oh_speed <= 0.0f) oh_speed = 1.5f;
		}
		else if (0 == i->key.compare("mh_low")){
			mh_low = atoi(i->value.c_str());
			if (mh_low < 0) mh_low = 0;
		}
		else if (0 == i->key.compare("mh_high")){
			mh_high = atoi(i->value.c_str());
			if (mh_high < 0) mh_high = 0;
		}
		else if (0 == i->key.compare("oh_low")){
			oh_low = atoi(i->value.c_str());
			if (oh_low < 0) oh_low = 0;
		}
		else if (0 == i->key.compare("oh_high")){
			oh_high = atoi(i->value.c_str());
			if (oh_high < 0) oh_high = 0;
		}
		else if (0 == i->key.compare("mh_type")){
			if (0 == i->value.compare("2h")) mh_type = 0;
			else if (0 == i->value.compare("1h")) mh_type = 1;
			else if (0 == i->value.compare("dagger")) mh_type = 2;
			else err("No such weapon type \"%s\".", i->value.c_str());
		}
		else if (0 == i->key.compare("oh_type")){
			if (0 == i->value.compare("2h")) oh_type = 0;
			else if (0 == i->value.compare("1h")) oh_type = 1;
			else if (0 == i->value.compare("dagger")) oh_type = 2;
			else err("No such weapon type \"%s\".", i->value.c_str());
		}
		else if (0 == i->key.compare("talent")){
			talent = atoi(i->value.c_str());
			if (talent < 0 || talent > 3333333
				|| TALENT_TIER(1) > 3 || TALENT_TIER(2) > 3
				|| TALENT_TIER(3) > 3 || TALENT_TIER(4) > 3 || TALENT_TIER(5) > 3
				|| TALENT_TIER(6) > 3 || TALENT_TIER(7) > 3)
				err("Talent set not vaild.");
		}
		else if (0 == i->key.compare("archmages_incandescence")){
			archmages_incandescence = !!atoi(i->value.c_str());
			if (archmages_incandescence) archmages_greater_incandescence = 0;
		}
		else if (0 == i->key.compare("archmages_greater_incandescence")){
			archmages_greater_incandescence = !!atoi(i->value.c_str());
			if (archmages_greater_incandescence) archmages_incandescence = 0;
		}
		else if (0 == i->key.compare("t17_2pc")){
			t17_2pc = !!atoi(i->value.c_str());
		}
		else if (0 == i->key.compare("t17_4pc")){
			t17_4pc = !!atoi(i->value.c_str());
		}
		else if (0 == i->key.compare("t18_2pc")){
			t18_2pc = !!atoi(i->value.c_str());
		}
		else if (0 == i->key.compare("t18_4pc")){
			t18_4pc = !!atoi(i->value.c_str());
		}
		else if (0 == i->key.compare("mh_enchant")){
			thunderlord_mh = !i->value.compare("thunderlord");
			bleeding_hollow_mh = !i->value.compare("bleedinghollow");
			shattered_hand_mh = !i->value.compare("shatteredhand");
			if (i->value.compare("none") && !thunderlord_mh && !bleeding_hollow_mh && !shattered_hand_mh)
				err("No such weapon enchant\"%s\".", i->value.c_str());
		}
		else if (0 == i->key.compare("oh_enchant")){
			thunderlord_oh = !i->value.compare("thunderlord");
			bleeding_hollow_oh = !i->value.compare("bleedinghollow");
			shattered_hand_oh = !i->value.compare("shatteredhand");
			if (i->value.compare("none") && !thunderlord_oh && !bleeding_hollow_oh && !shattered_hand_oh)
				err("No such weapon enchant\"%s\".", i->value.c_str());
		}
		else if (0 == i->key.compare("trinket1")){
			char* buf = new char[i->value.size()+6];
			char* p;
			strcpy(buf, i->value.c_str());
			for (p = buf; *p; p++){
				if (*p == ','){
					*p++ = 0;
					break;
				}
			}
			if (!*p || p[0]!='v' || p[1]!='a' || p[2]!='l' || p[3]!='u' || p[4]!='e' || p[5]!='=')
				if(strcmp(buf,"none")) err("Unexpected trinket grammar. Correct grammar:\n\ttrinket1=trinket_name,value=123\n\ttrinket1=none");
			trinket1_name = buf;
			if(strcmp(buf,"none"))
				trinket1_value = atoi(p+6);
			int x;
			for (x = 0; trinket_list[x]; x++){
				if ( 0 == trinket1_name.compare(trinket_list[x]) ) break;
			}
			if (!trinket_list[x]) err("No such trinket \"%s\".", trinket1_name.c_str());
			if (0 == trinket1_name.compare(trinket2_name) && 0 != trinket1_name.compare("none")) err("Duplicated trinkets \"%s\" not allowed.", trinket1_name.c_str());
			delete[] buf;
		}
		else if (0 == i->key.compare("trinket2")){
			char* buf = new char[i->value.size()+6];
			char* p;
			strcpy(buf, i->value.c_str());
			for (p = buf; *p; p++){
				if (*p == ','){
					*p++ = 0;
					break;
				}
			}
			if (!*p || p[0]!='v' || p[1]!='a' || p[2]!='l' || p[3]!='u' || p[4]!='e' || p[5]!='=')
				if(strcmp(buf,"none")) err("Unexpected trinket grammar. Correct grammar:\n\ttrinket2=trinket_name,value=123\n\ttrinket2=none");
			trinket2_name = buf;
			if(strcmp(buf,"none"))
				trinket2_value = atoi(p+6);
			int x;
			for (x = 0; trinket_list[x]; x++){
				if ( 0 == trinket2_name.compare(trinket_list[x]) ) break;
			}
			if (!trinket_list[x]) err("No such trinket \"%s\".", trinket2_name.c_str());
			if (0 == trinket1_name.compare(trinket2_name) && 0 != trinket1_name.compare("none")) err("Duplicated trinkets \"%s\" not allowed.", trinket1_name.c_str());
			delete[] buf;
		}
		else if (0 == i->key.compare("rng_engine")){
			if (0 == i->value.compare("mt127")) rng_engine = 127;
			else if (0 == i->value.compare("mwc64x")) rng_engine = 64;
			else if (0 == i->value.compare("lcg32")) rng_engine = 32;
			else err("No such rng engine \"%s\".", i->value.c_str());
		}
		else if (0 == i->key.compare("output")){
			report_path = new std::ofstream(i->value.c_str(), std::ofstream::out);
		}
		else if (0 == i->key.compare("calculate_scale_factors")){
			calculate_scale_factors = !!atoi(i->value.c_str());
		}
		else if (0 == i->key.compare("developer_debug")){
			developer_debug = !!atoi(i->value.c_str());
		}
		else if (0 == i->key.compare("list_available_devices")){
			list_available_devices = !!atoi(i->value.c_str());
		}
		else if (0 == i->key.compare("opencl_device_id")){
			ocl().opencl_device_id = atoi(i->value.c_str());
		}
		else{
			err("Cannot parse parameter \"%s\".", i->key.c_str());
		}
	}
}

void generate_predef(){
	char buffer[256];
	predef = "";
	predef.append("#define vary_combat_length ");
	sprintf(buffer, "%ff", vary_combat_length);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define max_length ");
	sprintf(buffer, "%ff", max_length);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define initial_health_percentage ");
	sprintf(buffer, "%ff", initial_health_percentage);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define death_pct ");
	sprintf(buffer, "%ff", death_pct);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define power_max ");
	sprintf(buffer, "%ff", power_max);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define PLATE_SPECIALIZATION ");
	sprintf(buffer, "%d", plate_specialization);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define SINGLE_MINDED ");
	sprintf(buffer, "%d", single_minded);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define BUFF_STR_AGI_INT ");
	sprintf(buffer, "%d", raidbuff.str);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define BUFF_AP ");
	sprintf(buffer, "%d", raidbuff.ap);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define BUFF_CRIT ");
	sprintf(buffer, "%d", raidbuff.crit);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define BUFF_HASTE ");
	sprintf(buffer, "%d", raidbuff.haste);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define BUFF_MASTERY ");
	sprintf(buffer, "%d", raidbuff.mastery);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define BUFF_MULT ");
	sprintf(buffer, "%d", raidbuff.mult);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define BUFF_VERS ");
	sprintf(buffer, "%d", raidbuff.vers);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define BUFF_SP ");
	sprintf(buffer, "%d", raidbuff.sp);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define BUFF_STA ");
	sprintf(buffer, "%d", raidbuff.sta);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define BUFF_POTION ");
	sprintf(buffer, "%d", raidbuff.potion);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define BUFF_BLOODLUST ");
	sprintf(buffer, "%d", raidbuff.bloodlust);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define RACE ");
	predef.append(race_str_kernel[race]); predef.append("\r\n");

	predef.append("#define MH_LOW ");
	sprintf(buffer, "%d", mh_low);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define MH_HIGH ");
	sprintf(buffer, "%d", mh_high);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define OH_LOW ");
	sprintf(buffer, "%d", oh_low);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define OH_HIGH ");
	sprintf(buffer, "%d", oh_high);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define MH_SPEED ");
	sprintf(buffer, "%ff", mh_speed);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define OH_SPEED ");
	sprintf(buffer, "%ff", oh_speed);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define MH_TYPE ");
	predef.append(weapon_type_str[mh_type]); predef.append("\r\n");

	predef.append("#define OH_TYPE ");
	predef.append(weapon_type_str[oh_type]); predef.append("\r\n");

	predef.append("#define TALENT_TIER3 ");
	sprintf(buffer, "%d", TALENT_TIER(3));
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define TALENT_TIER4 ");
	sprintf(buffer, "%d", TALENT_TIER(4));
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define TALENT_TIER6 ");
	sprintf(buffer, "%d", TALENT_TIER(6));
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define TALENT_TIER7 ");
	sprintf(buffer, "%d", TALENT_TIER(7));
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define archmages_incandescence ");
	sprintf(buffer, "%d", archmages_incandescence);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define archmages_greater_incandescence ");
	sprintf(buffer, "%d", archmages_greater_incandescence);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define t17_2pc ");
	sprintf(buffer, "%d", t17_2pc);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define t17_4pc ");
	sprintf(buffer, "%d", t17_4pc);
	predef.append(buffer); predef.append("\r\n");
	
	predef.append("#define t18_2pc ");
	sprintf(buffer, "%d", t18_2pc);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define t18_4pc ");
	sprintf(buffer, "%d", t18_4pc);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define thunderlord_mh ");
	sprintf(buffer, "%d", thunderlord_mh);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define thunderlord_oh ");
	sprintf(buffer, "%d", thunderlord_oh);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define bleedinghollow_mh ");
	sprintf(buffer, "%d", bleeding_hollow_mh);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define bleedinghollow_oh ");
	sprintf(buffer, "%d", bleeding_hollow_oh);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define shatteredhand_mh ");
	sprintf(buffer, "%d", shattered_hand_mh);
	predef.append(buffer); predef.append("\r\n");

	predef.append("#define shatteredhand_oh ");
	sprintf(buffer, "%d", shattered_hand_oh);
	predef.append(buffer); predef.append("\r\n");

	if(rng_engine == 127) predef.append("#define RNG_MT127\r\n");
	else if(rng_engine == 64) predef.append("#define RNG_MWC64X\r\n");

	if (trinket1_name.compare("none")){
		predef.append("#define trinket_");
		predef.append(trinket1_name);
		predef.append(" ");
		sprintf(buffer, "%d", trinket1_value);
		predef.append(buffer);
		predef.append("\r\n");
	}
	if (trinket2_name.compare("none")){
		predef.append("#define trinket_");
		predef.append(trinket2_name);
		predef.append(" ");
		sprintf(buffer, "%d", trinket2_value);
		predef.append(buffer);
		predef.append("\r\n");
	}
}

void auto_apl(){
	apl = "if(!UP(enrage.expire)||(REMAIN(bloodthirst.cd)>FROM_SECONDS(3)&&rti->player.ragingblow.stack<2))SPELL(berserkerrage);";

	if(TALENT_TIER(7) == 1 || TALENT_TIER(6) != 2) apl.append("SPELL(recklessness);");
		else apl.append("if(UP(bloodbath.expire))SPELL(recklessness);");

	if (TALENT_TIER(6) == 1) apl.append("if(UP(recklessness.expire)||REMAIN(recklessness.cd)>FROM_SECONDS(60)||rti->expected_combat_length-rti->timestamp<FROM_SECONDS(30))SPELL(avatar);");

	if (0 == strcmp(race_str_param[race], "troll"))
		if(TALENT_TIER(6) == 2) apl.append("if(UP(bloodbath.expire)||UP(recklessness.expire))SPELL(berserking);");
		else apl.append("SPELL(berserking);");

	if (0 == strcmp(race_str_param[race], "orc"))
		if(TALENT_TIER(6) == 2) apl.append("if(UP(bloodbath.expire)||UP(recklessness.expire))SPELL(bloodfury);");
		else apl.append("SPELL(bloodfury);");

	if (0 == strcmp(race_str_param[race], "bloodelf")) apl.append("if(rti->player.power<power_max-40)SPELL(arcane_torrent);");

	if (raidbuff.potion) apl.append("if((enemy_health_percent(rti)<20&&UP(recklessness.expire))||rti->expected_combat_length-rti->timestamp<FROM_SECONDS(25))SPELL(potion);");

	if (0 == trinket1_name.compare("vial_of_convulsive_shadows") || 0 == trinket2_name.compare("vial_of_convulsive_shadows"))
		if (TALENT_TIER(7) == 1) apl.append("if(UP(recklessness.expire)||rti->expected_combat_length-rti->timestamp<FROM_SECONDS(25))SPELL(vial_of_convulsive_shadows);");
		else apl.append("SPELL(vial_of_convulsive_shadows);");

	if (0 == trinket1_name.compare("scabbard_of_kyanos") || 0 == trinket2_name.compare("scabbard_of_kyanos")) apl.append("SPELL(scabbard_of_kyanos);");
	
	if (0 == trinket1_name.compare("badge_of_victory") || 0 == trinket2_name.compare("badge_of_victory"))
		if (TALENT_TIER(7) == 1) apl.append("if(UP(recklessness.expire)||rti->expected_combat_length-rti->timestamp<FROM_SECONDS(25))SPELL(badge_of_victory);");
		else apl.append("SPELL(badge_of_victory);");

	if (TALENT_TIER(6) == 2) apl.append("SPELL(bloodbath);");

	apl.append("if(rti->player.power>power_max-20&&enemy_health_percent(rti)>20)SPELL(wildstrike);");

	if(TALENT_TIER(3) == 3) apl.append("if(!UP(enrage.expire)||rti->player.ragingblow.stack<2)SPELL(bloodthirst);");
	else apl.append("if(rti->player.power<power_max-40||!UP(enrage.expire)||rti->player.ragingblow.stack<2)SPELL(bloodthirst);");

	if (TALENT_TIER(7) == 2)
		if (TALENT_TIER(6) == 2) apl.append("if(UP(bloodbath.expire))SPELL(ravager);");
		else apl.append("SPELL(ravager);");

	if (TALENT_TIER(7) == 3) apl.append("SPELL(siegebreaker);");

	if (TALENT_TIER(3) == 2) apl.append("if(UP(suddendeath.expire))SPELL(execute);");

	if (TALENT_TIER(4) == 1) apl.append("SPELL(stormbolt);");

	apl.append("if(UP(bloodsurge.expire))SPELL(wildstrike);");

	apl.append("if(UP(enrage.expire)||rti->expected_combat_length-rti->timestamp<FROM_SECONDS(12))SPELL(execute);");

	if (TALENT_TIER(4) == 3)
		if (TALENT_TIER(6) == 2) apl.append("if(UP(bloodbath.expire))SPELL(dragonroar);");
		else apl.append("SPELL(dragonroar);");

	apl.append("SPELL(ragingblow);");

	apl.append("if(REMAIN(bloodthirst.cd)<FROM_SECONDS(0.5)&&!power_check(rti,50))return;");

	apl.append("if(UP(enrage.expire)&&enemy_health_percent(rti)>20)SPELL(wildstrike);");

	if (TALENT_TIER(6) == 3) apl.append("SPELL(bladestorm);");

	if (TALENT_TIER(3) != 3 && TALENT_TIER(4) == 2) apl.append("SPELL(shockwave);");

	apl.append("SPELL(bloodthirst);");
}

void parameters_consistency(){
	single_minded = (mh_type == 1 && oh_type == 1);
	if (mh_high < mh_low){
		int t = mh_high;
		mh_high = mh_low;
		mh_low = t;
	}
	if (oh_high < oh_low){
		int t = oh_high;
		oh_high = oh_low;
		oh_low = t;
	}
	if (stat_not_pushed) {
		if (stat.name.empty()) stat.name = "<unnamed stat set>";
		stat_array.push_back(stat);
		stat_not_pushed = 0;
	}
	if (raidbuff.flask){
		for (auto i = stat_array.begin(); i != stat_array.end(); i++)
		i->gear_str += 250;
	}
	if (raidbuff.food){
		for (auto i = stat_array.begin(); i != stat_array.end(); i++)
		i->gear_crit += 125;
	}
	if (default_actions){
		auto_apl();
	}
	if (calculate_scale_factors){
		if (stat_array.size() > 1) {
			*report_path << "Scale factors enabled while multiple stat sets given. Stat sets except the first set are abbandoned." << std::endl;
		}
		stat = stat_array[0];
		stat_array.clear();
		stat.name = "scale factors baseline";
		stat_array.push_back(stat);

		stat.name = "scale factors str";
		stat.gear_str += 120;
		stat_array.push_back(stat);
		stat.gear_str -= 120;

		stat.name = "scale factors crit";
		stat.gear_crit += 120;
		stat_array.push_back(stat);
		stat.gear_crit -= 120;

		stat.name = "scale factors haste";
		stat.gear_haste += 120;
		stat_array.push_back(stat);
		stat.gear_haste -= 120;

		stat.name = "scale factors mastery";
		stat.gear_mastery += 120;
		stat_array.push_back(stat);
		stat.gear_mastery -= 120;

		stat.name = "scale factors mult";
		stat.gear_mult += 120;
		stat_array.push_back(stat);
		stat.gear_mult -= 120;

		stat.name = "scale factors vers";
		stat.gear_vers += 120;
		stat_array.push_back(stat);
		stat.gear_vers -= 120;
	}
}

void irecore_initialize(){
	set_default_parameters();
	parameters_consistency();
	generate_predef();

	ocl().init();
}

void irecore_hash(std::string& apl, float* result){
	iterations = 8;
	seed = 4262;
	ocl().run(apl, predef, result, 0);
}

void irecore_run(std::string& apl, float& dps, float& error, cl_program reuse){
	iterations = 10000;
	seed = 0;
	ocl().run(apl, predef, 0, reuse);
	dps = stat_array[0].dps;
	error = stat_array[0].dpse;
}