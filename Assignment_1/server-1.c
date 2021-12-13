/*
 
 Programming Assignment 1: Client using customized protocol on top of UDP protocol for sending information to the server.
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

#define PORT 8088

struct DataPacket
{
    uint16_t startOfPacketId;
    uint8_t clientId;
    uint16_t data;
    uint8_t segmentNumber;
    uint8_t length;
    char payLoad[255];
    uint16_t endOfPacketId;
};
typedef struct DataPacket DataPacket;

struct AckPacket
{
    uint16_t startOfPacketId;
    uint8_t clientId;
    uint16_t ack;
    uint8_t receivedSegmentNumber;
    uint16_t endOfPacketId;
};
typedef struct AckPacket AckPacket;

struct RejectPacket
{
    uint16_t startOfPacketId;
    uint8_t clientId;
    uint16_t reject;
    uint16_t rejectSubCode;
    uint8_t receivedSegmentNumber;
    uint16_t endOfPacketId;
};
typedef struct RejectPacket RejectPacket;

void printPacket(DataPacket dataPacket) {
	printf("Details of Packet Received from Client:\n");
	printf("Start of Packet ID: 0x%X\n",dataPacket.startOfPacketId);
	printf("Client ID: 0x%X (%d Decimal)\n",dataPacket.clientId, dataPacket.clientId);
	printf("Data: 0x%X\n",dataPacket.data);
	printf("Segment Number: %d\n",dataPacket.segmentNumber);
	printf("Length: 0x%X (%d Decimal)\n",dataPacket.length, dataPacket.length);
	printf("Payload: %s\n",dataPacket.payLoad);
	printf("End of Packet ID: 0x%X\n",dataPacket.endOfPacketId);
}

AckPacket createAckPacket(DataPacket dataPacket)
{
	AckPacket ackPacket;
	ackPacket.startOfPacketId = dataPacket.startOfPacketId;
	ackPacket.clientId = dataPacket.clientId;
	ackPacket.ack = ACK_PACKET_TYPE;
    ackPacket.receivedSegmentNumber = dataPacket.segmentNumber;
	ackPacket.endOfPacketId = dataPacket.endOfPacketId;
	return ackPacket;
}

RejectPacket createRejectPacket(DataPacket dataPacket)
{
	RejectPacket rejectPacket;
	rejectPacket.startOfPacketId = dataPacket.startOfPacketId;
	rejectPacket.clientId = dataPacket.clientId;
    rejectPacket.reject = REJECT_PACKET_TYPE;
    rejectPacket.receivedSegmentNumber = dataPacket.segmentNumber;
	rejectPacket.endOfPacketId = dataPacket.endOfPacketId;
	return rejectPacket;
}

void printRejectCodeErrorMsg(int errorCode)
{
    switch(errorCode)
    {
        case 0xFFF4 : printf("ERROR: Packet Received is OUT OF SEQUENCE\n");
                      break;
        case 0xFFF5 : printf("ERROR: Received Packet Payload LENGTH MISMATCH\n");
                      break;
        case 0xFFF6 : printf("ERROR: END OF PACKET MISSING\n");
                      break;
        case 0xFFF7 : printf("ERROR: DUPLICATE PACKET Received\n");
    }
}

int validatePacket(DataPacket dataPacket, uint8_t expectedPacket)
{
    if(dataPacket.segmentNumber>0 && dataPacket.segmentNumber<expectedPacket)
        return DUPLICATE_PACKET;
    if(dataPacket.segmentNumber != expectedPacket)
        return OUT_OF_SEQUENCE;
    if(dataPacket.length != strlen(dataPacket.payLoad))
        return PAYLOAD_LENGTH_MISMATCH;
    if(dataPacket.endOfPacketId != END_OF_PACKET_ID)
        return END_OF_PACKET_MISSING;
    return 0;
}

int main()
{
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
    uint8_t expectedPacket = 1;
    struct sockaddr_in clientAddress;
    socklen_t clientAddrLen = sizeof(clientAddrLen);
    DataPacket datapacket;

    while(1)
    {
        n = recvfrom(socketFd, &datapacket, sizeof(datapacket), 0, (struct sockaddr*) &clientAddress, &clientAddrLen);
        printf("----------Received New Packet----------\n");
        printPacket(datapacket);
        int rejectCode = validatePacket(datapacket, expectedPacket);
        if(rejectCode != 0)
        {
            RejectPacket rejectPacket = createRejectPacket(datapacket);

            rejectPacket.rejectSubCode = rejectCode;
            sendto(socketFd, &rejectPacket, sizeof(rejectPacket), 0, (struct sockaddr*) &clientAddress, clientAddrLen);
            printRejectCodeErrorMsg(rejectCode);
        }
        else
        {
            AckPacket ackPacket = createAckPacket(datapacket);
            sendto(socketFd, &ackPacket, sizeof(ackPacket), 0, (struct sockaddr*) &clientAddress, clientAddrLen);
            expectedPacket++;
        }
        printf("\n===================================================\n");
    }
    return 0;
}