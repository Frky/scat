#include <stdlib.h>
#include <stdio.h>

void test_int_out(int in, int* ptr, int* out) {
    *out = in * *ptr;
}

int main() {
    int val = 0;

    for (int i = 0; i < 100; i++) {
        int mul = 7;
        int out = 0;
        test_int_out(i, &mul, &out);
        val += out;
    }

    printf("%d\n", val);
    return 0;
}
