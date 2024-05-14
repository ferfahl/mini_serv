#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>

// estrutura do cliente, guardando o id do cliente e o fd de onde ele está conectando
typedef struct client 
{
    int fd;
    int id;
} client;

// mensagem de erro no strerr, saindo com codigo 1
void exit_error(char *str) 
{
    write(2, str, strlen(str));
    exit(1);
}

int main(int argc, char **argv)
{
    if (argc != 2) //verificando qtd de argumentos
    {
        exit_error("Wrong number of arguments\n");
    }

    const int MAX_CLIENTS = 128; // criando uma espécie de define para máximo de clientes permitidos no servidor
    const int BUFFER_SIZE = 200000; // criando uma espécie de define para buffer maximo do programa
    client clients[MAX_CLIENTS]; //usando a estrutura do cliente, define o máximo de clientes do que foi definido como máximo
    int next_id = 0; // inicializa o contador de clientes
    fd_set active_sockets, ready_sockets; // The fd_set data type represents file descriptor sets for the select function.
	// três buffers diferentes setados com o "define" do buffersize
    char buffer[BUFFER_SIZE];
    char msg_buffer[BUFFER_SIZE];
    char sub_buffer[BUFFER_SIZE];
    int server_socket; // variável int para ter as informações relativas ao socket

	// verifica se o socket novo é menor do que 0, se não, retorna o valor do novo socket do servidor
    /* socket -> int  socket(int domain, int type, int protocol);
       socket() creates an endpoint for  communication  and returns a file descriptor that
       refers to that  endpoint.   The  file  descriptor  returned  by  a  successful call
       will be the lowest-numbered file  descriptor not currently open for the process.
       
       AF_INET      IPv4 Internet protocols
       SOCK_STREAM     Provides sequenced,  reliable, two-way, connection-based  byte  streams. An out-of-band data transmission mechanism may be supported.*/
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
        exit_error("Fatal error\n");
    }

	// A estrutura SOCKADDR_IN especifica um endereço de transporte e uma porta para a família de endereços AF_INET.
    /*struct sockaddr_in {
    short            sin_family;   // Família do endereço (geralmente AF_INET)
    unsigned short   sin_port;     // Número de porta
    struct in_addr   sin_addr;     // Endereço IPv4
    char             sin_zero[8];  // Preenchimento para igualar o tamanho de sockaddr
    };*/
    struct sockaddr_in server_address = {0};
    server_address.sin_family = AF_INET;
    /*htonl,  htons, ntohl, ntohs - convert values between host and network byte order
    uint32_t htonl(uint32_t hostlong);
    INADDR_LOOPBACK é uma constante definida no header <netinet/in.h> em sistemas Unix-like.
    Ela representa o endereço IPv4 de loopback, que é usado para se referir ao próprio dispositivo.
    Em outras palavras, quando você se conecta a 127.0.0.1, está se conectando ao seu próprio computador.*/
    server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    // binds the port passed as arg
    server_address.sin_port = htons(atoi(argv[1]));

    /*bind - bind a name to a socket
    int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);*/
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) 
    {
        exit_error("Fatal error\n");
    }

    /*listen  -  listen  for  connections  on  a socket
    int listen(int sockfd, int backlog);
    The sockfd argument is a file descriptor that refers to a socket of type
    SOCK_STREAM or SOCK_SEQPACKET.

    The  backlog  argument defines the maximum length to which the queue of pending
    connections  for  sockfd may grow.  If a connection request arrives when the queue 
    is full, the client may receive an error with an indication of ECONNREFUSED or,
    if the underlying protocol supports retransmission, the request may be ignored so that a
    later reattempt at connection succeeds.*/
    if (listen(server_socket , MAX_CLIENTS) < 0) 
    {
        exit_error("Fatal error\n");
    }

    /*void bzero(void *s, size_t n);*/
    bzero(clients, sizeof(client) * MAX_CLIENTS);
    //void FD_ZERO(fd_set *set);
    /*This macro clears (removes all file descriptors from) set. It should be  employed 
    as  the first step in initializing a file descriptor set.*/
    FD_ZERO(&active_sockets);
    /*void FD_SET(int fd, fd_set *set);
    This macro adds the file descriptor fd  to set.  Adding a file descriptor that is already present in  the
    set  is  a no-op, and does not produce an error.*/
    FD_SET(server_socket, &active_sockets);
    // info from server_socket to max_socket
    int max_socket = server_socket;

    //loop infinito até saída
    while (42) {
        ready_sockets = active_sockets;
        /*int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
        */
        if (select(max_socket + 1, &ready_sockets, NULL, NULL, NULL) < 0) 
        {
            exit_error("Fatal error\n");
        }
        for (int socket_id = 0; socket_id <= max_socket; socket_id++) 
        {
            if (!FD_ISSET(socket_id, &ready_sockets)) 
            {
                continue ;
            }
            bzero(buffer, BUFFER_SIZE);
            if (socket_id == server_socket) 
            {
                int client_socket;

                if ((client_socket = accept(server_socket, NULL, NULL)) < 0) 
                {
                    exit_error("Fatal error\n");
                }
                FD_SET(client_socket, &active_sockets);
                max_socket = (client_socket > max_socket) ? client_socket : max_socket;
                clients[client_socket].fd = client_socket;
                clients[client_socket].id = next_id++;
                sprintf(buffer, "server: client %d just arrived\n", clients[client_socket].id);
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].fd != 0 && clients[i].fd != client_socket) 
                    {
                        send(clients[i].fd, buffer, strlen(buffer), 0);
                    }
                }
            } else {
                int bytes_read = recv(socket_id, buffer, sizeof(buffer) - 1, 0);

                if (bytes_read <= 0) 
                {
                    bzero(msg_buffer, BUFFER_SIZE);
                    sprintf(msg_buffer, "server: client %d just left\n", clients[socket_id].id);
                    for (int i = 0; i < MAX_CLIENTS; i++) 
                    {
                        if (clients[i].fd != socket_id && clients[i].fd != 0) 
                        {
                            send(clients[i].fd, msg_buffer, strlen(msg_buffer), 0);
                        }
                    }
                    close(socket_id);
                    FD_CLR(socket_id, &active_sockets);
                } else {
                    for (int i = 0, j = strlen(sub_buffer); i < bytes_read; i++, j++) {
                        sub_buffer[j] = buffer[i];
                        if (sub_buffer[j] == '\n') {
                            sub_buffer[j] = '\0';
                            bzero(msg_buffer, BUFFER_SIZE);
                            sprintf(msg_buffer, "client %d: %s\n", clients[socket_id].id, sub_buffer);
                            for (int i = 0; i < MAX_CLIENTS; i++) 
                            {
                                if (clients[i].fd != socket_id && clients[i].fd != 0) 
                                {
                                    send(clients[i].fd, msg_buffer, strlen(msg_buffer), 0);
                                }
                            }
                            bzero(sub_buffer, strlen(sub_buffer));
                            j = -1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}