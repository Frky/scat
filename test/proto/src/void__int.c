
#include <stdlib.h>

// ORACLE INT FOO(VOID)

int foo(void) {
    int a, b;
    int i;

    a = rand() % 65536;
    b = rand() % 65536;

    for (i = 0; i < b; i++) 
        a += b;

    return a;
}

int main(void) {
    
    int a = foo();
    int b = foo();

    return a + b; 
}
