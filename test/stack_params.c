#include <stdio.h>

void fn1(int r1, int r2, int r3, int r4, int r5, int r6,
		int stack1, int stack2, int stack3) {
	printf("%d\n", stack1 + stack2 + stack3);
}

void fn2(float r1, float r2, float r3, float r4,
		float r5, float r6, float r7, float r8,
		float stack1, float stack2, float stack3) {
	printf("%f\n", stack1 + stack2 + stack3);
}

void fn3(double r1, double r2, double r3, double r4,
		double r5, double r6, double r7, double r8,
		double stack1, double stack2, double stack3, double stack4) {
	printf("%d\n", stack1 + stack2 + stack3 + stack4);
}

void fn4(int r1, int r2, int r3, int* r4, int* r5, int r6,
		int* stack1, int* stack2, int stack3) {
	printf("%d\n", *stack1 + *stack2 + stack3);
}

int main() {
    int _4 = 4;
    int _5 = 5;
	int _7 = 7;
    int _8 = 8;
	for (int i = 0; i < 100; i++) {
		fn1(1, 2, 3, 4, 5, 6, 7, 8, 9);
		fn2(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11);
		fn3(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12);
		fn4(1, 2, 3, &_4, &_5, 6, &_7, &_8, 9);
	}
}
