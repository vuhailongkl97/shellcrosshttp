/*
MIT License

Copyright (c) 2022 Vu Hai Long

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

* */
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int
#define GETSOCKETERRNO() (errno)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *get_content_type(const char *path) {
    const char *last_dot = strrchr(path, '.');
    if (last_dot) {
        if (strcmp(last_dot, ".css") == 0)
            return "text/css";
        if (strcmp(last_dot, ".csv") == 0)
            return "text/csv";
        if (strcmp(last_dot, ".gif") == 0)
            return "image/gif";
        if (strcmp(last_dot, ".htm") == 0)
            return "text/html";
        if (strcmp(last_dot, ".html") == 0)
            return "text/html";
        if (strcmp(last_dot, ".ico") == 0)
            return "image/x-icon";
        if (strcmp(last_dot, ".jpeg") == 0)
            return "image/jpeg";
        if (strcmp(last_dot, ".jpg") == 0)
            return "image/jpeg";
        if (strcmp(last_dot, ".js") == 0)
            return "application/javascript";
        if (strcmp(last_dot, ".json") == 0)
            return "application/json";
        if (strcmp(last_dot, ".png") == 0)
            return "image/png";
        if (strcmp(last_dot, ".pdf") == 0)
            return "application/pdf";
        if (strcmp(last_dot, ".svg") == 0)
            return "image/svg+xml";
        if (strcmp(last_dot, ".txt") == 0)
            return "text/plain";
    }

    return "application/octet-stream";
}

SOCKET create_socket(const char *host, const char *port) {
    printf("Configuring local address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *bind_address;
    getaddrinfo(host, port, &hints, &bind_address);

    printf("Creating socket...\n");
    SOCKET socket_listen;
    socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype,
                           bind_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_listen)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        exit(1);
    }
    int reuse = 1;
    if (setsockopt(socket_listen, SOL_SOCKET, SO_REUSEADDR,
                   (const char *)&reuse, sizeof(reuse)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    printf("Binding socket to local address...\n");
    if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) {
        fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
        exit(1);
    }
    freeaddrinfo(bind_address);

    printf("Listening...\n");
    if (listen(socket_listen, 10) < 0) {
        fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
        exit(1);
    }

    return socket_listen;
}

// 50k
#define MAX_REQUEST_SIZE (1024 * 50)

struct client_info {
    socklen_t address_length;
    struct sockaddr_storage address;
    SOCKET socket;
    char request[MAX_REQUEST_SIZE + 1];
    int received;
    struct client_info *next;
};

static struct client_info *clients = 0;

struct client_info *get_client(SOCKET s) {
    struct client_info *ci = clients;

    while (ci) {
        if (ci->socket == s)
            break;
        ci = ci->next;
    }

    if (ci)
        return ci;
    struct client_info *n =
        (struct client_info *)calloc(1, sizeof(struct client_info));

    if (!n) {
        fprintf(stderr, "Out of memory.\n");
        exit(1);
    }

    n->address_length = sizeof(n->address);
    n->next = clients;
    clients = n;
    return n;
}

void drop_client(struct client_info *client) {
    CLOSESOCKET(client->socket);

    struct client_info **p = &clients;

    while (*p) {
        if (*p == client) {
            *p = client->next;
            free(client);
            return;
        }
        p = &(*p)->next;
    }

    fprintf(stderr, "drop_client not found.\n");
    exit(1);
}

const char *get_client_address(struct client_info *ci) {
    static char address_buffer[100];
    getnameinfo((struct sockaddr *)&ci->address, ci->address_length,
                address_buffer, sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
    return address_buffer;
}

fd_set wait_on_clients(SOCKET server) {
    fd_set reads;
    FD_ZERO(&reads);
    FD_SET(server, &reads);
    SOCKET max_socket = server;

    struct client_info *ci = clients;

    while (ci) {
        FD_SET(ci->socket, &reads);
        if (ci->socket > max_socket)
            max_socket = ci->socket;
        ci = ci->next;
    }

    if (select(max_socket + 1, &reads, 0, 0, 0) < 0) {
        fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
        exit(1);
    }

    return reads;
}

void send_400(struct client_info *client) {
    const char *c400 = "HTTP/1.1 400 Bad Request\r\n"
                       "Connection: close\r\n"
                       "Content-Length: 11\r\n\r\nBad Request";
    send(client->socket, c400, strlen(c400), 0);
    drop_client(client);
}

void send_404(struct client_info *client) {
    const char *c404 = "HTTP/1.1 404 Not Found\r\n"
                       "Connection: close\r\n"
                       "Content-Length: 9\r\n\r\nNot Found";
    send(client->socket, c404, strlen(c404), 0);
    drop_client(client);
}
void write_response(struct client_info *client, FILE *fp) {

    const char *ct = get_content_type("/.html");

#define BSIZE 1024
    char buffer[BSIZE];

    sprintf(buffer, "HTTP/1.1 200 OK\r\n");
    send(client->socket, buffer, strlen(buffer), 0);

    sprintf(buffer, "Connection: close\r\n");
    send(client->socket, buffer, strlen(buffer), 0);

    // sprintf(buffer, "Content-Length: %lu\r\n", 10);
    // send(client->socket, buffer, strlen(buffer), 0);

    sprintf(buffer, "Content-Type: %s\r\n", ct);
    send(client->socket, buffer, strlen(buffer), 0);

    sprintf(buffer, "\r\n");
    send(client->socket, buffer, strlen(buffer), 0);

    FILE *fp_form = fopen("form.html", "r");
    if (!fp_form) {
        perror("can't open form.html\n");
    } else {
        while (fgets(buffer, BSIZE, fp_form)) {
            send(client->socket, buffer, strlen(buffer), 0);
        }
    }

	const char *txtarea = "<textarea disabled=\"true\" style=\"border: none;background-color:white;width:100%;height:100%;\">";
    send(client->socket, txtarea, strlen(txtarea), 0); 
    while (fgets(buffer, BSIZE, fp)) {
            }
        send(client->socket, buffer, strlen(buffer), 0);
        puts(buffer);
    }

    const char *endOfHtml = "</code></pre></body> </html>";
    const char *endOfHtml = "</textarea>";
    send(client->socket, endOfHtml, strlen(endOfHtml), 0);

    pclose(fp);
    drop_client(client);
}

void decodeHTML(const char *cmd, char *output) {
    size_t ip = 0;
    for (int i = 0; i < strlen(cmd); i++) {
        char c = cmd[i];
        if (cmd[i] == '+')
            c = ' ';

        if (i + 2 < strlen(cmd) && cmd[i] == '%') {
            switch (cmd[i + 1]) {
            case '2':
                switch (cmd[i + 2]) {
                case 'F':
                    c = '/';
                    break;
                default:
                    break;
                }
                break;
            case '7':
                switch (cmd[i + 2]) {
                case 'E':
                    c = '~';
                    break;
                default:
                    break;
                }
                break;
            default:
                printf("this char haven't support yet [%% %c %c]\n", cmd[i + 1],
                       cmd[i + 2]);

                break;
            }
            i += 2;
        }

        output[ip] = c;
        ip++;
    }
}
void serve_command(struct client_info *client, const char *cmd) {
    printf("serve_command %s %s\n", get_client_address(client), cmd);
    printf("len of command is %lu\n", strlen(cmd));

#define MAX_CMD_BUF 200
    char p[MAX_CMD_BUF];
    memset(p, 0, MAX_CMD_BUF);

    decodeHTML(cmd, p);
    puts(p);

    FILE *fp = popen(p, "r");

    if (!fp) {
        perror("popen error\n");
        send_400(client);
        return;
    }

    if (strlen(cmd) > MAX_CMD_BUF / 2) {
        send_400(client);
        return;
    }

    write_response(client, fp);
}

void fake200(struct client_info *client) {
#define BSIZE 1024
    char buffer[BSIZE];

    const char *ct = get_content_type("/.html");
    sprintf(buffer, "HTTP/1.1 200 OK\r\n");
    send(client->socket, buffer, strlen(buffer), 0);

    sprintf(buffer, "Connection: close\r\n");
    send(client->socket, buffer, strlen(buffer), 0);

    send(client->socket, buffer, strlen(buffer), 0);

    sprintf(buffer, "Content-Type: %s; charset=utf-8\r\n", ct);
    send(client->socket, buffer, strlen(buffer), 0);

    sprintf(buffer, "\r\n");
    send(client->socket, buffer, strlen(buffer), 0);

    drop_client(client);
}

void handleGET(struct client_info *client) {
    char *q = strstr(client->request, "\r\n\r\n");
    if (q) {
        *q = 0;
        if (!strncmp("GET /favicon.ico", client->request, 14)) {
            fake200(client);
        } else if (!strncmp("GET /", client->request, 5)) {
            char *path = client->request + 6 + strlen("command=");
            char *end_path = strstr(path, " ");
            if (!end_path) {
                send_400(client);
            } else {
                *end_path = 0;
                serve_command(client, path);
            }
        } else {
            send_400(client);
        }
    }
}
void writeToFile(const char *filePath, const char *buf) {
    char tmpPath[100];
    snprintf(tmpPath, sizeof(tmpPath), "/tmp/%s", filePath);
    printf("write to file %s\n", tmpPath);

    FILE *fp = fopen(tmpPath, "w");
    if (fp) {
        // fwrite(buf, strlen(buf), 1, fp);
        fprintf(fp, "%s", buf);
    } else {
        printf("can't open file %s\n", tmpPath);
    }
    fclose(fp);
}
void handlePOST(struct client_info *client) {

    char *p = strstr(client->request, "------WebKitFormBoundary");
    if (p) {
        if (p) {
            p = strstr(client->request, "------WebKitFormBoundary");
            if (p) {

                p = strstr(p, "filename=");
                char fileName[50] = {0};
                memset(fileName, 0, sizeof(fileName));
                int i = 0;
                if (p) {
                    p += strlen("filename=\"");
                    while (p && *p && *p != '"') {
                        fileName[i] = *p;
                        i++;
                        p++;
                    }
                    printf("file name is %s\n", fileName);
                }
                p = strstr(p, "Content-Type:");
                // skip until met a newline character
                while (*p != '\n')
                    p++;
                if (p) {
                    char *head = p;
                    char *endReq = head + strlen(head) - 2;
                    while (endReq >= head && *endReq != '\n')
                        endReq--;
                    if (endReq >= head)
                        *endReq = '\0';

                    writeToFile(fileName, head);
                }
            }
        }
        fake200(client);
    }
}

int main() {

    signal(SIGPIPE, SIG_IGN);
    SOCKET server = create_socket(0, "1997");

    while (1) {

        fd_set reads;
        reads = wait_on_clients(server);

        if (FD_ISSET(server, &reads)) {
            struct client_info *client = get_client(-1);

            client->socket =
                accept(server, (struct sockaddr *)&(client->address),
                       &(client->address_length));

            if (!ISVALIDSOCKET(client->socket)) {
                fprintf(stderr, "accept() failed. (%d)\n", GETSOCKETERRNO());
                return 1;
            }

            printf("New connection from %s.\n", get_client_address(client));
        }

        struct client_info *client = clients;
        while (client) {
            struct client_info *next = client->next;

            if (FD_ISSET(client->socket, &reads)) {

                if (MAX_REQUEST_SIZE == client->received) {
                    send_400(client);
                    client = next;
                    continue;
                }

                int r = recv(client->socket, client->request + client->received,
                             MAX_REQUEST_SIZE - client->received, 0);

                if (r < 1) {
                    printf("Unexpected disconnect from %s.\n",
                           get_client_address(client));
                    drop_client(client);

                } else {
                    client->received += r;
                    client->request[client->received] = 0;

                    printf("----------------\n\n\n");
                    if (!strncmp("POST /", client->request, 5)) {

                        printf("post\n\n");
                        handlePOST(client);
                    } else {
                        handleGET(client);
                    }
                }
            }

            client = next;
        }

    } // while(1)

    printf("\nClosing socket...\n");
    CLOSESOCKET(server);

    printf("Finished.\n");
    return 0;
}
