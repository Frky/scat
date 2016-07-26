
#include <stdlib.h>

// ORACLE VOID foo(FLOAT)

void foo(float a) {
    float b;
    int i;

    b = ((float) (rand() % 65536)) / ((float) (rand() % 65536));

    for (i = 0; i < b; i++) 
        a += b;

    // Avoid compilation warning
    b = a;

    return;
}

int main(void) {
    
    int i;

    for (i = 0; i < 1000; i++) {
        foo(((float) (rand() % 65536)) / ((float) (rand() % 65536)));
    }

    return 0; 
}
