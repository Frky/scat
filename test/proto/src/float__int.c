
#include <stdlib.h>

// ORACLE FLOAT foo(INT)

float foo(int a) {
    float b;
    int i;

    b = ((float) (rand() % 65536)) / ((float) (rand() % 65536));

    for (i = 0; i < 100; i++) 
        b /= a;

    return b;
}

int main(void) {
    
    int i;
    float a = 0;

    for (i = 0; i < 1000; i++) {
        a += foo(rand() % 65536);
    }

    return 0; 
}
