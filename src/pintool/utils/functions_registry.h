#ifndef __FUNCTIONS_REGISTRY_H__
#define __FUNCTIONS_REGISTRY_H__

#include "pin.H"
#include "debug.h"

#include <tr1/unordered_map>

// Type of the unique ID given to each registered
// function allowing for fast lookup
typedef unsigned int FID;

// The unknown function ID
#define FID_UNKNOWN 0

// Type of the function lookup return value
// Use `fn_is_new` to check is the function got created
// and `fid_of` to get the corresponding FID
typedef long int FN_LOOKUP;

struct FnKey {
    string* img_name;
    ADDRINT img_addr;

    bool operator==(const FnKey &other) const {
        return *img_name == *other.img_name
                && img_addr == other.img_addr;
    }
};

struct fn_key_hash {

    std::size_t operator()(const FnKey& key) const {
        return 289
                + 17 * key.img_addr
                + std::hash<std::string>()(*key.img_name);
    }
};

unsigned int _fn_max_nb;

/* Number of functions we watch
   All futher arrays index from 0 to _fn_nb (included) */
unsigned int _fn_nb;

/* The couple (image(=binary), address in image)
   forms a unique identifier for each function */
FnKey* _fn_key;

/* Name of each function (if symbol table is present) */
string **_fn_name;

std::tr1::unordered_map<FnKey, FID, fn_key_hash> _fn_map;

// Initialize the functions registry
//   * Allocates all necessary space
//   * Create the unknown function entry
void fn_registry_init(unsigned int fn_max_nb) {
    _fn_max_nb = fn_max_nb + 1;

    _fn_key = (FnKey *) calloc(_fn_max_nb, sizeof(FnKey));
    _fn_name = (string **) calloc(_fn_max_nb, sizeof(string *));

    _fn_key[FID_UNKNOWN].img_name = new string("<unknown>");
    _fn_key[FID_UNKNOWN].img_addr = 0;
    _fn_name[FID_UNKNOWN] = new string("<unknown>");

    _fn_map[_fn_key[FID_UNKNOWN]] = FID_UNKNOWN;

    _fn_nb = 1;
}

inline unsigned int fn_nb() {
    return _fn_nb;
}

// Registers a function with the given informations
// Returns the newly created function entry FID
FID fn_register(FnKey key, string name) {
    if (_fn_nb >= _fn_max_nb) {
        return FID_UNKNOWN;
    }

    FID fid = _fn_nb;
    _fn_nb++;

    _fn_key[fid].img_name = key.img_name;
    _fn_key[fid].img_addr = key.img_addr;
    _fn_name[fid] = new string(name);

    _fn_map[_fn_key[fid]] = fid;

    return fid;
}

inline string fn_img(FID fid) {
    return *(_fn_key[fid].img_name);
}

inline ADDRINT fn_imgaddr(FID fid) {
    return _fn_key[fid].img_addr;
}

inline string fn_name(FID fid) {
    return *(_fn_name[fid]);
}

// Registers a function using informations from the given RTN
// Returns the newly created function entry FID
FID fn_register_from_rtn(RTN rtn) {
    IMG img = SEC_Img(RTN_Sec(rtn));
    FnKey fn_key;
    fn_key.img_name = new string(IMG_Name(img));
    ADDRINT img_offset = IMG_LoadOffset(img);
    fn_key.img_addr = RTN_Address(rtn) - img_offset;
    string name = RTN_Name(rtn);
    return fn_register(fn_key, name);
}

// Looks up an already registered function from its runtime address
// or registers it if not found
// NOTE: Require PIN Lock
FN_LOOKUP fn_lookup_or_register(ADDRINT runtime_addr) {
    IMG img = IMG_FindByAddress(runtime_addr);
    if (!IMG_Valid(img)) {
        return FID_UNKNOWN;
    }

    FnKey fn_key;
    fn_key.img_name = new string(IMG_Name(img));
    ADDRINT img_offset = IMG_LoadOffset(img);
    fn_key.img_addr = runtime_addr - img_offset;

    // Lookup the function by address
    if (_fn_map.count(fn_key) > 0) {
        return _fn_map[fn_key];
    }

    // Or register it without a name
    if (IMG_Valid(img)) {
        return -((FN_LOOKUP) fn_register(fn_key, ""));
    }
    else {
        // Some (rare) call target are not part of a
        // registered image, we won't be able
        // to identify them across all pintools.
        // Ignore them.
        return FID_UNKNOWN;
    }
}

inline bool fn_is_new(FN_LOOKUP fn_lookup) {
    return fn_lookup < 0;
}

inline FID fid_of(FN_LOOKUP fn_lookup) {
    return (FID) (fn_lookup >= 0 ? fn_lookup : -fn_lookup);
}

#endif
