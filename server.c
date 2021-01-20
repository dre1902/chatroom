#include "chatroom.h"

#define MAX_CL 8

/* Codes for chat commands */
enum cmd {
    C_MESSAGE,
    C_EXIT
};

struct client_t {
    /* struct sockaddr addr; */
    char name[MAX_NAME];
    int fd;
};

struct client_t cl[MAX_CL];
size_t cl_count;

pthread_mutex_t mutex;

/* Creates and binds the server socket and calls listen on it */
static int sv_init()
{
    int fd;
    struct sockaddr_in sv_addr;
    
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
       perror("socket"); 
       return -1;
    }
    sv_addr.sin_family = AF_INET;
    sv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    sv_addr.sin_port = htons(PORT_NUM);
    bind(fd, (struct sockaddr*)&sv_addr, sizeof(sv_addr));
    listen(fd, MAX_CL);

    return fd;
}

/* Sends msg to all clients except the sender */
static int sv_sendMsg(const int fd, const char *name, const char *msg)
{
    int i, namelen, msglen;

    pthread_mutex_lock(&mutex);

    for (i = 0; i < cl_count; i++)
        if (cl[i].fd != fd) {

            /* Send the name */
            namelen = htons(strlen(name));
            printf("Writing namelen of %d\n", namelen);
            if (write(cl[i].fd, (char*)&namelen, sizeof(namelen)) == -1)
                perror("write namelen");
            printf("Writing name: %s\n", name);
            if (write(cl[i].fd, name, strlen(name)) == -1)
                perror("write name");

            /* Send the message */
            msglen = htons(strlen(msg));
            printf("Writing msglen of %d\n", msglen);
            if (write(cl[i].fd, (char*)&msglen, sizeof(msglen)) == -1)
                perror("write msglen");
            printf("Writing msg: %s\n", msg);
            if (write(cl[i].fd, msg, strlen(msg)) == -1)
                perror("write msg");

        }

    pthread_mutex_unlock(&mutex);
}

/* Check if the requested name is available */
static int cl_init(struct client_t *c, const int fd)
{
    int approved, i;

    c->fd = fd;
    if(read(c->fd, c->name, MAX_NAME) <= 0) {
        perror("read name");
        return -1;
    }
    printf("New client request name of %s\n", c->name);

    approved = 1;
    for (i = 0; i < cl_count; i++)
        if (c->fd != cl[i].fd && strncmp(c->name, cl[i].name, MAX_NAME) == 0)
            approved = 0;

    if (write(c->fd, &approved, sizeof(approved)) == -1) {
        perror("write approved");
        return -1;
    }
    printf("New client name approved");
    return 0;
}

/* Parse msg to determine which command to use */
static int parse_cl(const char *msg)
{
    if (strncmp(msg, "exit", 4) == 0)
        return C_EXIT;
    return C_MESSAGE;
}

/*
 * Processes any client requests
 * Each client has their own thread running this function
 */
static void handle_cl(struct client_t *c)
{
    int cmd, fd;
    char msg[MAX_MSG];

    fd = c->fd;

    while (fd) {
        if (read(fd, msg, MAX_MSG) == -1) {
            perror("read msg");
            break;
        }
        cmd = parse_cl(msg);
        switch (cmd) {
        case C_MESSAGE:
            sv_sendMsg(fd, c->name, msg);
            break;
        case C_EXIT:
            close(fd);
            fd = 0;
            return;
        }
        memset(msg, '\0', MAX_MSG);
    }
}

int main(int argc, char *argv[])
{
    int sfd, cfd;
    struct sockaddr_storage cl_addr;
    socklen_t addrlen;
    pthread_t cl_t;

    sfd = sv_init();
    if (sfd == -1)
        exit(EXIT_FAILURE);

    while (1) {
        cfd = accept(sfd, (struct sockaddr*)&cl_addr, &addrlen);
        /*pthread_mutex_lock(&mutex);*/
        if (cfd == -1) {
            perror("accept");
            continue;
        }

        if (cl_init(&cl[cl_count], cfd) != 0) {
            close(cfd);
            continue;
        }

        /* Create a thread running handle_cl() for each client */
        if (pthread_create(&cl_t, NULL, (void*)&handle_cl, (void*)&cl[cl_count]) != 0)
            fprintf(stderr, "pthread_create\n");
        pthread_detach(cl_t);
        cl_count++;
        /*pthread_mutex_unlock(&mutex);*/
    }

    close(sfd);
    exit(EXIT_SUCCESS);
}
