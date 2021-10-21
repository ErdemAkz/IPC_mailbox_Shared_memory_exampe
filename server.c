#include <sys/msg.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/shm.h>
#include <pthread.h>

struct msgbuf {
    long mType;
    char mText[100];
};

void *worker_thread(void*);

int main() {

    int qId;
    key_t key;
    struct msgbuf msg, buf;

    //CREATE MAIN THREAD'S QUEUE
    if ((key = ftok("/tmp", 'C')) == -1) {
        perror("server: ftok failed:");
        exit(1);
    }

    printf("server: System V IPC key = %u\n", key);

    if ((qId = msgget(key, IPC_CREAT | 0666)) == -1) {
        perror("server: Failed to create message queue:");
        exit(2);
    }

    printf("server: Message queue id = %u\n", qId);
    printf("server is waiting for messages...\n");
    while (1){
        //WAIT FOR CLIENTS TO WRITE THEIR PIDs TO QUEUE
        if (msgrcv(qId, &buf, sizeof msg.mText, 1, 0) == -1) {
            perror("server: msgrcv failed:");
            printf("main thread mesaj alirken sorun yasadi!!\n");
            return 1;
        }
        int pid = (intptr_t)atoi(buf.mText);
        pthread_t t;

        //CREATE THREAD
        int iret1 = pthread_create( &t, NULL, worker_thread, (void*)(intptr_t) pid);
        pthread_detach(t);
    }

}

void *worker_thread(void* ptr){
    int pid_for_key = (intptr_t) ptr;
    printf("----------Thread Started its job for pId: %d----------\n",pid_for_key);

    struct msgbuf msg, buf;
    struct msqid_ds msgCtlBuf;
    int pid = pid_for_key;


    //CREATE 2 QUEUES TO COMMUNICATE WITH CLIENT
    int qId_rcv,qId_snd;
    if ((qId_snd = msgget(pid, IPC_CREAT | 0666)) == -1) {
        perror("server: Failed to create message queue:");
        exit(2);
    }

    if ((qId_rcv = msgget(pid + 32768, IPC_CREAT | 0666)) == -1) {
        perror("server: Failed to create message queue:");
        exit(2);
    }

    //GET THE MESSAGE CONTAINING ARRAY SIZES
    if (msgrcv(qId_rcv, &buf, sizeof msg.mText, 1, 0) == -1)
        perror("server: msgrcv failed:");
    else
        printf("server: message received => \"%s\"\n", buf.mText);

    //PARSE MESSAGE
    char *end = buf.mText;
    int arr_size[4];
    for (int i = 0; i < 4; i++) {
        int n = strtol(buf.mText, &end, 10);
        arr_size[i] = n;
        while (*end == ',') {
            end++;
        }
        strcpy(buf.mText, end);
    }
    //CALCULATE THE SIZE TO CREATE SHARED MEMOR
    int BUF_SIZE = arr_size[0] * arr_size[1] * sizeof(int) + arr_size[2] * arr_size[3] * sizeof(int) +
                   arr_size[0] * arr_size[3] * sizeof(int);
    key_t SHM_KEY = pid;
    printf("buf_size: %d\nkey: %d\n", BUF_SIZE, SHM_KEY);


    int shmid;
    void *virtaddr;

    shmid = shmget(SHM_KEY, BUF_SIZE, 0666|IPC_CREAT);
    printf("shmid value is %d\n", shmid);

    virtaddr = shmat(shmid, 0, 0);


    strcpy(msg.mText, "shared memory olusturuldu.");
    msg.mType = 1;

    if (msgsnd(qId_snd, &msg, sizeof msg.mText, 0) == -1) {
        perror("server: msgsnd failed:");
        exit(3);
    }
    else
        printf("server: message sent => \"%s\"\n",msg.mText);


    int *index = (int *) virtaddr;

    int *index1 = (int *) virtaddr;
    int *index2 = (int *) virtaddr;
    int *index3 = (int *) virtaddr;

    index2 += arr_size[0]*arr_size[1];
    index3 += arr_size[0]*arr_size[1] + arr_size[2]*arr_size[3];
    int curr;

    int a[arr_size[0]][arr_size[1]];
    int b[arr_size[2]][arr_size[3]];

    if (msgrcv(qId_rcv, &buf, sizeof msg.mText, 1, 0) == -1)
        perror("server: msgrcv failed:");
    else
        printf("server: message received => \"%s\"\n",buf.mText);


    int R1 = arr_size[0];
    int C1 = arr_size[1];
    int R2 = arr_size[2];
    int C2 = arr_size[3];

    int rslt[R1][C2];

    //MATRIX MULTIPLICATION
    for (int i = 0; i < R1; i++) {
        for (int j = 0; j < C2; j++) {
            rslt[i][j] = 0;

            for (int k = 0; k < R2; k++) {
                //rslt[i][j] += a[i][k] * b[k][j];
                rslt[i][j] += (*(index1+i*arr_size[1]+k)) * (*(index2+k*arr_size[3]+j));

            }
            *index3 = rslt[i][j];
            index3++;
        }
    }


    //LET THE CLIENT KNOW THE RESULT IS READY TO BE READ
    strcpy(msg.mText, "matrix carpimi tamamlandi.");
    if ( msgsnd( qId_snd, &msg, sizeof msg.mText, 0 ) == -1 ) {
        perror( "server: msgsnd failed:" );
        exit( 3 );
    }
    else
        printf("server: message sent => \"%s\"\n",msg.mText);

    shmdt(virtaddr);
    printf("---------------DONE----------------\n");
}