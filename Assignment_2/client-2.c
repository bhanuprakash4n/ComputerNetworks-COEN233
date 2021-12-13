/*
 
Programming Assignment 2: Client using customized protocol on top of UDP protocol for rquesting identification from server for access permission to the cellular network.
One client connects to one server.
 Name: Bhanu Prakash Naredla
 Campus ID: W1630571
 
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#define START_OF_PACKET_ID 0xFFFF
#define END_OF_PACKET_ID 0xFFFF

#define DATA_PACKET_TYPE 0xFFF1
#define ACK_PACKET_TYPE 0xFFF2
#define REJECT_PACKET_TYPE 0xFFF3

#define OUT_OF_SEQUENCE 0xFFF4
#define PAYLOAD_LENGTH_MISMATCH 0xFFF5
#define END_OF_PACKET_MISSING 0xFFF6
#define DUPLICATE_PACKET 0xFFF7

#define ACCESS_PERMISSION 0xFFF8

#define TECHNOLOGY_2G 02
#define TECHNOLOGY_3G 03
#define TECHNOLOGY_4G 04
#define TECHNOLOGY_5G 05

#define NOT_PAID 0xFFF9
#define NOT_EXIST 0xFFFA
#define ACCESS_OK 0xFFFB

#define INPUT_FILE_NAME "input.txt"

#define PORT 8090

#define TIMEOUT 3

struct AccessPermissionRequest
{
    uint16_t startOfPacketId;
	uint8_t clientID;
	uint16_t accessPermission;
	uint8_t segmentNumber;
	uint8_t length;
	uint8_t technology;
	unsigned int sourceSubscriberNumber;
	uint16_t endOfPacketId;
};
typedef struct AccessPermissionRequest AccessPermissionRequest;

struct AccessPermissionResponse
{
    uint16_t startOfPacketId;
	uint8_t clientID;
	uint16_t type;
	uint8_t segmentNumber;
	uint8_t length;
	uint8_t technology;
	unsigned int sourceSubscriberNumber;
	uint16_t endOfPacketId;
};
typedef struct AccessPermissionResponse AccessPermissionResponse;

void printPacket(AccessPermissionRequest accessRequestPacket)
{
	printf("Start of RequestPacket ID: 0x%X\n",accessRequestPacket.startOfPacketId);
	printf("Client ID: 0x%X (%d Decimal)\n",accessRequestPacket.clientID);
	printf("Access Permission: 0x%X\n",accessRequestPacket.accessPermission);
	printf("Segment Number : %d \n",accessRequestPacket.segmentNumber);
	printf("Length of the Packet: %d\n",accessRequestPacket.length);
	printf("Technology: %d \n", accessRequestPacket.technology);
	printf("Subscriber Number: %u \n",accessRequestPacket.sourceSubscriberNumber);
	printf("End of RequestPacket ID: 0x%X\n",accessRequestPacket.endOfPacketId);
}

AccessPermissionRequest createRequestPacket()
{
	AccessPermissionRequest requestPacket;
	requestPacket.startOfPacketId = 0XFFFF;
	requestPacket.clientID = 0XFF;
	requestPacket.accessPermission = 0XFFF8;
	requestPacket.endOfPacketId = 0XFFFF;
	return requestPacket;

}

void printAuthenticationMsg(int errorCode)
{
    switch(errorCode)
    {
        case 0xFFF9 : printf("ERROR: Subscriber has not paid for the service\n");
                      break;
        case 0xFFFA : printf("ERROR: Subscriber not found in database (or) Unknown Subscriber's Technology\n");
                      break;
        case 0xFFFB : printf("ACCESS_OK: Subscriber permitted to access the network\n");
    }
}

int main()
{
    int socketFd, portNumber;
    struct sockaddr_in serverAddress;
    socklen_t serverAddrLen = sizeof(serverAddress);
    socketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd < 0)
    {
        printf("ERROR opening socket\n");
        return -1;
    }
    bzero((char *) &serverAddress, sizeof(serverAddress));

    portNumber = PORT;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(portNumber);

    struct timeval timeoutValue;
    timeoutValue.tv_sec = TIMEOUT;
    timeoutValue.tv_usec = 0;

    setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeoutValue, sizeof(struct timeval));
    
    int packet = 1;
    char line[30];
    FILE *fp = fopen(INPUT_FILE_NAME, "rt");
    if( fp == NULL)
    {
        printf("ERROR: Cannot open Input file\n");
        exit(0);
    }
    AccessPermissionRequest requestPacket;
    AccessPermissionResponse responsePacket;

    while(fgets(line, sizeof(line), fp) != NULL)
    {
        requestPacket = createRequestPacket();
        char *words;
        words = strtok(line," ");
		requestPacket.length = strlen(words);
		requestPacket.sourceSubscriberNumber = (unsigned) atoi(words);
		words = strtok(NULL," ");
		requestPacket.length += strlen(words);
		requestPacket.technology = atoi(words);
		words = strtok(NULL," ");
		requestPacket.segmentNumber = packet;

        printf("\n==========NEW REQUEST==========\n");
        printPacket(requestPacket);

        int n = 0, count = 0;
        while(n<=0 && count < 3)
        {
            sendto(socketFd, &requestPacket, sizeof(requestPacket), 0, (struct sockaddr *)&serverAddress, serverAddrLen);
			n = recvfrom(socketFd, &responsePacket, sizeof(responsePacket), 0, NULL, NULL);
            if (n <= 0)
			{
				printf("No Response received from server, rerying...\n");
				count++;
			}
            else
            {
                printAuthenticationMsg(responsePacket.type);
            }
        }
        if( count >= 3)
        {
            printf("ERROR: Server does not respond.\n");
            return -1;
        }
        packet++;
        printf("\n===================================================\n");
    }
    fclose(fp);
    close(socketFd);
    return 0;
}