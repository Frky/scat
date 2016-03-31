/**
 * Les types de retour 'int' sont détectées
 */

#include <stdlib.h>
#include <stdio.h>

int int_value() {
	return (int) (rand() * 10.0);
}

int main() {
	int val = 0;
	for (int i = 0; i < 100; i++) {
		val += int_value();
	}

	printf("%d\n", val);
	return 0;
}
