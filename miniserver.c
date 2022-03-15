#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include "miniserver.h"

#define ISVALIDSOCKET(s) ((s) >= 0)
#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

static int handler(void* config, const char* section, const char* name,
                   const char* value)
{
    configuration* pconfig = (configuration*)config;

    if (MATCH("address", "port"))
    {
        pconfig->port = strdup(value);
    }
    else if (MATCH("prefix", "p1"))
    {
        pconfig->p1 = strdup(value);
    }
    else if (MATCH("prefix", "p2"))
    {
        pconfig->p2 = strdup(value);
    }
    else
    {
        return 0;  /* unknown section/name, error */
    }
    return 1;
}

static int callback(void *NotUsed, int argc, char **argv, char **azColName)
{
    int i;
    for(i = 0; i<argc; i++)
    {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

bool Prefix(const char *pre, const char *str)
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);

    return lenstr < lenpre ? false : memcmp(pre, str, lenpre) == 0;
}

pthread_t thread0;
static int gRunning = 0;
void sort_func();

void myTimer( void *param )
{
    int delay = (intptr_t)param;
    while(gRunning)
    {
        sort_func();
        sleep(delay);
    }
}

void sort_func(void)
{
    sqlite3    *db;
    char       *zErrMsg = 0;

    int rc = sqlite3_open("database.db", &db);
    if( rc )
    {
        fprintf(stderr, "Can't open database in thread: %s\n", sqlite3_errmsg(db));
    }
    else
    {
        //fprintf(stdout, "Opened database successfully in thread\n");
    }

    char *sqlInsert = "SELECT * FROM PREFIX ORDER BY P1 ASC;";
    rc = sqlite3_exec(db, sqlInsert, callback, 0, &zErrMsg);
    if( rc == SQLITE_OK )
    {
        //fprintf(stdout, "Records created successfully in thread\n");

    }
    else
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    sqlite3_close(db);
}
int main()
{
    configuration fconfig;
    fconfig.port = NULL;
    fconfig.p1 = NULL;
    fconfig.p2 = NULL;

    if (ini_parse("fconfig.ini", handler, &fconfig)<0)
    {
        printf("Can not find fconfig file!!!\n");
        return 1;
    }
    printf("Config loader from 'fconfig.ini':\n----------\n port=%s\n p1=%s\n p2=%s\n----------\n",\
           fconfig.port, fconfig.p1, fconfig.p2);

    /* Create database */
    sqlite3    *db;
    char       *sqlCreate;
    char       *zErrMsg = 0;

    int rc = sqlite3_open("database.db", &db);
    if( rc )
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    else
    {
        //fprintf(stdout, "Opened database successfully\n");
    }

    sqlCreate = "CREATE TABLE PREFIX("  \
                /* "ID INT PRIMARY KEY     NOT NULL," \*/
                "P1             INT          NOT NULL," \
                "P2             char(100)    NOT NULL," \
                "TIME           char(100)    NOT NULL);";
    rc = sqlite3_exec(db, sqlCreate, callback, 0, &zErrMsg);

    if( rc != SQLITE_OK )
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else
    {
        //fprintf(stdout, "Table created successfully\n");
    }
    /* End SQL statement */

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *bind_address;
    getaddrinfo(0, fconfig.port, &hints, &bind_address);


    //printf("Creating socket...\n");
    int socket_listen;
    socket_listen = socket(bind_address->ai_family,
                           bind_address->ai_socktype, bind_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_listen))
    {
        fprintf(stderr, "socket() failed. (%d)\n", errno);
        return 1;
    }


    //printf("Binding socket to local address...\n");
    if (bind(socket_listen,
             bind_address->ai_addr, bind_address->ai_addrlen))
    {
        fprintf(stderr, "bind() failed. (%d)\n", errno);
        return 1;
    }
    freeaddrinfo(bind_address);


    //printf("Listening...\n");
    if (listen(socket_listen, 10) < 0)
    {
        fprintf(stderr, "listen() failed. (%d)\n", errno);
        return 1;
    }

    printf("Waiting for connection...\n");
    /* Timer Started */
    int delay = 60; //(s)
    pthread_create(&thread0, NULL, myTimer, delay);

    while(true)
    {
        gRunning = 1;
        struct sockaddr_storage client_address;
        socklen_t client_len = sizeof(client_address);
        int socket_client = accept(socket_listen,
                                   (struct sockaddr*) &client_address, &client_len);
        if (!ISVALIDSOCKET(socket_client))
        {
            fprintf(stderr, "accept() failed. (%d)\n", errno);
            return 1;
        }

        printf("Client is connected... ");
        char address_buffer[100];
        getnameinfo((struct sockaddr*)&client_address,
                    client_len, address_buffer, sizeof(address_buffer), 0, 0,
                    NI_NUMERICHOST);
        printf("%s\n", address_buffer);

        //printf("Reading request...\n");

        char request[1024];
        memset(request, 0, 1023); //prevent crash

        int bytes_received = recv(socket_client, request, 1024, 0);
        //printf("Received %d bytes.\n", bytes_received);
        //printf("%.*s\n", bytes_received, request);

        /* Create SQL statement */
        char *p1 = NULL;
        char *p2 = NULL;
        time_t timer;
        bool flag = false;
        char sqlInsert[100];

        memset(sqlInsert, 0, 99);

        timer = time(NULL);
        struct tm tm = *localtime(&timer);

        gRunning = 0;

        char time_msg[100];
        sprintf(time_msg, "%d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

        if (Prefix(fconfig.p1, request))
        {
            p1 = &(request[strlen(fconfig.p1)]);
            printf("p1 is : %s\n", p1);
            flag = true;
        }
        else if(Prefix(fconfig.p2, request))
        {
            p2 = &(request[strlen(fconfig.p2)]);
            printf("p2 is : %s\n", p2);
            flag = true;
        }

        if (flag) //prevent junk insert
        {
            sprintf(sqlInsert, "INSERT INTO PREFIX (P1,P2,TIME) VALUES ('%s', '%s', '%s');",p1, p2, time_msg );
            rc = sqlite3_exec(db, sqlInsert, callback, 0, &zErrMsg);

            if( rc != SQLITE_OK )
            {
                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                sqlite3_free(zErrMsg);
            }
            else
            {
                //fprintf(stdout, "Records created successfully\n");
            }
            /* End SQL statement */
        }
        gRunning = 1;
        //printf("Sending response...\n");
        const char *response = "Request OK\r\n";
        int bytes_sent = send(socket_client, response, strlen(response), 0);
		

        //printf("Closing connection...\n");
        close(socket_client);
    }
    sqlite3_close(db);
    printf("Closing listening socket...\n");
    close(socket_listen);

    printf("Finished.\n");

    return 0;
}
