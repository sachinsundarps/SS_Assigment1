Name		: Sachin Sundar Pungampalayam Shanmugasundaram
ASU ID		: 1213230978

normal_web_server:

Execution steps:
1. Execute 'make' command to create the executable 'normal_web_server'.
2. Run './normal_web_server <port>' specifying a desired for the server to run.

Implementation:

For the given port number start a server. To start the server, get a socket specifying all the server address details. Once a socket is obtained, bind that socket with the server address and start listening in that socket for client requests.

Once server is started and listening, start accepting the client requests using 'accetp()' call and get the connection socket for each client and create a new process to service that client request. Using the client connection socket descriptor, receive the request message from client and check if it is a GET request. If not, return a '404 Not Found' HTTP response as our server backdoor should not respond to nothing but GET request.

If it is a GET request, check if it starts with '/exec/' so that we can be sure that we have received something to process. Take the contents after '/exec/' and decode that into a ASCII format. Once decoded, execute that using 'popen()' and get the output of that. Send the '200 OK' HTTP response and write output of the command to the client. Once sending is done, shutdown and close the socket.
