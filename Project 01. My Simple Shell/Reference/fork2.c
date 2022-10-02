/* fork example */

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

int pids[10]; // pid array
int count;
int flag = 0;
void alrm_handler(int signo)
{
	// send signal to child
	int idx = count %10;
	kill(pids[idx], SIGUSR1);
	printf("At time : %d! send signal to %d\n", count, pids[idx]);
	count++;

	if (count >= 30) flag=1;
}

void child_handler(int signo)
{
	printf("(%d) sigusr1 cnt: %d!\n", getpid(), count);
	count++;

	// send signal to child

	if (count >= 3) flag=1;
}
int main()
{
	int i = 0;

	struct sigaction old_sa;
	struct sigaction new_sa;

	// set up SIGALRM handler
	memset(&old_sa, 0, sizeof(old_sa));
	memset(&new_sa, 0, sizeof(new_sa));
	new_sa.sa_handler = alrm_handler;
	sigaction(SIGALRM, &new_sa, &old_sa);

	// fire!
	struct itimerval new_timer;
	struct itimerval old_timer;
	memset(&new_timer, 0, sizeof(new_timer));
	memset(&old_timer, 0, sizeof(old_timer));
	new_timer.it_interval.tv_sec = 1;
	new_timer.it_value.tv_sec = 1;
	setitimer(ITIMER_REAL, &new_timer, &old_timer);

	for (i = 0 ; i < 10 ; i++) {
		int ipid;
		int child_status;
		ipid = fork();
		if (ipid < 0) {
			perror("fork error");
		} else if (ipid == 0) {
			// child
			// set up SIGALRM handler
			memset(&old_sa, 0, sizeof(old_sa));
			memset(&new_sa, 0, sizeof(new_sa));
			new_sa.sa_handler = child_handler;
			sigaction(SIGUSR1, &new_sa, &old_sa);


			count = 0;
			flag = 0;
			while (!flag) {
				sleep(1);
			}
			printf("pid: %d, done. make sure you're not creating additional proc.\n", getpid());
			return 0;
		} else {
			// parent

			pids[i] = ipid;
			//ipid = wait(&child_status);
			//printf("(%d) proc %d completed.\n", getpid(), ipid);
		}
	}

	while (!flag) {
		sleep(1);
	}

	return 0;
}
















