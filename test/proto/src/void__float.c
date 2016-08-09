
#include <stdlib.h>

// ORACLE FLOAT foo(VOID)

float foo(void) {
    int a;
    float b;
    int i;

    a = rand() % 65536;
    b = 1.0 /  a;

    for (i = 0; i < a; i++) 
        b += ((float) a) / ((float) i);

    return b;
}

int main(void) {
    
    int i;
    float a = 0, b = 0;

    for (i = 0; i < 100; i++) {
        a += foo();
        b += foo();
    }

    return 0; 
}
