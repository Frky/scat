
#include <stdlib.h>

// ORACLE VOID foo(INT)

void foo(int b) {
    int a;
    int i;

    a = rand(); 

    for (i = 0; i < b; i++) 
        a += b;

    // to avoid warning
    i = a; 

    return;
}

int main(void) {
    int i;
    for (i = 0; i < rand() % 65536; i++)
        foo(rand() % 65536);
    return 0; 
}

