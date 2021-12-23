//Program 1 of the distributed mutex lock check

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include "psu_lock.h"

void spin();

int lockno;

int main(int argc, char* argv[]) {
	if(argc < 2) {
		printf("params: <lockno>\n");
		return 0;
	}

	lockno = atoi(argv[1]);

	if(lockno < 0 || lockno > LOCK_COUNT) {
		printf("lockno out of bounds. try again.\n");
		return 0;
	}
	
	psu_init_lock(lockno);
	printf("[p] !psu_lock initialized!\n");
	while(1) {
		psu_mutex_lock(lockno);
		spin();
		psu_mutex_unlock(lockno);
		sleep(rand() % 6);
	}
	return 0;
}

void spin() {
	int i = 1;
	while(i < 5) {
		printf("[p] cspin: %d\n", i);
		i++;
		sleep(1);
	}
}
