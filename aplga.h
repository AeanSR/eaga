#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <cstdio>
#include <intrin.h>
#include <stdint.h>
#include <algorithm>
#include "irecore.h"

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

std::mt19937& rng();
double uni_rng();
double nor_rng();

class condnode_t{
private:
    int type;
    int leaf;
public:
    condnode_t(int _type, int _leaf = 0) : type(_type), leaf(_leaf) {};
    condnode_t* l;
    condnode_t* r;
    int complexity();
    void print(std::string& str);
    void alternate();
    void addition();
    void deletion();
    void cascade_destruction();
    condnode_t(const condnode_t& copy) : type(copy.type), leaf(copy.leaf){
        if (copy.l) l = new condnode_t(*(copy.l));
        else l = nullptr;
        if (copy.r) r = new condnode_t(*(copy.r));
        else r = nullptr;
    }
    const condnode_t& operator= (const condnode_t& copy){
        type = copy.type;
        leaf = copy.leaf;
        cascade_destruction();
        if (copy.l) l = new condnode_t(*(copy.l));
        else l = nullptr;
        if (copy.r) r = new condnode_t(*(copy.r));
        else r = nullptr;
    }
};


class cond_t{
private:
    condnode_t* root;
public:
    int complexity();
    cond_t() : root(nullptr) {};
    void mutation();
    void chiasma(cond_t& mate);
    void print(std::string& str);
    cond_t(const cond_t& copy) {
        if (copy.root) root = new condnode_t(*(copy.root));
        else root = nullptr;
    };
    const cond_t& operator= (const cond_t& copy){
        if (root){
            root->cascade_destruction();
            delete root;
        }
        if (copy.root) root = new condnode_t(*(copy.root));
        else root = nullptr;
    }
    ~cond_t(){
        if (root){
            root->cascade_destruction();
            delete root;
        }
    }
};

class actionnode_t{
public:
    int action;
    cond_t cond;
    actionnode_t* next;
    actionnode_t(int _action) : action(_action){};
    int complexity();
    int length();
    void addition();
    void deletion();
    void alternate();
    void chiasma(actionnode_t** mate);
    void condmutation();
    void condchiasma(actionnode_t** mate);
    void cascade_destruction();
    void print(std::string& str);
    actionnode_t(actionnode_t& copy) : cond(copy.cond), action(copy.action){
        if (copy.next){
            next = new actionnode_t(*copy.next);
        }
        else{
            next = nullptr;
        }
    }
    const actionnode_t& operator=(const actionnode_t& copy){
        cascade_destruction();
        action = copy.action;
        cond = copy.cond;
        if (copy.next){
            next = new actionnode_t(*copy.next);
        }
        else{
            next = nullptr;
        }
    }
};

class apl_t{
private:
    actionnode_t* head;
public:
    apl_t();
    int complexity();
    int length();
    void mutation();
    void chiasma(apl_t& mate);
    void print(std::string& str);
    apl_t(apl_t& copy){
        if (copy.head){
            head = new actionnode_t(*copy.head);
        }
    }
    ~apl_t(){
        if (head){
            head->cascade_destruction();
            delete head;
        }
    }
    const apl_t& operator=(const apl_t& copy){
        if (head){
            head->cascade_destruction();
            delete head;
        }
        if (copy.head){
            head = new actionnode_t(*copy.head);
        }
        else{
            head = nullptr;
        }
    }
};


typedef struct {
    apl_t* apl;
    float dps;
	float dpse;
	float fitness;
	float fitness_error;
	int iterations;
    int complexity;
	cl_program saved_kernel;
}lelem_t;

bool hash_apl(lelem_t& elem);

extern int equal_apls;
extern int repeated_apls;