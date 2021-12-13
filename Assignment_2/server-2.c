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

#define VERIFICATION_FILE_NAME "Verification_Database.txt"
#define MAX_DATABASE_SIZE 100

#define PORT 8090

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

struct SubScriber
{
    unsigned long subscriberNumber;
	uint8_t technology;
	int status;
};
typedef struct SubScriber SubScriber;

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

AccessPermissionResponse createResponsePacket(AccessPermissionRequest accessRequestPacket)
{
	AccessPermissionResponse responsePacket;
	responsePacket.startOfPacketId = accessRequestPacket.startOfPacketId;
	responsePacket.clientID = accessRequestPacket.clientID;
	responsePacket.segmentNumber = accessRequestPacket.segmentNumber;
	responsePacket.length = accessRequestPacket.length;
	responsePacket.technology = accessRequestPacket.technology;
	responsePacket.sourceSubscriberNumber = accessRequestPacket.sourceSubscriberNumber;
	responsePacket.endOfPacketId = accessRequestPacket.endOfPacketId;
	return responsePacket;
}

void getSubscribersList(SubScriber subscriberDatabase[], int* pSubscriberDataBaseSize)
{
	char line[30];
	int i = 0;

	FILE *fp = fopen(VERIFICATION_FILE_NAME, "rt");

	if(fp == NULL)
	{
		printf("Cannot Open Verification Database File!\n");
		return;
	}

	while(fgets(line, sizeof(line), fp) != NULL)
	{
		char * words=NULL;
		words = strtok(line," ");
		subscriberDatabase[i].subscriberNumber =(unsigned) atol(words);
		words = strtok(NULL," ");
		subscriberDatabase[i].technology = (uint8_t)(atoi(words));
		words = strtok(NULL," ");
		subscriberDatabase[i].status = atoi(words);
		i++;
	}
    *pSubscriberDataBaseSize = i;
	fclose(fp);
}

void printAuthenticationMsg(int errorCode)
{
    switch(errorCode)
    {
        case 0xFFF9 : printf("ERROR: Subscriber has not paid for the service\n");
                      break;
        case 0xFFFA : printf("ERROR: Subscriber not found in database\n");
                      break;
        case 0xFFFB : printf("ACCESS_OK: Subscriber permitted to access the network\n");
                      break;
        default : printf("ERROR: Unknown Subscriber's Technology \n");
    }
}

int validateSubscriber(SubScriber subscriberDatabase[], int subscriberDataBaseSize,unsigned long subscriberNumber,uint8_t technology)
{
    int j;
	for(j = 0; j < subscriberDataBaseSize; j++)
    {
		if(subscriberDatabase[j].subscriberNumber == subscriberNumber && subscriberDatabase[j].technology == technology)
        {
			if(subscriberDatabase[j].status == 1)
            {
                return ACCESS_OK;
            }
            else
            {
                return NOT_PAID;
            }
		}
        else if (subscriberDatabase[j].subscriberNumber == subscriberNumber && subscriberDatabase[j].technology != technology)
        {
            return -1; // unknown technology
        }
	}
	return NOT_EXIST;
}

int main()
{
    int subscriberDatabaseSize = 0;
    SubScriber subscriberDatabase[MAX_DATABASE_SIZE];
    getSubscribersList(subscriberDatabase, &subscriberDatabaseSize);
    
    int socketFd, portNumber;
    struct sockaddr_in serverAddress;
    socketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd < 0)
    {
        printf("ERROR opening socket\n");
        return -1;
    }
    int reuse = 1;
    if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0)
        error("setsockopt(SO_REUSEADDR) failed");
    if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    bzero((char *) &serverAddress, sizeof(serverAddress));

    portNumber = PORT;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(portNumber);
    if (bind(socketFd, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0)
    {
        printf("ERROR on binding\n");
        return -1;
    }
    else
        printf("Server started...\n");
    
    int n;
    struct sockaddr_in clientAddress;
    socklen_t clientAddrLen = sizeof(clientAddrLen);
    AccessPermissionRequest requestPacket;

    while(1)
    {
        n = recvfrom(socketFd, &requestPacket, sizeof(requestPacket), 0, (struct sockaddr*) &clientAddress, &clientAddrLen);
        printf("----------Received New Packet----------\n");
        printPacket(requestPacket);
        if(n > 0 && requestPacket.accessPermission == ACCESS_PERMISSION)
        {
            int authenticationReturnCode = validateSubscriber(subscriberDatabase, subscriberDatabaseSize, requestPacket.sourceSubscriberNumber, requestPacket.technology);
            printAuthenticationMsg(authenticationReturnCode);
            AccessPermissionResponse responsePacket = createResponsePacket(requestPacket);
            if(authenticationReturnCode != -1)
            {
                responsePacket.type = authenticationReturnCode;
            }
            else
            {
                responsePacket.type = NOT_EXIST;
            }
            sendto(socketFd, &responsePacket, sizeof(responsePacket), 0, (struct sockaddr *)&clientAddress, clientAddrLen);
        }
        else
        {
            printf("ERROR: Unknown PacketType\n");
        }
        printf("\n===================================================\n");
    }
    return 0;
}