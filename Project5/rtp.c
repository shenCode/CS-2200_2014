#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "rtp.h"

/* GIVEN Function:
 * Handles creating the client's socket and determining the correct
 * information to communicate to the remote server
 */
CONN_INFO* setup_socket(char* ip, char* port){
	struct addrinfo *connections, *conn = NULL;
	struct addrinfo info;
	memset(&info, 0, sizeof(struct addrinfo));
	int sock = 0;

	info.ai_family = AF_INET;
	info.ai_socktype = SOCK_DGRAM;
	info.ai_protocol = IPPROTO_UDP;
	getaddrinfo(ip, port, &info, &connections);

	/*for loop to determine corr addr info*/
	for(conn = connections; conn != NULL; conn = conn->ai_next){
		sock = socket(conn->ai_family, conn->ai_socktype, conn->ai_protocol);
		if(sock <0){
			if(DEBUG)
				perror("Failed to create socket\n");
			continue;
		}
		if(DEBUG)
			printf("Created a socket to use.\n");
		break;
	}
	if(conn == NULL){
		perror("Failed to find and bind a socket\n");
		return NULL;
	}
	CONN_INFO* conn_info = malloc(sizeof(CONN_INFO));
	conn_info->socket = sock;
	conn_info->remote_addr = conn->ai_addr;
	conn_info->addrlen = conn->ai_addrlen;
	return conn_info;
}

void shutdown_socket(CONN_INFO *connection){
	if(connection)
		close(connection->socket);
}

/* 
 * ===========================================================================
 *
 *			STUDENT CODE STARTS HERE. PLEASE COMPLETE ALL FIXMES
 *
 * ===========================================================================
 */


/*
 *  Returns a number computed based on the data in the buffer.
 */
static int checksum(char *buffer, int length){

	 int total = 0;
	 
	 for (int i = 0; i < length; i++) {
		total += buffer[i];
	 }
	 return total;
}

/*
 *  Converts the given buffer into an array of PACKETs and returns
 *  the array.  The value of (*count) should be updated so that it 
 *  contains the length of the array created.
 */
static PACKET* packetize(char *buffer, int length, int *count){

	 int numOfPackets = 0;
	 
	 if (length % MAX_PAYLOAD_LENGTH == 0) {
		numOfPackets = length / MAX_PAYLOAD_LENGTH;
	 } else {
		numOfPackets = length / MAX_PAYLOAD_LENGTH + 1;
	 }
	 
	 PACKET *packets = calloc(numOfPackets, sizeof(PACKET));

	 for (int i = 0; i < numOfPackets; i++) {
		if (i != numOfPackets-1) {
			packets[i].type = DATA;
			packets[i].payload_length = MAX_PAYLOAD_LENGTH;
		} else {
			packets[i].type = LAST_DATA;
			packets[i].payload_length = length % MAX_PAYLOAD_LENGTH;
		}
		for (int j = 0; j < packets[i].payload_length; j++) {
			packets[i].payload[j] = buffer[MAX_PAYLOAD_LENGTH * i + j];
		}
		packets[i].checksum = checksum(packets[i].payload, packets[i].payload_length);
	}
	
	 *count = numOfPackets;
	 return packets;
}

/*
 * Send a message via RTP using the connection information
 * given on UDP socket functions sendto() and recvfrom()
 */
int rtp_send_message(CONN_INFO *connection, MESSAGE*msg){
	 int *count = malloc(sizeof(int));
	 PACKET *packets = packetize(msg->buffer, msg->length, count);
	 PACKET *response = malloc(sizeof(PACKET));
	 for (int i = 0; i < *count; i++) {
		sendto(connection->socket, (void *)&packets[i], sizeof(PACKET), 0, connection->remote_addr, connection->addrlen);
		recvfrom(connection->socket, (void *)response, sizeof(PACKET), 0, connection->remote_addr, &connection->addrlen);
		if (response->type == NACK) {
			i--;
		}
	 }
	 return 1;
}

/*
 * Receive a message via RTP using the connection information
 * given on UDP socket functions sendto() and recvfrom()
 */
MESSAGE* rtp_receive_message(CONN_INFO *connection){
	int size = 10;
	int length = 0;
	MESSAGE* message = malloc(sizeof(MESSAGE));
	char* buffer = malloc(size*sizeof(char));
	PACKET* packet = malloc(sizeof(PACKET));
	PACKET* response = malloc(sizeof(PACKET)); // response back to server
	while(1) {
		/* recvfrom(int s, void *buf, size_t len, int flags,
                 struct sockaddr *from, socklen_t *fromlen); */
		recvfrom(connection->socket, packet, sizeof(PACKET), 0, connection->remote_addr, &connection->addrlen);
		if (packet->checksum == checksum(packet->payload, packet->payload_length)) {
			response->type = ACK;
			length += packet->payload_length;
			if (length+packet->payload_length>size) {
				size = size * 2;
				char* tmp = buffer;
				buffer = malloc(size*sizeof(char));
				strcpy(buffer, tmp);
			}
			strcat(buffer, packet->payload);
			if (packet->type == LAST_DATA) {
				sendto(connection->socket, (void *)response, sizeof(PACKET), 0, connection->remote_addr, connection->addrlen);
				break;
			}
		} else {
			response->type = NACK;
		}
		sendto(connection->socket, response, sizeof(PACKET), 0, connection->remote_addr, connection->addrlen);
	}
	message->buffer = buffer;
	message->length = length+1;
	return message;
}

