#include "chatroom.h"

#define PROMPT "Enter message > "

pthread_mutex_t mutex;
/*
 * curr is used to keep track of the cursor
 * when messages from the server are printed
 */
int curr;

/* Request approval from the server using the requested name */
static int cl_setUser(const int fd, const char *name)
{
    char approved;

    if (write(fd, name, strlen(name)) != strlen(name)) {
        perror("write");
        exit(EXIT_FAILURE);
    }
    /*printf("wrote name to server\n");*/

    if (read(fd, &approved, sizeof(approved)) != sizeof(approved)) {
        perror("read");
        exit(EXIT_FAILURE);
    }
    /*printf("read approved from server\n");*/

    return approved;
}

/* Prints the message to the next row and move the cursor back */
static void print_msg(const char *name, const char *msg)
{
    static int row = 1;

    pthread_mutex_lock(&mutex);

    printf("\033[%d;0f<%s> %s", row++, name, msg);
    /*printf("\033[40;0f\033[K%s", PROMPT);*/
    printf("\033[40;%df", curr);

    pthread_mutex_unlock(&mutex);
}

/* On its own thread. Recieves messages from the server and calls print_msg */
static void recv_msg(int *fd)
{
    int namelen, msglen;
    char name[MAX_NAME];
    char msg[MAX_MSG];

    while (1) {

        /* Recieves the name */
        if (read(*fd, (char*)&namelen, sizeof(namelen)) == -1) {
            perror("read namelen");
            return;
        }
        namelen = ntohs(namelen);
        /*printf("read namelen of %d\n", namelen);*/
        if (read(*fd, name, namelen) == -1) {
            perror("read name");
            return;
        }
        /*printf("read name: %s\n", name);*/

        /* Recieves the message */
        if (read(*fd, (char*)&msglen, sizeof(msglen)) == -1) {
            perror("read msglen");
            return;
        }
        msglen = ntohs(msglen);
        /*printf("read msglen of %d\n", msglen);*/
        if (read(*fd, msg, msglen) == -1) {
            perror("read msg");
            return;
        }
        /*printf("read msg: %s\n", msg);*/

        print_msg(name, msg);
        printf("\033[30;30fWHY IS THIS NOT WORKING!!!");
        memset(name, 0, MAX_NAME);
        memset(msg, 0, MAX_MSG);
    }
}

/* Reads input from user and places it in msg */
static int get_input(char *msg)
{
    int c, i;

    curr = strlen(PROMPT);
    i = 0;
    memset(msg, 0, MAX_MSG);
    do {
        c = getchar();
        curr++;
        if (i > MAX_MSG) {
            curr = 0;
            return -1;
        }
        msg[i++] = c;
    } while (c != '\n');
    curr = strlen(PROMPT);

    return 0;
}

int main(int argc, char *argv[])
{
    int fd, s;
    char msg[MAX_MSG];
    struct sockaddr_in sv_addr;
    pthread_t fetch_t;

    if (argc != 3) {
        printf("usage error\n");
        exit(EXIT_FAILURE);
    }

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    sv_addr.sin_family = AF_INET;
    sv_addr.sin_port = htons(PORT_NUM);
    s = inet_pton(AF_INET, argv[1], &sv_addr.sin_addr);
    if (s <= 0) {
        if (s == 0)
            fprintf(stderr, "Address not in proper format");
        else
            perror("inet_pton");
        exit(EXIT_FAILURE);
    }
    
    if (connect(fd, (struct sockaddr*)&sv_addr, sizeof(sv_addr)) == -1) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    if (strlen(argv[2]) > MAX_NAME) {
        printf("Username too long\n");
        exit(EXIT_FAILURE);
    }

    if (cl_setUser(fd, argv[2]) != 1) {
        printf("Username not available\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&fetch_t, NULL, (void*)&recv_msg, (void*)&fd) != 0) {
        fprintf(stderr, "pthread_create\n");
        exit(EXIT_FAILURE);
    }
    pthread_detach(fetch_t);

    read(fd, (char*)&s, sizeof(s));

    curr = strlen(PROMPT);
    printf("\033[2J");
    printf("\033[40;0f\033[K%s", PROMPT);
    while (1) {
        get_input(msg);
        /* Print the user's message and then send it to the server */
        print_msg(argv[2], msg);
        if (write(fd, msg, strlen(msg)) != strlen(msg))
            perror("write");
    }

    exit(EXIT_SUCCESS);
}
