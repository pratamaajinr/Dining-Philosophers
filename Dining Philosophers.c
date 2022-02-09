#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define N 5
#define THINKING 0
#define HUNGRY 1
#define EATING 2

#define LEFT (i + N-1) % N
#define RIGHT (i + 1) % N

int semun;
int i;

struct shm{
    int state[N];
}*shared_memory;

// Create Shared memory
void SharedMemory(){
    int shmID;
	shmID = shmget(IPC_PRIVATE, sizeof(*shared_memory), IPC_CREAT | 0x1ff);
    shared_memory = (struct shm*)shmat(shmID, NULL, 0);
    if (shmID == -1) {
      printf("Error\n");
      return 1;
    }else{
        printf("Shmat succeed\n");
    }
}

// UP fuction
void up(int semnr) {
    struct sembuf my_sem_b;
    my_sem_b.sem_num = semnr;
    my_sem_b.sem_op = 1; // Up the semaphore
    my_sem_b.sem_flg = 0;
    semop(semun, & my_sem_b, 1);
}

// DOWN fuction
void down(int semnr) {
    struct sembuf my_sem_b;
    my_sem_b.sem_num = semnr;
    my_sem_b.sem_op = -1; // Down the semaphore
    my_sem_b.sem_flg = 0;
    semop(semun, & my_sem_b, 1);
}

void test(int i){
	if (shared_memory->state[i] == HUNGRY && shared_memory->state[LEFT] != EATING && shared_memory->state[RIGHT] != EATING){
		shared_memory->state[i]=EATING;
		up(i);
	}
}

// Take fork
void take_forks(int i){
	down(N);// Down mutex
	printf("Philosopher [%d] is hungry.\n", i);
	shared_memory->state[i]=HUNGRY;
	test(i);
	up(N);
	down(i);
}

// Put forks
void put_forks(int i){
	down(N);// Down mutex
	shared_memory->state[i]=THINKING;
	test(LEFT);
	test(RIGHT);
	up(N);
}

void think(){
	printf("Philosopher [%d] is thinking.\n", i);
}

void eat(){
	printf("Philosopher [%d] is eating.\n", i);
}

// Philosopher process
void philosopher(int i){
	while(1){
		think();
		sleep(1);
		take_forks(i);
		eat();
		sleep(1);
		put_forks(i);
	}
}

int main(){
	int j;

	//Initialize Shared memory
	SharedMemory();

	//Initialize Semaphores
	semun = semget(IPC_PRIVATE, N + 1, IPC_CREAT | 0x1ff);
	printf("Memory attached at shmID: %d\n", semun);
	if (semun == -1) {
		printf("Semaphore Error\n");
		exit(1);
	}

	ushort semvals[N];
    union semun  {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
    } arg;

    semctl(semun, 0, IPC_STAT, arg);
	for(j = 0; j < N ; j++)semvals[j] = 0;
	arg.array = semvals;
    semctl(semun, 0, SETALL, arg);
    arg.val = 1;
    semctl(semun, j, SETVAL, arg);

	int pid;
	for(i = 0; i < N; i++){
		pid=fork();
		if (pid == 0) break;
	}

	if (pid == 0){
		philosopher(i);
	} else if (pid < 0){
		printf("Error\n");
		exit(1);
	} else {
		wait(NULL);
	}

	return 0;
}