
#include <stdlib.h>

// ORACLE VOID foo(VOID)

void foo(void) {
    int a, b;
    int i;

    a = rand() % 65536; 
    b = rand() % 65536;

    for (i = 0; i < b; i++) 
        a += b;

    // to avoid warning
    i = a; 

    return;
}

int main(void) {
    int i;
    for (i = 0; i < (rand() % 65536); i++)
        foo();
    return 0; 
}
