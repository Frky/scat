#ifndef __FUNCTIONS_REGISTRY_H__
#define __FUNCTIONS_REGISTRY_H__

#include "pin.H"
#include "debug.h"

#include <tr1/functional>

// Type of the unique ID given to each registered
// function allowing for fast lookup
typedef unsigned int FID;

// The unknown function ID
#define FID_UNKNOWN 0

struct FnEntry {
    /* The couple (image(=binary), address in image)
       forms a unique identifier for each function */
    FID fid;
    string* img_name;
    ADDRINT img_addr;

    /* Name of the function (only available if symbol table is) */
    string* fn_name;

    /* Next entry in the hash table in case of collision */
    FnEntry* next;

    unsigned int hash;
};

unsigned int _fn_max_nb;

/* Number of functions we watch
   All futher arrays index from 0 to _fn_nb (included) */
unsigned int _fn_nb;

FnEntry* _entries_by_fid;
unsigned int _hash_mask;
FnEntry** _entries_by_hash;

// Initialize the functions registry
//   * Allocates all necessary space
//   * Create the unknown function entry
void fn_registry_init(unsigned int fn_max_nb) {
    unsigned int next_pot = 1;
    while (next_pot < fn_max_nb) {
        next_pot *= 2;
    }

    _fn_max_nb = fn_max_nb + 1;

    _entries_by_fid = (FnEntry*) calloc(_fn_max_nb, sizeof(FnEntry));

    FnEntry* unknown_entry = _entries_by_fid + FID_UNKNOWN;
    unknown_entry->fid = FID_UNKNOWN;
    unknown_entry->img_name = new string("<unknown>");
    unknown_entry->img_addr = 0;
    unknown_entry->fn_name = new string("<unknown>");

    _hash_mask = next_pot - 1;
    _entries_by_hash = (FnEntry**) calloc(next_pot, sizeof(FnEntry*));

    _fn_nb = 1;
}

inline unsigned int fn_nb() {
    return _fn_nb;
}

string basename(string img_name) {
    std::size_t found = img_name.find_last_of("/");
    return found == string::npos
        ? img_name
        : img_name.substr(found+1);
}

inline unsigned int fn_hash(string img_name, ADDRINT img_addr) {
    unsigned int hash = 289
                 + 17 * img_addr
                 + std::tr1::hash<std::string> () (img_name);
    return hash & _hash_mask;
}

// Registers a function with the given informations
// Returns the newly created function entry FID
FID fn_register(string img_name, ADDRINT img_addr, string name) {
    if (_fn_nb >= _fn_max_nb) {
        return FID_UNKNOWN;
    }

    string img_basename = basename(img_name);

    FID fid = _fn_nb;
    _fn_nb++;

    FnEntry* entry = _entries_by_fid + fid;
    entry->fid = fid;
    entry->img_name = new string(img_basename);
    entry->img_addr = img_addr;
    entry->fn_name = new string(name);

    FnEntry** bucket = _entries_by_hash + fn_hash(img_basename, img_addr);
    entry->next = *bucket;
    *bucket = entry;

    return fid;
}

inline string fn_img(FID fid) {
    return *(_entries_by_fid[fid].img_name);
}

inline ADDRINT fn_imgaddr(FID fid) {
    return _entries_by_fid[fid].img_addr;
}

inline string fn_name(FID fid) {
    return *(_entries_by_fid[fid].fn_name);
}

// Registers a function using informations from the given RTN
// Returns the newly created function entry FID
FID fn_register_from_rtn(RTN rtn) {
    IMG img = SEC_Img(RTN_Sec(rtn));
    if (!IMG_Valid(img)) {
        return FID_UNKNOWN;
    }

    string img_name = IMG_Name(img);
    ADDRINT img_offset = IMG_LoadOffset(img);
    ADDRINT img_addr = RTN_Address(rtn) - img_offset;
    string name = RTN_Name(rtn);
    return fn_register(img_name, img_addr, name);
}

// Registers a function using informations for the given address
// with an empty name
// Returns the newly created function entry FID
FID fn_register_from_address(ADDRINT runtime_addr) {
    IMG img = IMG_FindByAddress(runtime_addr);
    if (!IMG_Valid(img)) {
        return FID_UNKNOWN;
    }

    string img_name = IMG_Name(img);
    ADDRINT img_offset = IMG_LoadOffset(img);
    ADDRINT img_addr = runtime_addr - img_offset;
    return fn_register(img_name, img_addr, "");
}

// Looks up an already registered function
// NOTE: Require PIN Lock
FID fn_lookup(IMG img, ADDRINT runtime_addr) {
    if (!IMG_Valid(img)) {
        return FID_UNKNOWN;
    }

    string img_name = basename(IMG_Name(img));
    ADDRINT img_addr = runtime_addr - IMG_LoadOffset(img);

    FnEntry* entry = _entries_by_hash[fn_hash(img_name, img_addr)];
    while (entry != NULL
            && (img_name != *(entry->img_name)
            || img_addr != entry->img_addr)) {
        entry = entry->next;
    }

    return entry == NULL
            ? FID_UNKNOWN
            : entry->fid;
}

// Looks up an already registered function based on a rtn
// NOTE: Require PIN Lock
FID fn_lookup_by_rtn(RTN rtn) {
    IMG img = SEC_Img(RTN_Sec(rtn));
    return fn_lookup(img, RTN_Address(rtn));
}

// Looks up an already registered function from its runtime address
// NOTE: Require PIN Lock
FID fn_lookup_by_address(ADDRINT runtime_addr) {
    IMG img = IMG_FindByAddress(runtime_addr);
    return fn_lookup(img, runtime_addr);
}

double fn_bucket_mean_size() {
    double bucket_count = 0;
    double entries = 0;
    for (unsigned int i = 0; i <= _hash_mask; i++) {
        FnEntry* entry = _entries_by_hash[i];
        if (entry != NULL) {
            bucket_count++;

            while (entry != NULL) {
                entries++;
                entry = entry->next;
            }
        }
    }

    return entries / bucket_count;
}

#endif
