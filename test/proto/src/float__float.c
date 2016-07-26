
#include <stdlib.h>

// ORACLE FLOAT foo(FLOAT)

float foo(float a) {
    float b;
    int i;

    b = ((float) (rand() % 65536)) / ((float) (rand() % 65536));

    for (i = 0; i < b; i++) 
        a += b;

    // Avoid compilation warning
    b = a;

    return b;
}

int main(void) {
    
    int i;
    float a = 0;

    for (i = 0; i < 1000; i++) {
        a += foo((float) (rand() % 65536)) / ((float) (rand() % 65536));
    }

    return a; 
}
