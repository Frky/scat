#ifndef __FUNCTIONS_REGISTRY_H__
#define __FUNCTIONS_REGISTRY_H__

#include "pin.H"

typedef unsigned int FID;
#define FID_UNKNOWN 0

unsigned int _fn_max_nb;
void (*_fn_registered)(FID fid);

/* Number of functions we watch
   All futher arrays index from 0 to _fn_nb (included) */
unsigned int _fn_nb;

/* The couple (_fn_img[fid], _fn_imgaddr[fid])
   forms a unique identifier for the function */
/* Name of image containing each function */
string **_fn_img;
/* Address inside its image for each function */
ADDRINT *_fn_imgaddr;

/* Execution/Virtual address inside the running program memory
   for each function */
ADDRINT *_fn_addr;
/* Name of each function (if symbol table is present) */
string **_fn_name;

void fn_registry_init(unsigned int fn_max_nb, void (*fn_registered)(FID fid)) {
    _fn_max_nb = fn_max_nb + 1;
    _fn_registered = fn_registered;

    _fn_img = (string **) calloc(_fn_max_nb, sizeof(string *));
    _fn_imgaddr = (ADDRINT *) calloc(_fn_max_nb, sizeof(ADDRINT));
    _fn_addr = (ADDRINT *) calloc(_fn_max_nb, sizeof(ADDRINT));
    _fn_name = (string **) calloc(_fn_max_nb, sizeof(string *));

    _fn_img[FID_UNKNOWN] = new string("<unknown>");
    _fn_imgaddr[FID_UNKNOWN] = 0;
    _fn_addr[FID_UNKNOWN] = 0;
    _fn_name[FID_UNKNOWN] = new string("<unknown>");

    _fn_registered(0);

    _fn_nb = 1;
}

inline unsigned int fn_nb() {
    return _fn_nb;
}

FID fn_register(IMG img, ADDRINT addr, string name) {
    if (_fn_nb >= _fn_max_nb) {
        return FID_UNKNOWN;
    }

    FID fid = _fn_nb;
    _fn_nb++;

    _fn_img[fid] = new string(IMG_Name(img));
    _fn_imgaddr[fid] = addr - IMG_LoadOffset(img);
    _fn_addr[fid] = addr;
    _fn_name[fid] = new string(name);

    _fn_registered(fid);

    return fid;
}

FID fn_register_from_rtn(RTN rtn) {
    IMG img = SEC_Img(RTN_Sec(rtn));
    ADDRINT addr = RTN_Address(rtn);
    string name = RTN_Name(rtn);
    return fn_register(img, addr, name);
}

FID fn_get_or_register(ADDRINT addr) {
    // Lookup the function by address
    for (FID fid = 1; fid < _fn_nb; fid++) {
        if (_fn_addr[fid] == addr)
            return fid;
    }

    // Or register it without a name
    IMG img = IMG_FindByAddress(addr);
    if (IMG_Valid(img)) {
        return fn_register(img, addr, "");
    }
    else {
        // Some (rare) call target are not part of a
        // registered image, we won't be able
        // to identify them across all pintools.
        // Ignore them.
        return FID_UNKNOWN;
    }
}

inline string fn_img(FID fid) {
    return *(_fn_img[fid]);
}

inline ADDRINT fn_imgaddr(FID fid) {
    return _fn_imgaddr[fid];
}

inline ADDRINT fn_addr(FID fid) {
    return _fn_addr[fid];
}

inline string fn_name(FID fid) {
    return *(_fn_name[fid]);
}

#endif
