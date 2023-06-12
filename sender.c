#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>


int main(int argc, char *argv[]){



    if (argc != 2) {
        printf("expected 1 argument(ringbuffer size), got %d\n", argc - 1);
        exit(1);
    }
    int bufferSize = atoi(argv[1]);
    if (bufferSize < 1) {
        printf("expected positive integer, got %d\n", bufferSize);
        exit(1);
    }





    key_t senderKey = ftok("sender", bufferSize);
    if (senderKey == -1) {
        printf("ftok failed\n");
        exit(1);
    }
    key_t bufferKey = ftok("buffer", bufferSize);
    if (bufferKey == -1) {
        printf("ftok failed\n");
        exit(1);
    }
    key_t senderKey2 = ftok("sender2", bufferSize);
    if (senderKey2 == -1) {
        printf("ftok failed\n");
        exit(1);
    }
    key_t receiverKey2 = ftok("receiver2", bufferSize);
    if (receiverKey2 == -1) {
        printf("ftok failed\n");
        exit(1);
    }





    int senderLockSemId = semget(senderKey, 1, IPC_EXCL | IPC_CREAT | 0666 );
    if (senderLockSemId == -1) { 
        if (errno == EEXIST) { //semaphore for sender already exists
            senderLockSemId = semget(senderKey, 1, 0666); //get the semaphore
        } else {
            printf("semget failed\n");
            exit(1);
        }
    } else { //if sender was just created Set its value to 1 so that someone can start sending
        semctl(senderLockSemId, 0, SETVAL, 1);
    }





    struct sembuf sembufLock = {.sem_num = 0, .sem_op = -1, .sem_flg = IPC_NOWAIT};
    if (semop(senderLockSemId, &sembufLock, 1) == -1) {
        printf("semop lock failed\n");
        exit(1);
    }





    int senderIndexShmId = shmget(senderKey, sizeof(int), IPC_EXCL | IPC_CREAT | 0666);
    if (senderIndexShmId == -1) {
        if (errno == EEXIST) { //senderIndex already exists
            senderIndexShmId = shmget(senderKey, sizeof(int), 0666); //get the senderIndex
        } else {
            printf("shmget failed\n");
            exit(1);
        }
    } else { //if senderIndex was just created Set its value to 0 so that someone can start sending
        int *senderIndex = shmat(senderIndexShmId, NULL, 0);
        *senderIndex = 0;
        shmdt(senderIndex);
    } 
    int *senderIndex = shmat(senderIndexShmId, NULL, 0);





    int bufferShmId = shmget(bufferKey, bufferSize * sizeof(char), IPC_EXCL | IPC_CREAT | 0666);
    if (bufferShmId == -1) {
        if (errno == EEXIST) { //buffer already exists
            bufferShmId = shmget(bufferKey, bufferSize * sizeof(char), 0666); //get the buffer
        } else {
            printf("shmget failed\n");
            exit(1);
        }
    } else { //if buffer was just created Set its value to 0 so that someone can start sending
        char *buffer = shmat(bufferShmId, NULL, 0);
        for (int i = 0; i < bufferSize; i++) {
            buffer[i] = 0;
        }
        shmdt(buffer);
    } 
    char *buffer = shmat(bufferShmId, NULL, 0);





    int senderSemId2 = semget(senderKey2, 1, IPC_EXCL | IPC_CREAT | 0666 );
    if (senderSemId2 == -1) { 
        if (errno == EEXIST) { 
            senderSemId2 = semget(senderKey2, 1, 0666); 
        } else {
            printf("semget failed\n");
            exit(1);
        }
    } else { 
        semctl(senderSemId2, 0, SETVAL, bufferSize); //number of free spaces in buffer
    }

    int receiverSemId2 = semget(receiverKey2, 1, IPC_EXCL | IPC_CREAT | 0666 );
    if (receiverSemId2 == -1) { 
        if (errno == EEXIST) { 
            receiverSemId2 = semget(receiverKey2, 1, 0666); 
        } else {
            printf("semget failed\n");
            exit(1);
        }
    } else { 
        semctl(receiverSemId2, 0, SETVAL, 0); //number of free spaces in buffer
    }

    char c;
    while((c = getchar()) != EOF) {
        struct sembuf sembuf1 = {.sem_num = 0, .sem_op = -1, .sem_flg = 0};
        if (semop(senderSemId2, &sembuf1, 1) == -1) {
            printf("semop lock failed\n");
            exit(1);
        }
        buffer[*senderIndex] = c;
        *senderIndex = (*senderIndex + 1) % bufferSize;
        struct sembuf sembuf2 = {.sem_num = 0, .sem_op = 1, .sem_flg = 0};
        if (semop(receiverSemId2, &sembuf2, 1) == -1) {
            printf("semop unlock failed\n");
            exit(1);
        }
    }







    struct sembuf sembufUnlock = {.sem_num = 0, .sem_op = 1, .sem_flg = 0};
    if (semop(senderLockSemId, &sembufUnlock, 1) == -1) {
        printf("semop unlock failed\n");
        exit(1);
    }



   
    return 0;
}
