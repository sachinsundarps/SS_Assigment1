#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>

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

	if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
		printf("Error: bind()\n");
		exit(1);
	}
		
	// Listen to socket for new connections.
	if (listen(sockfd, 100) == -1) {
		printf("Error: listen()\n");
		exit(1);
	}
	printf("***********Server started*********\n");

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

	if (strncmp(protocol, "GET\0", 4) == 0 && strncmp(http_version, "HTTP/1.1\0", 9)) {
		printf("Encoded command: %s\n", cmd);
		if (strncmp(cmd, "/exec/", 6) == 0) {
			cmd = cmd + 6;
			char *decoded_cmd = calloc(strlen(cmd), sizeof(char));
			char *tmp = decoded_cmd;
			
			// Convert the encoded URL to decoded string command.
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
			printf("Decoded command: %s\n", decoded_cmd);
			
			// Send HTTP/1.1 200 OK response before sending command result.
			send(connfd, "HTTP/1.1 200 OK\n", 16, 0);

			// Execute the command.
			FILE *fp = popen(decoded_cmd, "r");
			if (fp == NULL) {
				printf("Error: popen()\n");
			}

			printf("Respose:\n");
			// Send the output as response.
			char response[1000];
			while (fgets(response, 1000, fp) != NULL) {
				printf("%s", response);
				write(connfd, response, 1000);
			}
			printf("Response sent\n\n\n");
		} else {
			write(connfd, "HTTP/1.1 404 Not Found\n", 23);
		}
	} else {
		write(connfd, "HTTP/1.1 404 Not Found\n", 23);
	}

	shutdown(connfd, SHUT_RDWR);
	close(connfd);
}

/*
 * TODO: Close socket on signal or abrupt closing of program.
 */
int main(int argc, char *argv[]) {

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
	printf("Port: %d\n", port_no);
	
	// Start the server for the given port.
	int sockfd = start_server(port_no);

	struct sockaddr_in client_addr;
	socklen_t client_addr_len;
	int i = 0;
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
