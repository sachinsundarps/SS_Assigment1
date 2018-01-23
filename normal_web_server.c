#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>

int sockfd, i;

/*
 * Start the server on the given port number.
 */
int start_server(int port_no) {
	int sockfd;
	struct sockaddr_in serv_addr;

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port_no);

	sockfd = socket(serv_addr.sin_family, SOCK_STREAM, 0);
	if (sockfd == -1) {
		printf("Error: socket()\n");
		exit(1);
	}
	int option = 1;
	setsockopt(sockfd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char*)&option, sizeof(option));

	if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
		printf("Error: bind()\n");
		exit(1);
	}
		
	// Listen to socket for new connections.
	if (listen(sockfd, 100) == -1) {
		printf("Error: listen()\n");
		exit(1);
	}

	return sockfd;
}

/*
 * Process the request message sent to the server and return a HTTP/1.1 response
 * and result.
 */
void process_client_request(int connfd) {

	char full_request[1000];
	memset(full_request, 0, 1000);
	
	int ret = recv(connfd, full_request, 1000, 0);

	if (ret == -1) {
		printf("Error: recv()\n");
	} else if (ret == 0) {
		printf("Error: recv() 0\n");
	}

	char *protocol = strtok(full_request, " ");
	char *cmd;
	char *http_version;
	if (protocol != NULL) {
		cmd = strtok(NULL, " ");
	}
	if (cmd != NULL) {
		http_version = strtok(NULL, "\n");
	}

	// Convert the encoded URL to decoded string command.
	printf("%s\n", cmd);
	char *decoded_cmd = calloc(strlen(cmd), sizeof(char));
	char *tmp = decoded_cmd;
	while (*cmd) {
		if (*cmd == '%') {
			if (*(cmd + 1) && *(cmd + 2)) {
				char hex[3];
				char ascii;
				hex[0] = *(cmd + 1);
				hex[1] = *(cmd + 2);
				hex[2] = '\0';
				sscanf(hex, "%x", (unsigned int *)&ascii);
				*tmp = ascii;
				tmp++;
				cmd += 3;
			}
		} else {
			*tmp++ = *cmd++;
		}
	}
	*tmp = '\0';

	if (strncmp(protocol, "GET\0", 4) == 0 && strncmp(http_version, "HTTP/1.1\0", 9)) {
		if (strncmp(decoded_cmd, "/exec/", 6) == 0) {
			decoded_cmd = decoded_cmd + 6;
			
			
			// Send HTTP/1.1 200 OK response before sending command result.
			send(connfd, "HTTP/1.1 200 OK\n\n", 17, 0);

			// Execute the command.
			FILE *fp = popen(decoded_cmd, "r");
			if (fp == NULL) {
				printf("Error: popen()\n");
			}

			// Send the output as response.
			char response[1000];
			while (fgets(response, 1000, fp) != NULL) {
				write(connfd, response, strlen(response));
			}
			printf("%s", response);
		} else {
			write(connfd, "HTTP/1.1 404 Not Found\r\n", 24);
		}
	} else {
		write(connfd, "HTTP/1.1 404 Not Found\r\n", 24);
	}

	shutdown(connfd, SHUT_RDWR);
	close(connfd);
}

void signal_handler(int signal_no) {
	if (signal_no == SIGINT || signal_no == SIGINT) {
		close(sockfd);
		exit(0);
	}
}

/*
 * TODO: Close socket on signal or abrupt closing of program.
 */
int main(int argc, char *argv[]) {

	signal(SIGINT, signal_handler);
	signal(SIGKILL, signal_handler);

	// Check for proper input.
	if (argc != 2) {
		printf("Usage:./normal_web_server <port>\n");
		return 0;
	}
	int port_no = atoi(argv[1]);
	if (port_no == 0) {
		printf("Invalid port number\n");
		return 0;
	}
	
	// Start the server for the given port.
	sockfd = start_server(port_no);

	struct sockaddr_in client_addr;
	socklen_t client_addr_len;
	i = 0;
	while(1) {
		client_addr_len = sizeof(client_addr);
		// Accept a client request.
		int connfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len);

		if (connfd == -1) {
			printf("Error: accept() %d\n", i++);
		} else {
			// Start a new process to service the client request.
			if (fork() == 0) {
				process_client_request(connfd);
				exit(0);
			}
		}
	}

	return 0;
}
