/*
 * This test highlights that even with a huge recursion,
 * all relevant functions are detected (start, rec, end),
 * despite the fact that our arity and type pintools only
 * keep tracks of only a limited subset of the call stack.
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// ORACLE INT start(INT, ADDR)
// ORACLE INT rec(INT, ADDR, INT)
// ORACLE INT end(INT, ADDR)

int end(int param1, char* param2) {
    return rand();
}

int rec(int param1, char* param2, int n) {
    return n == 0
            ? end(param1, param2)
            : rec(param1, param2, n - 1);
}

int start(int param1, char* param2) {
    return rec(param1, param2, 15000);
}

int main() {
    int val = 0;
    for (int i = 0; i < 100; i++) {
        val += start(10, "Test Overflow");
    }

    return 0;
}
