
#include <stdlib.h>

// ORACLE INT foo(VOID)

int foo(void) {
    int a, b;
    int i;

    a = rand() % 65536;
    b = rand() % 65536;

    for (i = 0; i < b; i++) 
        a += b;

    return a % 65536;
}

int main(void) {
    
    int a = 0, b = 0, i;

    for (i = 0; i < 100; i++) {
        a += foo();
        b += foo();
    }

    return a + b; 
}
