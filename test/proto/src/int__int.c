
#include <stdlib.h>

// ORACLE INT foo(INT)

int foo(int a) {
    int b;
    int i;
    int res = 0; 

    b = rand() % 65536;

    for (i = 0; i < b; i++) 
        res += a;

    return res % 65536;
}

int main(void) {
    
    int a = 0, i;

    for (i=0; i < 1000; i++) {
        a += foo(rand() % 65536);
    }

    return 0; 
}
