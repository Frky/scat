#include <stdlib.h>

// ORACLE INT foo(INT, INT, INT, ADDR, ADDR, INT, ADDR, INT, ADDR, INT)

int foo(int r1, int r2, int r3, int* r4, int* r5, int r6,
        int* stack1, int stack2, int* stack3, int stack4) {
    return r1 + r2 + r3 + *r4 + *r5 + r6
            + *stack1 + stack2 + *stack3 + stack4;
}

int main() {
    int _4 = 4;
    int _5 = 5;
	int _7 = 7;
    int _9 = 9;

	for (int i = 0; i < 100; i++) {
		foo(1, 2, 3, &_4, &_5, 6, &_7, 8, &_9, 10);
	}
}
