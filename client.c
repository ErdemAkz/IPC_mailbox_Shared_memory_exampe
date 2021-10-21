#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <string.h>
#include<sys/shm.h>

int row1_len,col1_len;
int row2_len,col2_len;

//function that reads from the file and puts integer into an array
int** read_ints (const char* file_name, int i)
{
    FILE* file = fopen (file_name, "r");
    int row = 0;
    int col = 0;
    fscanf(file,"%d %d",&row,&col);

    if (i == 1){
        row1_len = row;
        col1_len = col;
    }else if(i == 2){
        row2_len = row;
        col2_len = col;
    }

    int **arr=(int **)malloc(sizeof(int *)*row);
    for (int i = 0; i < row; i++) {

        arr[i]=(int *)malloc(sizeof(int)*col);
        for (int j = 0; j < col; j++) {

            fscanf(file,"%d", &arr[i][j]); //&arr[(i*j) + j]

        }
    }

    fclose (file);
    return arr;
}

struct msgbuf {
    long mType;
    char mText[100];
};

int main(int argc, char *argv[]) {

    if (argc < 3){
        printf("kullanim: ./client array1.txt array2.txt\n");
        printf("array1[a][b]  array[b][c]\n");
        printf("          ^         ^\n");
        return 1;
    }

    int pid=getpid(); /*call returns id of this process */
    printf("Client started with pId: %d\n",pid);
    int** arr1 = read_ints(argv[1],1);
    int** arr2 = read_ints(argv[2],2);


    if (col1_len != row2_len){
        printf("matris 1 in sutun uzunlugu matris 2 nin satir uzunluguna esit degil!\n");
        printf("kullanim: ./client array1.txt array2.txt\n");
        printf("array1[a][b]  array[b][c]\n");
        printf("          ^         ^\n");
        return 1;
    }

    int qId;
    key_t key;
    struct msgbuf msg, buf;
    struct msqid_ds msgCtlBuf;

    //GET THE MESSAGE QUEUE KEY THE SERVER USES
    if ( ( key = ftok( "/tmp", 'C' ) ) == -1 ) {
        perror( "client: ftok failed:" );
        exit( 1 );
    }
    //GET THE MESSAGE QUEUE ID THE SERVER USES
    if ( ( qId = msgget( key, IPC_CREAT | 0666 ) ) == -1 ) {
        perror( "client: Failed to create message queue:" );
        exit( 2 );
    }

    char mesaj_pid[] = "";
    char pid_as_string[30];
    sprintf(pid_as_string, "%d", pid);
    strcat(mesaj_pid,pid_as_string);
    strcpy( msg.mText, mesaj_pid );
    msg.mType = 1;

    //WRITE PROCESS ID TO QUEUE OF MAIN SERVER THREAD
    if ( msgsnd( qId, &msg, sizeof msg.mText, 0 ) == -1 ) {
        perror( "client: msgsnd failed:" );
        exit( 3 );
    }
    else
        printf("client: message sent => \"%s\"\n",msg.mText);


    //arr[5][10]  arr2[10][4]  => CREATE MESSAGE "5,10,10,4"
    char mesaj[] = "";
    char row1_s[10];
    char col1_s[10];
    char row2_s[10];
    char col2_s[10];
    sprintf(row1_s, "%d", row1_len);
    sprintf(col1_s, "%d", col1_len);
    sprintf(row2_s, "%d", row2_len);
    sprintf(col2_s, "%d", col2_len);
    strcat(mesaj,row1_s);strcat(mesaj,",");
    strcat(mesaj,col1_s);strcat(mesaj,",");
    strcat(mesaj,row2_s);strcat(mesaj,",");
    strcat(mesaj,col2_s);


    strcpy( msg.mText, mesaj );
    msg.mType = 1;

    //CREATE 2 QUEUES TO COMMUNICATE WITH WORKER THREAD
    int qId_rcv,qId_snd;
    if ( ( qId_rcv = msgget( pid, IPC_CREAT | 0666 ) ) == -1 ) {
        perror( "client: Failed to create message queue:" );
        exit( 2 );
    }
    if ( ( qId_snd = msgget( pid+32768, IPC_CREAT | 0666 ) ) == -1 ) {
        perror( "client: Failed to create message queue:" );
        exit( 2 );
    }

    //SEND ARRAY SIZES TO NEW QUEUE
    if ( msgsnd( qId_snd, &msg, sizeof msg.mText, 0 ) == -1 ) {
        perror( "client: msgsnd failed:" );
        exit( 3 );
    }
    else
        printf("client: message sent => \"%s\"\n",msg.mText);



    //GET THE CONFIRMATION OF THE SHARED MEMORY CREATED OR NOT
    if ( msgrcv( qId_rcv, &buf, sizeof msg.mText, 1, 0 ) == -1 )
        perror( "client: msgrcv failed:" );
    else
        printf( "client: Message received = \"%s\"\n", buf.mText );

    int BUF_SIZE = row1_len * col1_len * sizeof(int) + row2_len * col2_len * sizeof(int) + row1_len * col2_len * sizeof(int);
    key_t SHM_KEY;

    SHM_KEY = pid;
    printf("buf_size: %d\nkey: %d\n",BUF_SIZE,SHM_KEY);

    int  shmid;
    void  *virtaddr;
    int  *p;

    shmid = shmget(SHM_KEY, BUF_SIZE, 0);
    printf("shmid value is %d\n",shmid);

    virtaddr = shmat(shmid, (void *)0, 0); /* Attach the shared segment */
    //printf("virtual address 0x%x\n",virtaddr);


    p = (int *)virtaddr; /* Put two integer values into it */

    //WRITE ARRAYS TO SHARED MEMORY
    for (int i = 0; i < row1_len; i++) {
        for (int j = 0; j < col1_len; j++) {
            *p = arr1[i][j];
            p++;
        }
    }
    for (int i = 0; i < row2_len; i++) {
        for (int j = 0; j < col2_len; j++) {
            *p = arr2[i][j];
            p++;
        }
    }

    strcpy(msg.mText, "shared memory'ye veri yazildi.");
    if ( msgsnd( qId_snd, &msg, sizeof msg.mText, 0 ) == -1 ) {
        perror( "client: msgsnd failed:" );
        exit( 3 );
    }
    else
        printf("client: message sent => \"%s\"\n",msg.mText);


    if (msgrcv(qId_rcv, &buf, sizeof msg.mText, 1, 0) == -1)
        perror("client: msgrcv failed:");
    else
        printf("client: message received => \"%s\"\n",buf.mText);

    //READ SHARED MEMORY FOR RESULTS
    printf("Result:\n");
    for (int i = 0; i < row1_len; i++) {
        for (int j = 0; j < col2_len; j++) {
            printf("%d ",*p);
            p++;
        }
        printf("\n");
    }

    //DELETE QUEUES AND SHARED MEMORY
    shmdt(virtaddr);
    if (shmctl(shmid, IPC_RMID, 0) == -1) {
        perror("shmctl");
        return 1;
    }
    if ( msgctl( qId_snd, IPC_RMID, &msgCtlBuf ) == -1 ) {
        perror( "client: msgctl failed:" );
        exit( 4 );
    }
    if ( msgctl( qId_rcv, IPC_RMID, &msgCtlBuf ) == -1 ) {
        perror( "client: msgctl failed:" );
        exit( 4 );
    }

    return 0;
}


