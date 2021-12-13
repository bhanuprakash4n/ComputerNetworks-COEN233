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

#define TIMEOUT 3 //sec

struct DataPacket
{
    uint16_t startOfPacketId;
    uint8_t clientId;
    uint16_t type;
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
    uint16_t type;
    uint8_t receivedSegmentNumber;
    uint16_t endOfPacketId;
};
typedef struct AckPacket AckPacket;

struct RejectPacket
{
    uint16_t startOfPacketId;
    uint8_t clientId;
    uint16_t type;
    uint16_t rejectSubCode;
    uint8_t receivedSegmentNumber;
    uint16_t endOfPacketId;
};
typedef struct RejectPacket RejectPacket;

void printPacket(DataPacket dataPacket) {
	printf("Details of Packet:\n");
	printf("Start of Packet ID: 0x%X\n",dataPacket.startOfPacketId);
	printf("Client ID: 0x%X (%d Decimal)\n",dataPacket.clientId, dataPacket.clientId);
	printf("Data: 0x%X\n",dataPacket.type);
	printf("Segment Number: %d\n",dataPacket.segmentNumber);
	printf("Length: 0x%X (%d Decimal)\n",dataPacket.length, dataPacket.length);
	printf("Payload: %s\n",dataPacket.payLoad);
	printf("End of Packet ID: 0x%X\n",dataPacket.endOfPacketId);
}

DataPacket createDataPacket()
{
    DataPacket datapacket;
    datapacket.startOfPacketId = START_OF_PACKET_ID;
    datapacket.clientId = 1;
    datapacket.type = DATA_PACKET_TYPE;
    datapacket.endOfPacketId = END_OF_PACKET_ID;
    return datapacket;
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

char* generatePayload(int packet)
{
    switch(packet)
    {
        case 1: return (char*)"Message One - no error";
        case 2: return (char*)"Message Two - no error";
        case 3: return (char*)"Message Three - no error";
        case 4: return (char*)"Message Four - no error";
        case 5: return (char*)"Message Five - no error";
        case 6: return (char*)"Message Six - Out of Sequence packet";
        case 7: return (char*)"Message Seven - Length Mismatch Packet";
        case 8: return (char*)"Message Eight - no error";
        case 9: return (char*)"Message Nine - end of packet miss";
        case 10: return (char*)"Message Ten - Duplicate packet";
    }
    return (char*)"Message from Client!";
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
    
    DataPacket datapacket;

    int packet = 1;
    uint8_t expectedPacket = 1;
    while(packet <= 10)
    {
        datapacket = createDataPacket();
        char *payload = generatePayload(packet);
        strcpy(datapacket.payLoad, payload);
        switch(packet)
        {
            case 1:
            case 2:
            case 3:
            case 4:
            case 5: datapacket.segmentNumber = expectedPacket;
                    datapacket.length = strlen(datapacket.payLoad);
                    break;
            case 6: datapacket.segmentNumber = expectedPacket + 3;
                    datapacket.length = strlen(datapacket.payLoad);
                    break;
            case 7: datapacket.segmentNumber = expectedPacket;
                    datapacket.length = strlen(datapacket.payLoad) - 3;
                    break;
            case 8: datapacket.segmentNumber = expectedPacket;
                    datapacket.length = strlen(datapacket.payLoad);
                    break;
            case 9: datapacket.segmentNumber = expectedPacket;
                    datapacket.length = strlen(datapacket.payLoad);
                    datapacket.endOfPacketId = 0xFF0F;
                    break;
            case 10:datapacket.segmentNumber = expectedPacket - 3;
                    datapacket.length = strlen(datapacket.payLoad);
                    break;
        }
        printPacket(datapacket);

        int n = 0, count = 0;
        RejectPacket responsePacket;
        while(n <= 0 && count < 3)
        {
            sendto(socketFd, &datapacket, sizeof(datapacket), 0, (struct sockaddr *)&serverAddress, serverAddrLen);
			n = recvfrom(socketFd, &responsePacket, sizeof(struct RejectPacket), 0, NULL, NULL);
            if (n <= 0)
			{
				printf("No Response received from server, rerying...\n");
				count++;
			}
			else if (responsePacket.type == ACK_PACKET_TYPE)
			{
				printf("ACKnowledgement received from server!!\n \n");
                expectedPacket++;
			}
			else if (responsePacket.type == REJECT_PACKET_TYPE)
			{
				printf("Reject packet received from server! \n");
                printRejectCodeErrorMsg(responsePacket.rejectSubCode);
			}
        }
        if(count >= 3)
        {
            printf("ERROR: Server does not respond.\n");
            return -1;
        }
        packet++;
        printf("\n===================================================\n");
    }
    close(socketFd);
    return 0;
}