/*
 * Ce programme montre deux cas identiques d'appels récursifs.
 * Un fonction "test" appel une fonction recursive "rec". La
 * fonction "test" retourne la valeur de retour de "rec" sans y
 * toucher.
 * Autrement dit on est dans un cas ou la valeur de retour de "rec"
 * doit "remonter" la pile d'appel depuis son accés dans la fonction
 * appelant "test" jusqu'à son écriture dans la fonction "rec".
 *
 * Actuellement, la taille de la pile d'appel est fixée, et en cas
 * d'overflow les appels les moins récents sont "écrasés" au profit
 * des plus récents.
 *
 * L'exemple montre que :
 *      - Dans le cas (test1, rec1) avec peu d'appels recursifs
 *        le retour de "test" est bien detecté par scat
 *      - Dans le cas (test2, rec2) avec suffisament d'appel pour
 *        déclencher l'overflow, le retour de "test" n'est
 *        pas détecté.
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int rec1(int param1, char* param2, int n) {
	return n > 0
			? 1 + rec1(param1, param2, n - 1)
			: rand();
}

int test1(int param1, char* param2) {
	printf("%d - %s    ", param1, param2);
	return rec1(param1, param2, 10);
}

int rec2(int param1, char* param2, int n) {
	return n > 0
			? 1 + rec2(param1, param2, n - 1)
			: rand();
}

int test2(int param1, char* param2) {
	printf("%d - %s    ", param1, param2);
	return rec2(param1, param2, 15000);
}

int main() {
	int val = 0;
	for (int i = 0; i < 100; i++) {
		val += test1(10, "Test Overflow");
		val += test2(10, "Test Overflow");
	}

	return 0;
}

/*
Résultats avant correction :
	('0x7f17cc0ccf80', ('check_match', 6, 1))
	('0x7f17cc0cd120', ('do_lookup_x', 6, 1))
	('0x7f17b86b77f0', ('random_r', 2, 1))
	('0x400400', ('', 3, 0))
	('0x7f17b86c5020', ('_itoa_word', 4, 1))
	('0x7f17cc0cdc90', ('_dl_lookup_symbol_x', 6, 1))
*	('0x4005eb', ('test2', 2, 0))
	('0x7f17cc0dd4e0', ('strcmp', 6, 1))
	('0x7f17b86c7940', ('vfprintf', 3, 1))
	('0x4005b0', ('rec_overflow2', 3, 1))
	('0x400571', ('test1', 2, 1))
	('0x7f17b870a1a0', ('strchrnul', 7, 1))
	('0x400536', ('rec_overflow1', 3, 1))
	('0x7f17b86b7660', ('random', 0, 1))
	('0x400420', ('', 0, 1))
	('0x7f17b8708260', ('__memcpy_sse2', 3, 1))

Résultats après correction :
	('0x7f0ff386df80', ('check_match', 6, 1))
	('0x7f0ff386e120', ('do_lookup_x', 6, 1))
	('0x7f0fdfe597f0', ('random_r', 2, 1))
	('0x7f0fdfeaa260', ('__memcpy_sse2', 3, 1))
	('0x7f0fdfe67020', ('_itoa_word', 4, 1))
	('0x7f0ff386ec90', ('_dl_lookup_symbol_x', 6, 1))
*	('0x4005eb', ('test2', 2, 1))
	('0x7f0ff387e4e0', ('strcmp', 6, 1))
	('0x4005b0', ('rec2', 3, 1))
	('0x400571', ('test1', 2, 1))
	('0x7f0fdfeac1a0', ('strchrnul', 7, 1))
	('0x400536', ('rec1', 3, 1))
	('0x7f0fdfe69940', ('vfprintf', 3, 1))
	('0x7f0fdfe59660', ('random', 0, 1))
	('0x400420', ('', 0, 1))
	('0x400400', ('', 3, 0))

*/