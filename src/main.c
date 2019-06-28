/****************************************************************************
 * main.c
 * openacousticdevices.info
 * July 2017
 *****************************************************************************/

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "rs232.h"

/* XMODEM constants and structure */

#define X_SOH           0x01
#define X_STX           0x02
#define X_ACK           0x06
#define X_NAK           0x15
#define X_EOF           0x04

#define PACKET_SIZE     128

#pragma pack(push,1)

struct xmodem_chunk {
    uint8_t start;
    uint8_t block;
    uint8_t block_neg;
    uint8_t payload[PACKET_SIZE];
    uint16_t crc;
};

#pragma pack(pop)

/* Communication constants */

#define MAXIMUM_NUMBER_OF_FLUSH_CYCLES       100

#define MAXIMUM_NUMBER_OF_RECEIVE_CYCLES     50
#define MAXIMUM_NUMBER_OF_PACKET_REPEATS     10

#define DELAY_BETWEEN_PACKETS_MS             10
#define DELAY_BETWEEN_INSTRUCTIONS_MS        250

#define FLASH_CRC_LENGTH                     4
#define SERIAL_NUMBER_LENGTH                 16

/* CRC constant, structure and functions */

#define CRC_POLY 0x1021

static uint16_t crc_update(uint16_t crc_in, int incr) {

    uint16_t xor = crc_in >> 15;
    uint16_t out = crc_in << 1;

    if (incr) {
        out++;
    }

    if (xor) {
        out ^= CRC_POLY;
    }

    return out;

}

static uint16_t crc16(const uint8_t *data, uint16_t size) {

    uint16_t crc, i;

    for (crc = 0; size > 0; size--, data++) {
        for (i = 0x80; i; i >>= 1) {
            crc = crc_update(crc, *data & i);
        }
    }

    for (i = 0; i < 16; i++) {
        crc = crc_update(crc, 0);
    }

    return crc;

}

static uint16_t swap16(uint16_t in) {

    return (in >> 8) | ((in & 0xff) << 8);

}

/* Useful macros */

#define MIN(a, b)       ((a) < (b) ? (a) : (b))

#define PRINT_AND_RETURN(message) { \
    printf("%s\n", message); \
    return 0; \
}

#define PRINT_ERROR_AND_RETURN(message) { \
    printf("ERROR: %s\n", message); \
    return 0; \
}

#define WRITE_WITH_RETURN_ON_ERROR(index, data, length, message) { \
    int bytesSent = comWrite(index, data, length); \
    if (bytesSent != length) { \
        printf("ERROR: %s\n", message); \
        return 0; \
    } \
}

#define WRITE_WITH_CLOSE_AND_RETURN_ON_ERROR(index, data, length, message) { \
    int bytesSent = comWrite(index, data, length); \
    if (bytesSent != length) { \
        printf("ERROR: %s\n", message); \
        comClose(index); \
        return 0; \
    } \
}

/* Default message buffer */

char buffer[1024];
 
/* Useful functions */

void wait_ms(int miliseconds) {

    clock_t start = clock();

    clock_t stop = start + miliseconds * CLOCKS_PER_SEC / 1000;

    while (clock() < stop);

}

bool waitForConfirmationFromReceiver(int index, char *message) {

    int count = 0;

    char answer = '\0';
    
    int charMatch = 0;

    int messageLength = (int)strlen(message);
    
    do {

        wait_ms(DELAY_BETWEEN_PACKETS_MS);

        comRead(index, &answer, sizeof(answer));
        
        if (answer == message[charMatch]) charMatch += 1;
        
        if (charMatch == messageLength) return true;
        
        count += 1;

    } while (charMatch < messageLength && count < MAXIMUM_NUMBER_OF_RECEIVE_CYCLES);

    return false;
    
}

bool waitForResponse(int index, char *buffer, size_t length) {

    int response;

    int count = 0;

    do {

        wait_ms(DELAY_BETWEEN_PACKETS_MS);

        response = comRead(index, buffer, length);

        count += 1;

    } while (response != length && count < MAXIMUM_NUMBER_OF_RECEIVE_CYCLES);

    return (count < MAXIMUM_NUMBER_OF_RECEIVE_CYCLES);

}

void readResponse(int index, char *response, size_t length) {
    
    int len = comRead(index, buffer, sizeof(buffer));
    
    memcpy(response, buffer + (len - length - 2), length);

}

/* Send file using XMODEM protocol */

int sendXMODEM(int index, char *fileData, int fileSize) {

    /* Wait for receiver ping */

    bool response = waitForConfirmationFromReceiver(index, "ReadyC");

    if (!response) PRINT_ERROR_AND_RETURN("Timed out waiting for send confirmation from AudioMoth")

    /* Initialise first chunk */

    struct xmodem_chunk chunk;

    chunk.block = 1;

    chunk.start = X_SOH;

    /* Main write loop */

    int numberOfRepeats = 0;

    int bytesToSend = fileSize;

    while (bytesToSend > 0 && numberOfRepeats < MAXIMUM_NUMBER_OF_PACKET_REPEATS) {

        size_t z = MIN(bytesToSend, sizeof(chunk.payload));

        memcpy(chunk.payload, fileData, z);

        memset(chunk.payload + z, 0xFF, sizeof(chunk.payload) - z);

        chunk.crc = swap16(crc16(chunk.payload, sizeof(chunk.payload)));

        chunk.block_neg = 0xFF - chunk.block;
        
        WRITE_WITH_RETURN_ON_ERROR(index, (char*)&chunk, sizeof(chunk), "Could not send chunk")

        /* Wait for response */

        char answer = '\0';

        response = waitForResponse(index, &answer, sizeof(answer));

        if (!response) PRINT_ERROR_AND_RETURN("Did not receive response from AudioMoth after sending chunk")
 
        if (answer == X_ACK) {

            numberOfRepeats = 0;

            chunk.block += 1;

            bytesToSend -= z;

            fileData += z;

        } else {

            numberOfRepeats += 1;

        }

    }

    if (numberOfRepeats == MAXIMUM_NUMBER_OF_PACKET_REPEATS) PRINT_ERROR_AND_RETURN("Exceeded maximum retries when sending chunk")

    /* Send end of file */

    char eof = X_EOF;

    char answer = '\0';

    WRITE_WITH_RETURN_ON_ERROR(index, (char*)&eof, sizeof(eof), "Could not send end of file")

    response = waitForResponse(index, &answer, sizeof(answer));

    if (!response) PRINT_ERROR_AND_RETURN("Did not receive response from AudioMoth after sending end of file")

    if (answer != X_ACK) PRINT_ERROR_AND_RETURN("Did not receive ACK from AudioMoth after sending end of file")

    return fileSize;

}

/* Code entry point */

char flashCRC[FLASH_CRC_LENGTH + 1];

char serialNumber[SERIAL_NUMBER_LENGTH + 1];

typedef enum {LIST_PORTS, READ_SERIAL_NUMBER, READ_FLASH_CRC, NONDESTRUCTIVE_WRITE, DESTRUCTIVE_WRITE} flashmode_t;

int main(int argc, char **argv) {

    /* Determine the mode */

    flashmode_t mode;

    char *port;
    char *file;

    if (argc == 1) {

        mode = LIST_PORTS;

    } else if (argc == 3 && strcmp(argv[1], "-i") == 0) {

        mode = READ_SERIAL_NUMBER;

        port = argv[2];

    } else if (argc == 3 && strcmp(argv[1], "-c") == 0) {
            
        mode = READ_FLASH_CRC;
            
        port = argv[2];
            
    } else if (argc == 4 && strcmp(argv[1], "-u") == 0) {

        mode = NONDESTRUCTIVE_WRITE;

        port = argv[2];

        file = argv[3];

    } else if (argc == 4 && strcmp(argv[1], "-d") == 0) {

        mode = DESTRUCTIVE_WRITE;

        port = argv[2];

        file = argv[3];

    } else {

        PRINT_ERROR_AND_RETURN("Incorrect arguments.\nflash               // List serial ports\nflash -i port       // Show AudioMoth serial number\nflash -c port       // Show current firmware CRC value\nflash -u port file  // Upload new firmware")

    }

    /* Enumerate the port */

    int numberOfPorts = comEnumerate();

    /* Print list of ports */

    if (mode == LIST_PORTS) {

        int count = 0;

        for (int i = 0; i < numberOfPorts; i += 1) {

            #ifdef _WIN32
                if (strstr(comGetPortName(i), "COM") != NULL) {
            #else
                if (strstr(comGetInternalName(i), "usb") != NULL || strstr(comGetInternalName(i), "ACM") != NULL) {
            #endif

                if (count > 0) {
                    printf("%s", " ");
                }

                #ifdef _WIN32
                    printf("%s", comGetPortName(i));                 
                #else
                    printf("%s", comGetInternalName(i));
                #endif

                count += 1;

            }

        }

        if (count == 0) PRINT_AND_RETURN("No serial ports found")

        printf("\n");

        return 0;

    }

    /* Find the specified port */

    #ifdef _WIN32

	    int index = comFindPort(port);

        if (index == -1) PRINT_ERROR_AND_RETURN("Could not find port")

    #else

        int index = 0;

        while (index < numberOfPorts && strcmp(port, comGetInternalName(index))) {

            index += 1;

        }

        if (index == numberOfPorts) PRINT_ERROR_AND_RETURN("Could not find port")
            
    #endif

    /* Read the file */

    int fileSize = 0;

    char *fileData = NULL;

    if (mode == NONDESTRUCTIVE_WRITE || mode == DESTRUCTIVE_WRITE) {

        FILE *fileHandle = fopen(file, "rb");

        if (fileHandle == NULL) PRINT_ERROR_AND_RETURN("Could not open file")

        /* Read contents into buffer */

        fseek(fileHandle, 0, SEEK_END);

        fileSize = (int)ftell(fileHandle);
    
        if (fileSize == 0) PRINT_ERROR_AND_RETURN("File has zero size")
								   
	if (mode == NONDESTRUCTIVE_WRITE && fileSize > 256 * 1024 - 0x4000) PRINT_ERROR_AND_RETURN("File is too big")

        if (mode == DESTRUCTIVE_WRITE && fileSize > 256 * 1024) PRINT_ERROR_AND_RETURN("File is too big")

        fileData = malloc(fileSize);

        if (fileData == NULL) PRINT_ERROR_AND_RETURN("Could not allocate memory for file data")

        fseek(fileHandle, 0, SEEK_SET);

        fread(fileData, sizeof(char), fileSize, fileHandle);

        fclose(fileHandle);

    }

    /* Open and flush specified port */

    if (!comOpen(index, 9600)) PRINT_ERROR_AND_RETURN("Could not open port")

    int count = 0;

    while (comRead(index, buffer, sizeof(buffer)) > 0 && count < MAXIMUM_NUMBER_OF_FLUSH_CYCLES) {

        count += 1;

    }

    if (count == MAXIMUM_NUMBER_OF_FLUSH_CYCLES) PRINT_ERROR_AND_RETURN("Could not flush serial port")

    wait_ms(DELAY_BETWEEN_INSTRUCTIONS_MS);

    /* Read serial number */

    if (mode == READ_SERIAL_NUMBER) {

        WRITE_WITH_CLOSE_AND_RETURN_ON_ERROR(index, "i", 1, "Could not send 'i' instruction")

        wait_ms(DELAY_BETWEEN_INSTRUCTIONS_MS);
        
        readResponse(index, serialNumber, SERIAL_NUMBER_LENGTH);

        printf("Serial Number: %s\n", serialNumber);

        comClose(index);

        return 0;

    }
    
    /* Read the flash CRC */
        
    if (mode == READ_FLASH_CRC) {
        
        WRITE_WITH_CLOSE_AND_RETURN_ON_ERROR(index, "c", 1, "Could not send 'c' instruction")
        
        wait_ms(DELAY_BETWEEN_INSTRUCTIONS_MS);
        
        readResponse(index, flashCRC, FLASH_CRC_LENGTH);
        
        printf("Flash CRC: %s\n", flashCRC);
        
        comClose(index);
        
        return 0;
        
    }

    /* Send the file to the device */

    if (mode == NONDESTRUCTIVE_WRITE) {

        /* Send non-destructive write instruction */

        WRITE_WITH_CLOSE_AND_RETURN_ON_ERROR(index, "u", 1, "Could not send 'u' instruction")

    } else {

        /* Confirm with user */

        char response[5];

        printf("This will overwrite the bootloader. Are you sure? Type 'y' or 'yes' to confirm. : ");

        fgets(response, 5, stdin);

        if (strcmp(response, "y\n") != 0 && strcmp(response, "yes\n") != 0) PRINT_AND_RETURN("No upload performed")

        /* Send destructive write instruction */

        WRITE_WITH_CLOSE_AND_RETURN_ON_ERROR(index, "d", 1, "Could not send 'd' instruction")

    }

    wait_ms(DELAY_BETWEEN_INSTRUCTIONS_MS);

    int size = sendXMODEM(index, fileData, fileSize);

    if (size > 0) {

        printf("Programmed: %d bytes\n", size);

        if (mode == NONDESTRUCTIVE_WRITE) {
            
            WRITE_WITH_CLOSE_AND_RETURN_ON_ERROR(index, "c", 1, "Could not send 'c' instruction")
            
        } else {
            
            WRITE_WITH_CLOSE_AND_RETURN_ON_ERROR(index, "v", 1, "Could not send 'v' instruction")
            
        }
        
        wait_ms(DELAY_BETWEEN_INSTRUCTIONS_MS);
        
        readResponse(index, flashCRC, FLASH_CRC_LENGTH);
        
        printf("Flash CRC: %s\n", flashCRC);

        WRITE_WITH_CLOSE_AND_RETURN_ON_ERROR(index, "b", 1, "Could not send 'b' instruction")

    }

    comClose(index);

    return 0;

}
