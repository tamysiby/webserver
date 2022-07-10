//
//  main.c
//  WebServer
//
//  Created by Tamy Siby on 2021/03/23.
//  Based on : ShellWave (YouTube) for backbones of socket programming and ozgurhepsag (GitHub) for basics of pthreads

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <pthread.h>
#include <semaphore.h>

#define PATH "/Users/tamysiby/Desktop/sem6/cloud-computing/WebServer/src/" //download the src file and then insert the pathname to that file.

char *get_req(char *buffer);                    //function parses GET, then returns requested data according to request
void get_webpage(int client_fd, char *req);     //function takes in the request, and then writes the requested page to the client
void html_handler(int socket, char *file_name); //function handles getting local html files
void *connection_handler(void *client_fd);      //connection handler for the thread

int t_count = 0;
sem_t mutex;

//error page
char errorpage[] =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n\r\n"
    "<!DOCTYPE html>\r\n"
    "<html><head><title>Tamy</title>"
    "<style>body { background-color: #FFFFFF }</style></head>\r\n"
    "<body><center><br><br><h2>The page you're searching for doesn't exist.</h2><br>\r\n"
    "<br><h4>Count to 10 slowly and then try again;)</h4>\r\n"
    "<p>If it's your 5th time here after refreshing, then sorry the page doesn't exist:'(</p><br>\r\n"
    "</center></body></html>\r\n";

int main(int argc, const char *argv[])
{
    sem_init(&mutex, 0, 1);   //semaphore is for resource handling when multi-threading
    int server_fd, client_fd; // return value for socket func
    int *new_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t sin_len = sizeof(client_addr);
    int on = 1;

    server_fd = socket(AF_INET, SOCK_STREAM, 0); //initializing a socket
    if (server_fd < 0)
    {
        perror("socket creating failed");
        exit(1);
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)); //optional, but will be safer when working with sockets

    //defining socket characteristics
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    //binding socket to address
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind failed");
        close(server_fd);
        exit(1);
    }

    printf("Will listen to localhost:8080\n");

    //socket will listen for incoming connection.
    if (listen(server_fd, 5) == -1)
    {
        perror("listen failed");
        close(server_fd);
        exit(1);
    }
    printf("Listening to port...\n");

    //accept function to accept connection from client
    while ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &sin_len)))
    {
        printf("Connected.\n");

        //creating thread
        pthread_t thread_id;
        //defining new sockets for different threads
        new_socket = malloc(1);
        *new_socket = client_fd;

        //creating thread
        if (pthread_create(&thread_id, NULL, connection_handler, (void *)new_socket) < 0)
        {
            printf("Creating thread failed.\n");
            return 1;
        }
    }

    return 0;
}

//connection handler
void *connection_handler(void *sock_desc)
{
    char *req = NULL;
    char buffer[2048];

    int sock = *((int *)sock_desc);

    //reads the data in the socket to the buffer which includes the GET request
    read(sock, buffer, 2047);

    sem_wait(&mutex);
    t_count++;

    //only allowing 10 requests at the same time
    if (t_count > 10)
    {
        write(sock, errorpage, sizeof(errorpage) - 1);
        t_count--;
        sem_post(&mutex);
        free(sock_desc);
        shutdown(sock, SHUT_RDWR);
        close(sock);
        sock = -1;
        pthread_exit(NULL);
    }
    sem_post(&mutex);

    sem_wait(&mutex);
    req = get_req(buffer);
    sem_post(&mutex);

    //My failed attemt at catching the favicon. This will affect the t_count above.
    // if (strcmp(req, "favicon") == 0) // Discard the favicon.ico requests.
    // {
    //     printf("favico shiz\n");
    //     sem_wait(&mutex);
    //     t_count--;
    //     sem_post(&mutex);
    //     free(sock_desc);
    //     shutdown(sock, SHUT_RDWR);
    //     close(sock);
    //     sock = -1;
    //     pthread_exit(NULL);
    // }
    // else
    // {

    //semaphore functions to be safe
    sem_wait(&mutex);
    get_webpage(sock, req);
    sem_post(&mutex);
    // }

    //free(sock_desc);
    shutdown(sock, SHUT_RDWR);
    close(sock);
    sock = -1;
    sem_wait(&mutex);
    t_count--;
    sem_post(&mutex);
    pthread_exit(NULL);

    return 0;
}

//function parses GET, then returns request
char *get_req(char *buffer)
{
    char *res = NULL;
    char *first = NULL; //contains first line
    res = malloc(sizeof(char));
    first = malloc(sizeof(char));
    int j = 5;

    for (int i = 0; i < 30; i++)
    {
        first[i] = buffer[i];
        if (buffer[i] == '\n')
        {
            break;
        }
    }

    for (int i = 0; i < strlen(first); i++)
    {
        if (first[j] == ' ')
        {
            break;
        }
        res[i] = first[j];
        j++;
    }

    free(first);
    return res;
    //will return NULL if nothing in the URL
}

//get_webpage() to show the webpage to the client
void get_webpage(int client_fd, char *req)
{
    //pink is in code, yellow is in separate html
    char *request = req;

    char *html = malloc(6);
    char *extension = malloc(6);
    char n[1] = "\0";
    char pink[4] = "pink";

    //to parse out the request and get the .html extension
    strcpy(html, ".html");
    extension[strlen(extension)] = '\0';
    strncpy(extension, (request + (strlen(request) - 5)), 6); //checks for .html file

    //pages that the program provides
    char pinkweb[] =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n\r\n"
        "<!DOCTYPE html>\r\n"
        "<html><head><title>Tamy</title>"
        "<style>body { background-color: #fad0c4; color: #ffffff }</style></head>\r\n"
        "<body><center><h1>It's pink~</h1><br>\r\n"
        "<body><center><h3>hello, welcome to the pink page!</h3><br>\r\n"
        "</center></body></html>\r\n";

    char original[] =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n\r\n"
        "<!DOCTYPE html>\r\n"
        "<html><head><title>Tamy</title>"
        "<style>body { background-color: #FFFFFF }</style></head>\r\n"
        "<body><center><h1>Hello World!</h1><br>\r\n"
        "</center></body></html>\r\n";

    if ((strcmp(request, n) == 0))
    {
        write(client_fd, original, sizeof(original) - 1);
        close(client_fd);
        return;
    }
    else if ((strcmp(request, pink) == 0))
    {
        write(client_fd, pinkweb, sizeof(pinkweb) - 1);
        close(client_fd);
        return;
    }
    else if ((strcmp(extension, html) == 0))
    {
        html_handler(client_fd, request);
        close(client_fd);
        return;
    }
    else
    {
        write(client_fd, errorpage, sizeof(errorpage) - 1);
        close(client_fd);
        return;
    }
}

//Handle html files
void html_handler(int client_fd, char *request)
{
    char *buffer;
    char *full_path = (char *)malloc((strlen(PATH) + strlen(request)) * sizeof(char));
    FILE *fp;

    char *test = malloc(sizeof(char));
    strcpy(full_path, PATH); // Merge the file name that requested and path of the root folder
    strcat(full_path, request);

    fp = fopen(full_path, "r");
    if (fp != NULL) //FILE FOUND
    {
        printf("File Found.\n");

        fseek(fp, 0, SEEK_END); // Find the file size.
        long bytes_read = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        send(client_fd, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n", 44, 0); // Send the header for succesful respond.
        buffer = (char *)malloc(bytes_read * sizeof(char));

        fread(buffer, bytes_read, 1, fp); // Read the html file to buffer.
        write(client_fd, buffer, bytes_read);

        fclose(fp);
    }
    else // If there is not such a file.
    {
        write(client_fd, errorpage, sizeof(errorpage) - 1);
    }

    //free(full_path);
    //free(buffer);
}
