#include <stdlib.h>

// ORACLE FLOAT foo(FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT)

float foo(float r1, float r2, float r3, float r4,
        float r5, float r6, float r7, float r8,
        float stack1, float stack2, float stack3) {
    return r1 + r2 + r3 + r4
            + r5 + r6 + r7 + r8
            + stack1 + stack2 + stack3;
}

int main() {
	for (int i = 0; i < 100; i++) {
		foo(1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11.);
	}
}
