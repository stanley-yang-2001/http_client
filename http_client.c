/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "3490" // the port client will be connecting to 

#define MAXDATASIZE 150 // max number of bytes we can get at once 

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// remove substring
void removeSubstring(char *str, const char *sub) {
	char *found;
	while ((found = strstr(str, sub)) != NULL) {
		// Shift characters to remove substring
		int subLen = strlen(sub);
		memmove(found, found+subLen, strlen(found + subLen)+1);
	}
}

void parse_url(const char *url, char *hostname, char *port, char *path
){
	if (strncmp(url, "http://", 7) == 0){
		url += 7;	// skips http
	}
	if (sscanf(url, "%255[^:/]:%9[^/]/%1023[^\n]", hostname, port, path) == 3 ){
		return; // parse hostname, port, and path
	}
	if (sscanf(url, "%255[^:/]:%9[^/]", hostname, port)== 2) {
		strcpy(path, ""); //no path
		return; 
	}
	if (sscanf(url, "%255[^:/]/%1023[^\n]", hostname, path)== 2 ) {
		strcpy(port, "80");
		return;
	}
	if (sscanf(url, "%255s",hostname) == 1){
		strcpy(port, "80");
		strcpy(path, "");
	}
}
int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];
	char hostname[MAXDATASIZE], port[10], path[MAXDATASIZE];

	char check[4];

	FILE *file = fopen("output", "w");
	if (!file) {
		perror("Error opening file");
		exit(1);
	}

	strncpy(check, argv[1], 4);
	int ishttp = strcmp(check, "http");
	if (ishttp != 0){
		printf("INVALIDPROTOCOL\n");
		fwrite("INVALIDPROTOCOL", sizeof(char), 15, file);
		return 1;
	}

	parse_url(argv[1], hostname, port, path);
	/*printf("url: %s\n", argv[1]);
	printf("hostname: %s\n", hostname);
	printf("port: %s\n", port);
	printf("path: %s\n", path[0] ? path : "/ (root)");

	int result1 = strcmp(hostname, "localhost");
	if (result1 == 0){
		printf("localhost verified\n");
	} else {
		printf("localhost error\n");
	}
	int result2 = strcmp(port, "8000");

	if (result2 == 0){
		printf("port verified\n");
	} else {
		printf("port error\n");
	}*/

	if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	printf("result: %s\n ",argv[1]);
	if ((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(rv));
		return 1;
	}
	/*if ((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(rv));
		return 1;
	}*/

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	char request[MAXDATASIZE];
	printf("path: %s\n", path);
	printf("hostname: %s\n", hostname);
	snprintf(request, sizeof(request), "GET /%s HTTP/1.0\r\nHost: %s\r\nAccept: */*\r\nConnection: Keep-Alive\r\n", path, hostname);
	printf("check Get format: %s\n", request);

	// send the request
	if (send(sockfd, request, strlen(request), 0) == -1){
		perror("send");
		close(sockfd);
		return 3;
	}
	printf("client: GET request sent\n");

	/*while ((numbytes = recv(sockfd, buf, MAXDATASIZE -1, 0)) > 0){
		buf[numbytes] = '\0';	// numm terminate the response
		printf("%s", buf);
	}

	if (numbytes == -1) {
		perror("recv");
	}*/
	printf("getting file ready\n");


	char buf2[MAXDATASIZE];
	int numbytes_buf;
	printf("writing to file\n");

	while ((numbytes_buf = recv(sockfd, buf2, MAXDATASIZE-1, 0)) > 0) {
		buf2[numbytes_buf] = '\0';
		fwrite(buf2, sizeof(char), numbytes_buf, file);
	}
	printf("finish writing\n");

	/*if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
	    perror("recv");
	    exit(1);
	}

	buf[numbytes] = '\0';

	printf("client: received '%s'\n",buf);*/
	fclose(file);
	close(sockfd);

	return 0;
}

