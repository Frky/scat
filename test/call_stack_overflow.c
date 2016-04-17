/*
 * Ce programme montre deux cas identiques d'appels récursifs.
 * Un fonction "start" appel une fonction recursive "rec". La
 * fonction "start" retourne la valeur de retour de "rec" sans y
 * toucher. De la même manière la fonction "rec" pour son cas
 * de base appelle une fonction "end" dont elle renvoie directement
 * le résultat.
 * Autrement dit on est dans un cas ou la valeur de retour de "end"
 * doit "remonter" la pile d'appel depuis son accés dans la fonction
 * appelant "start" jusqu'à son écriture dans la fonction "end" en
 * passant par les appels de "rec".
 *
 * Actuellement, la taille de la pile d'appel est fixée, et en cas
 * d'overflow les appels les moins récents sont "écrasés" au profit
 * des plus récents.
 *
 * L'exemple montre que :
 *      - Dans le cas (start1, rec1, end1) avec peu d'appels recursifs
 *        le retour de "start" est bien detecté par scat
 *      - Dans le cas (start2, rec2, end2) avec suffisament d'appel pour
 *        déclencher l'overflow, le retour de "start" n'est
 *        pas détecté.
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int end1(int param1, char* param2) {
    return rand();
}

int rec1(int param1, char* param2, int n) {
	return n == 0
	        ? end1(param1, param2)
	        : rec1(param1, param2, n - 1);
}

int start1(int param1, char* param2) {
	return rec1(param1, param2, 10);
}

int end2(int param1, char* param2) {
    return rand();
}

int rec2(int param1, char* param2, int n) {
	return n == 0
			? end2(param1, param2)
			: rec2(param1, param2, n - 1);
}

int start2(int param1, char* param2) {
	return rec2(param1, param2, 15000);
}

int main() {
	int val = 0;
	for (int i = 0; i < 100; i++) {
		val += start1(10, "Test Overflow");
		val += start2(10, "Test Overflow");
	}

	return 0;
}
