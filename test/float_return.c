/**
 * Les types de retour 'float' sont détectées
 */

#include <stdlib.h>
#include <stdio.h>

float float_value() {
	return (float) rand();
}

int main() {
	float val = 0;
	for (int i = 0; i < 100; i++) {
		val += float_value();
	}

	printf("%f\n", val);
	return 0;
}