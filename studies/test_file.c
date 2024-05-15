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
	/*uint16_t htons(uint16_t hostshort);*/
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
		`int nfds`: Este é o parâmetro mais alto entre todos os descritores de arquivo (file descriptors) passados 
		para o `select()`, mais 1. É necessário porque `select()` examina os descritores de arquivo de 0 a `nfds - 1`.
		Este parâmetro é usado principalmente para otimização e não precisa ser exato, apenas um valor maior ou igual
		ao maior descritor de arquivo sendo monitorado.
		`fd_set *readfds`: Este é um ponteiro para um conjunto (`fd_set`) de descritores de arquivo que serão 
		monitorados para leitura. Se um descritor de arquivo neste conjunto estiver pronto para leitura, a 
		função `select()` retornará. Se você não estiver interessado em monitorar descritores de arquivo para 
		leitura, pode passar `NULL`.
		`fd_set *writefds`: Semelhante ao `readfds`, este é um ponteiro para um conjunto de descritores de arquivo
		que serão monitorados para escrita. Se um descritor de arquivo neste conjunto estiver pronto para escrita,
		a função `select()` retornará. Se você não estiver interessado em monitorar descritores de arquivo para escrita,
		pode passar `NULL`.
		`fd_set *exceptfds`: Este é um ponteiro para um conjunto de descritores de arquivo que serão monitorados para
		exceções. Se um descritor de arquivo neste conjunto estiver em um estado excepcional, a função `select()`
		retornará. Se você não estiver interessado em monitorar exceções, pode passar `NULL`.
		`struct timeval *timeout`: Este é um ponteiro para uma estrutura `timeval` que define um limite de tempo
		para o `select()` esperar por atividade nos descritores de arquivo. Se `timeout` for `NULL`, `select()`
		será bloqueado até que ocorra atividade em algum dos descritores de arquivo. Se `timeout` for definido
		como 0 (zero), `select()` retornará imediatamente após verificar os descritores de arquivo uma vez.
		Se `timeout` for definido para um valor positivo, `select()` esperará até que ocorra atividade ou até que o
		tempo limite seja atingido.

		*/
		if (select(max_socket + 1, &ready_sockets, NULL, NULL, NULL) < 0) 
		{
			exit_error("Fatal error\n");
		}

		for (int socket_id = 0; socket_id <= max_socket; socket_id++) 
		{
			/*int  FD_ISSET(int fd, fd_set *set);
			FD_ISSET()
			  select() modifies the  contents  of the sets according to the rules described below.  After  calling 
			  select(), the FD_ISSET() macro can be used to test if a  file  descriptor is  still present in a set.
			  FD_IS‐SET() returns nonzero if the  file descriptor fd  is present in set,
			  and zero if it is not.*/
			if (!FD_ISSET(socket_id, &ready_sockets)) 
			{
				continue ;
			}
			bzero(buffer, BUFFER_SIZE);
			if (socket_id == server_socket) 
			{
				int client_socket;

				/*int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
				return -1 if error
				Claro! A função `accept()` é comumente usada em sistemas Unix para aceitar uma conexão TCP entrante
				em um socket de servidor. Aqui está uma explicação dos parâmetros dessa função:
				1. `int sockfd`: Este é o descritor de arquivo (file descriptor) do socket de servidor que está
				esperando por conexões. O socket de servidor deve estar previamente criado, vinculado a um endereço
				e ouvindo por conexões usando a função `listen()`.
				2. `struct sockaddr *addr`: Este é um ponteiro para uma estrutura `sockaddr` na qual o endereço do
				cliente será armazenado se a conexão for aceita com sucesso. Como `accept()` é usada comumente com
				sockets de domínio AF_INET (IPv4), normalmente você passa um ponteiro para uma estrutura `sockaddr_in`
				que será preenchida com o endereço do cliente.
				3. `socklen_t *addrlen`: Este é um ponteiro para uma variável do tipo `socklen_t` que contém o tamanho
				da estrutura apontada por `addr`. Antes de chamar `accept()`, você deve definir o valor dessa variável
				como o tamanho da estrutura `sockaddr` passada. Após a chamada de `accept()`, o valor de `addrlen` será
				atualizado para refletir o tamanho real do endereço do cliente.
				A função `accept()` retorna um novo descritor de arquivo (file descriptor) que representa a conexão TCP
				estabelecida com o cliente. Esse novo descritor de arquivo pode ser usado para enviar e receber dados
				da conexão com o cliente. Se ocorrer um erro, `accept()` retorna -1 e define `errno` para indicar o
				tipo de erro ocorrido.
				*/
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
			}
			
			else
			{
				int bytes_read = recv(socket_id, buffer, sizeof(buffer) - 1, 0);

				if (bytes_read <= 0) 
				{
					bzero(msg_buffer, BUFFER_SIZE);
					/*int sprintf(char *str, const char *format, ...);
					A função sprintf() é uma função da linguagem de programação C (e também disponível em C++)
					que é usada para formatar e armazenar strings em um buffer. Ela é usada de maneira semelhante
					à função printf(), mas em vez de imprimir a string formatada no console, ela a armazena em um
					array de caracteres (string).*/
					sprintf(msg_buffer, "server: client %d just left\n", clients[socket_id].id);
					for (int i = 0; i < MAX_CLIENTS; i++) 
					{
						if (clients[i].fd != socket_id && clients[i].fd != 0) 
						{
							/*ssize_t send(int sockfd, const void *buf, size_t len, int flags);
							*/
							send(clients[i].fd, msg_buffer, strlen(msg_buffer), 0);
						}
					}
					close(socket_id);
					/*void FD_CLR(int fd, fd_set *set);
					FD_CLR()
					This macro removes the  file  descriptor fd from set.  Removing a file descriptor 
					that is not present in the set is a  no-op, and does not produce an error.*/
					FD_CLR(socket_id, &active_sockets);
				}
				else
				{
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