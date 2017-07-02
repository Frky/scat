
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>

#include "pin.H"

#include "read.h"

typedef struct {
    string img_name;
    ADDRINT img_addr;
    string name; 
    unsigned int nb_param;
    vector<bool> type_param;
} fn_type_t;

void read_parameters(std::ifstream &ifile, fn_type_t *fn) {
    char m = '!';
    fn->nb_param = 0;
    while (ifile && m != '\n') {
        ifile.read(&m, 1);
        if (m == 'A')
            fn->type_param.push_back(true);
        else if (m == 'I' || m == 'V' || m == 'U')
            fn->type_param.push_back(false);
        else
            continue;
        fn->nb_param += 1;
        while (ifile && m != '\n' && m != ',')
            ifile.read(&m, 1);
    }
    return;
}

fn_type_t *read_one_type(std::ifstream &ifile) {
    if (!ifile.is_open())
        return NULL;
    fn_type_t *fn = new fn_type_t;
    fn->img_name = read_part(ifile);
    fn->img_addr = atol(read_part(ifile).c_str());
    fn->name = read_part(ifile);

    read_parameters(ifile, fn);

    return fn;
}

