#include <stdarg.h>
#include <stdio.h>

int sum(int p1, ...) {
	va_list args;
	va_start(args, p1);

	int sum = 0;
	for (;;) {
		int arg = va_arg(args, int);
		if (arg == p1) {
			break;
		}

		sum += arg;
	}

	va_end(args);
	return sum;
}


int main() {
	for (int i = 0; i < 100; i++) {
		printf("%d\n", sum(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0));
	}
}