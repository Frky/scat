
#ifndef __FTABLE_H__
#define __FTABLE_H__

#include "../utils/functions_registry.h"

void log_ftable(ofstream &ofile) {
    FID fid = 1;
    ofile << _fn_nb - 1 << endl;
    while (fid < _fn_nb) {
        ofile << fn_img(fid) << ":" << fn_imgaddr(fid) << ":" << fn_name(fid) << endl; 
        fid++;
    }
}

#endif
