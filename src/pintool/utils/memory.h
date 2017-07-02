#ifndef MEMORY_H_
#define MEMORY_H_

#include <map>
std::map<ADDRINT, long> allocated;
ADDRINT last_allocated = 0;

int fake_alloc(ADDRINT addr, long size) {
    std::cerr << "alloc(" << size << "): " << addr << endl;
    allocated[addr] = size;
    last_allocated = addr;
    return 0;
}

int fake_free(ADDRINT addr) {
    std::cerr << "free(" << addr << ")\n";
    if (addr == 0) {
        return 0;
    }

    if (allocated.find(addr) == allocated.end()) {
        return 1;
    } else {
        allocated.erase(addr);
        return 0;
    }
}

bool is_allocated(ADDRINT addr) {
    std::map<ADDRINT, long>::iterator i = allocated.begin();
    while (i->first < addr && i->second + i->first - 1 < addr && i != allocated.end()) {
        i++;
    }

    if (addr >= i->first && addr <= i->first + i->second - 1 ) {
        return true;
/*
    } else if ( addr < last_allocated - 100000 || addr > last_allocated + 100000 )  {
        return true;
*/
    } else {
        return false;
    }

}    


#endif
