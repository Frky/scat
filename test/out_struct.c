#include <stdlib.h>
#include <stdio.h>

typedef struct {
    int field1;
    int field2;
    int field3;
} strct;

void test_struct_out(int in, strct* out) {
    out->field2 = in * 7;
}

int main() {
    int val = 0;

    for (int i = 0; i < 100; i++) {
        int mul = 7;
        strct out = {0,};
        test_struct_out(i, &out);
        val += out.field2;
    }

    printf("%d\n", val);
    return 0;
}
