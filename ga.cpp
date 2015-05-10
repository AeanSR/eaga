#include "aplga.h"


lelem_t list[270];

void eval(int idx){
	if (list[idx].iterations == 0){
		if (!hash_apl(list[idx])){
			list[idx].dps = -1;
			return;
		}
	}
    std::string str;
    list[idx].apl->print(str);

    float dps;
	float error;
    irecore_run(str, dps, error);
    if (dps < .0) abort();
	list[idx].dps = (list[idx].dps * list[idx].iterations + dps * 10000) / (list[idx].iterations + 10000);
	error = error * error * 10000 * 10000;
	list[idx].dpse = list[idx].dpse * list[idx].dpse * list[idx].iterations * list[idx].iterations;
	list[idx].dpse += error;
	list[idx].dpse /= list[idx].iterations + 10000;
	list[idx].dpse /= list[idx].iterations + 10000;
	list[idx].dpse = sqrt(list[idx].dpse);
	list[idx].iterations += 10000;

	list[idx].fitness = list[idx].dps / (1.0 + exp(2 * (list[idx].apl->complexity() - 14)));
	list[idx].fitness_error = list[idx].dpse / (1.0 + exp(2 * (list[idx].apl->complexity() - 14)));
}

void fitness_sort(int first, int last){
    if (last - first > 1){
        int i = first + 1;
        int j = last;
        float key = list[first].fitness;
        while (1){
            while (key > list[j].fitness)
                j--;
            while (key < list[i].fitness && i<j)
                i++;
            if (i >= j) break;
            auto tmpelem = list[i];
            list[i] = list[j];
            list[j] = tmpelem;
            if (list[i].fitness == key)
                j--;
            else
                i++;
        }
        auto tmpelem = list[j];
        list[j] = list[first];
        list[first] = tmpelem;
        if (first < i - 1) fitness_sort(first, i - 1);
        if (j + 1 < last) fitness_sort(j + 1, last);
    }
    else{
        if (list[first].fitness < list[last].fitness){
            auto tmpelem = list[first];
            list[first] = list[last];
            list[last] = tmpelem;
        }
    }
}


int main(){
    int i;
    unsigned int gen = 0;
    
    FILE* fdata = fopen("data.txt", "wb");
    FILE* fbest = fopen("best.txt", "wb");
    
	irecore_initialize();

    /* init genetic. */
    for (i = 0; i < 200; i++){
        list[i].apl = new apl_t;
        eval(i);
        while (list[i].dps <= .0){
            list[i].apl->mutation();
            eval(i);
        }
    }
    while (1){
        /* chiasma */
        for (i = 200; i < 240; i+=2){
            int j = i + 1;
            int m1 = static_cast<int>(200 * uni_rng());
            int m1r = static_cast<int>(200 * uni_rng());
            m1 = m1 < m1r ? m1 : m1r;
            int m2 = static_cast<int>(200 * uni_rng());
            int m2r = static_cast<int>(200 * uni_rng());
            m2 = m2 < m2r ? m2 : m2r;

            list[i].apl = new apl_t(*list[m1].apl);
            list[j].apl = new apl_t(*list[m2].apl);
            list[i].apl->chiasma(*list[j].apl);
        }
        /* mutation */
        for (i = 240; i < 260; i++){
            int m = static_cast<int>(200 * uni_rng());
            int mr = static_cast<int>(200 * uni_rng());
            m = m < mr ? m : mr;

            list[i].apl = new apl_t(*list[m].apl);
            list[i].apl->mutation();
        }
        /* double mutation */
        for (i = 260; i < 270; i++){
            int m = static_cast<int>(200 + 60 * uni_rng());
            int mr = static_cast<int>(200 + 60 * uni_rng());
            m = m < mr ? m : mr;

            list[i].apl = new apl_t(*list[m].apl);
            list[i].apl->mutation();
        }
        /* evaluation */
        for (i = 200; i < 270; i++){
            eval(i);
        }
        /* sort */
        fitness_sort(0, 269);
        int fine = 0;
		while (!fine){
			fine = 1;
			for (i = 0; i < 200; i++){
				for (int j = 200; j < 270; j++){
					if (1.47*(list[i].fitness_error + list[j].fitness_error) > abs(list[i].fitness - list[j].fitness)){
						if (list[i].dpse > 3){
							eval(i);
							fine = 0;
						}
						if (list[j].dpse > 3){
							eval(j);
							fine = 0;
						}
					}
				}
			}
			fitness_sort(0,269);
		}
        /* select */
        for (i = 200; i < 270; i++){
            list[i].dps = .0;
			list[i].dpse = .0;
			list[i].fitness = .0;
			list[i].fitness_error = .0;
			list[i].iterations = 0;
            delete list[i].apl;
            list[i].apl = nullptr;
        }
        /* record */
        gen++;
        std::string str;
        list[0].apl->print(str);
        rewind(fbest);
        fprintf(fbest, "DPS %.3f, APL:\n%s\n\n**********\n", list[0].dps, str.c_str());
        printf("Gen %d, Max %.3f, Min %.3f. Best APL:\n%s\n", gen, list[0].dps, list[99].dps, str.c_str());
        fprintf(fdata, "%d, %.3f, %.3f\n", gen, list[0].dps, list[99].dps);
        fflush(fbest);
        fflush(fdata);
    }
}