/*
 *  Thread creation that represents race condition.
 *
 *  T1; increase balance
 *  T2; decrease balance
 *  balance is a global variable.
 */

#include <stdio.h>
#include <pthread.h>

void* deposit(void* arg);
void* withdraw(void* arg);

pthread_mutex_t lock;

// global variable
int balance = 0x1000;

int main(void) {
    // thread creation
    pthread_t t1, t2;
    int *res1;
    int *res2;

    pthread_mutex_init(&lock, NULL);
    pthread_create(&t1, NULL, deposit, NULL);
    pthread_create(&t2, NULL, withdraw, NULL);

    pthread_join(t1, (void**)&res1);
    pthread_join(t2, (void**)&res2);
    return 0;
}

void* deposit(void* arg) {
    for (int i = 0; i < 0x1000; i++) {
	for (int j = 0; j < 0x100; j++) {
	    for (int k = 0; k < 0x10; k++) {
		// critical section
		pthread_mutex_lock(&lock);
		balance += 1;
		pthread_mutex_unlock(&lock);
	    }
	}
	printf("balance: %d (dep_index %d)\n", balance, i);
    }
    return 0;
}

void* withdraw(void* arg) {
    for (int i = 0; i < 0x1000; i++) {
	for (int j = 0; j < 0x100; j++) {
	    for (int k = 0; k < 0x10; k++) {
		// critical section
		pthread_mutex_lock(&lock);
		balance -= 1;
		pthread_mutex_unlock(&lock);
	    }
	}
	printf("balance: %d (wit_index %d)\n", balance, i);
    }
    return 0;
}      
