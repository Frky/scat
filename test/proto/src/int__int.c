
#include <stdlib.h>

// ORACLE INT FOO(INT)

int foo(int a) {
    int b;
    int i;

    b = rand() % 65536;

    for (i = 0; i < b; i++) 
        a += b;

    return a;
}

int main(void) {
    
    int a = 0, i;

    for (i=0; i < 10000; i++) {
        a += foo(rand() % 65536);
    }

    return a; 
}
