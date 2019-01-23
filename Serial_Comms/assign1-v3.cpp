/* EEEN20060 Communication Systems, Assignment 1
   This is an outline of the application program for file transfer.
   This version uses C functions. */

#include <stdio.h>  // standard input-output library
#include "linklayer.h"  // link layer functions

#define BLK_SIZE 80  // data block in bytes - adjust for your design

// Function prototypes
int copyFile(char *fName, bool debug);
int sendFile(char *fName, bool debug);
int receiveFile(char *fName, bool debug);

using namespace std;

int main()
{
    char inString[10];  // string to hold user command
    char fName[80];  // string to hold filename
    int retCode;  // return code from functions
    bool debug = false;  // flag to select more printing

    printf("Assignment 1 - File Transfer Program\n");  // welcome message
    // First ask if the user wants lots of debug information
    printf("\nSelect debug or quiet mode: ");
    scanf("%7s", inString);  // get user response
    if ((inString[0] == 'd')||(inString[0] == 'D')) debug = true;

    // Then ask what the user wants to do
    printf("\nSelect send, receive or copy: ");
    scanf("%7s", inString);  // get user response

    // Decide what to do, based on what the user entered
    switch (inString[0])
    {
        case 'c':
        case 'C':
            printf("\nEnter name of file to copy with extension (name.ext): ");
            scanf("%70s", fName);  // get filename
            printf("\n");  // blank line
            retCode = copyFile(fName, debug);  // call function to copy file
            if (retCode == 0) printf("\nFile copied!\n");
            else printf("\n*** Copy failed, code %d\n", retCode);
            break;

        case 's':
        case 'S':
            printf("\nEnter name of file to send with extension (name.ext): ");
            scanf("%70s", fName);  // get filename
            printf("\n");  // blank line
            retCode = sendFile(fName, debug);  // call function to send file
            if (retCode == 0) printf("\nFile sent!\n");
            else printf("\n*** Send failed, code %d\n", retCode);
            break;

        case 'r':
        case 'R':
            printf("\nEnter name of file to hold received data (name.ext): ");
            scanf("%70s", fName);  // get filename
            printf("\n");  // blank line
            retCode = receiveFile(fName, debug);  // call function to receive file
            if (retCode == 0) printf("\nFile received!\n");
            else printf("\n*** Receive failed, code %d\n", retCode);
            break;

        default:
            printf("\nCommand not recognised\n");
            break;

   } // end of switch

    return 0;
}  // end of main

// ============================================================================
/* Function to demonstrate file handling operations.
   It opens the given input file, and output file "copy.txt".
   It reads blocks of data from the input, and writes each
   block to the output.  It repeats until end-of-file is
   detected on the input, then closes both files.
   If debug is true, it prints progress messages, if false,
   it only prints error messages.
   It returns 0 for success, or a non-zero error code.  */
int copyFile(char *fName, bool debug)
{
    FILE *fpi, *fpo;  // file handles for input and output
    unsigned char data[BLK_SIZE+1];  // array of bytes
    int nRead, nWrite;  // number of bytes read or written
    long byteCount = 0; // total number of bytes read

    // Open the input file and check for failure
    if (debug) printf("Copy: Opening %s for input\n", fName);
    fpi = fopen(fName, "rb");  // open for binary read
    if (fpi == NULL)
    {
        perror("Copy: Error opening input file");
        return 1;
    }

    // Open output file
    if (debug) printf("Copy: Opening copy.txt for output\n");
    fpo = fopen("copy.txt", "wb");  // open for binary write
    if (fpo == NULL)
    {
        perror("Copy: Error opening output file");
        fclose(fpi);  // close the input file
        return 2;
    }

    // Copy the input file to the output file
    do
    {
        // read bytes from input
        nRead = (int) fread(data, 1, BLK_SIZE, fpi);
        if (ferror(fpi))  // check for error
        {
            perror("Copy: Error reading input file");
            fclose(fpi);
            fclose(fpo);
            return 3;
        }
        if (debug) printf("Copy: Read %d bytes, writing...\n", nRead);
        byteCount += nRead;  // add to byte count

        // write bytes to output
        nWrite = (int) fwrite(data, 1, nRead, fpo);
        if (ferror(fpo))  // check for error
        {
            perror("Copy: Error writing output file");
            fclose(fpi);
            fclose(fpo);
            return 4;
        }
    }
    while ((feof(fpi) == 0));  // until input file ends
    if (debug) printf("Copy: End of input file after %ld bytes\n",byteCount);

    fclose(fpi);    // close both files
    fclose(fpo);
    return 0;   // indicate success
}  // end of copyFile

// ============================================================================
/* Function to send a file, using the link layer protocol.
   It opens the given input file, and connects to another computer.
   It reads blocks of data of fixed size from the input, and sends
   each blockover the connection.  It repeats until end-of-file is
   detected on the input.
   If debug is true, it prints progress messages, if false,
   it only prints error messages.
   It returns 0 for success, or a non-zero error code.  */
int sendFile(char *fName, bool debug)
{
    FILE *fpi;  // file handle for input file
    unsigned char data[BLK_SIZE+1];  // array of bytes
    int nRead;  // number of bytes read
    int retCode;  // return code from functions
    long byteCount = 0; // total number of bytes read

    // Open the input file and check for failure
    if (debug) printf("\nSend: Opening %s for input\n", fName);
    fpi = fopen(fName, "rb");  // open for binary read
    if (fpi == NULL)
    {
        perror("Send: Error opening input file");
        return 1;
    }

    // Ask link layer to connect to other computer
    if (debug) printf("Send: Connecting...\n");
    retCode = LL_connect(debug);  // try to connect
    if (retCode < 0)  // problem connecting
    {
        fclose(fpi);     // close input file
        return retCode;  // pass back the error
    }

    /* We have connected to the other computer, but sent no data yet.
       This is where we might transfer the name or the size of the file,
       if that is part of the design.  */

    // Start sending the contents of the file, one block at a time
    do  // loop block by block
    {
        // Special block read - does not ty to interpret data at all
        nRead = (int) fread(data, 1, BLK_SIZE, fpi); // read bytes from file
        if (ferror(fpi))  // check for error
        {
            perror("Send: Error reading input file");
            fclose(fpi);   // close input file
            LL_discon(debug);  // disconnect link
            return 3;  // we are giving up on this
        }
        if (debug) printf("Send: Read %d bytes, sending...\n", nRead);
        byteCount += nRead;  // add to byte count

        retCode = LL_send(data, nRead, debug);  // send bytes to link layer
        // retCode is 0 if succeeded, non-zero if failed
    }
    while ((retCode == 0) && (feof(fpi) == 0));  // until input file ends or error

    if (retCode < 0) printf("Send: Error sending data\n");
    else if (debug) printf("Send: End of input file after %ld bytes\n",byteCount);
    fclose(fpi);    // close input file

    /* We have now sent all the data from the file.  This is where we
       might do something to mark the end of the file, so the receiving
       computer knows that it should close the file and disconnect. */

    // Ask link layer to disconnect
    if (debug) printf("Send: Disconnecting...\n");
    LL_discon(debug);  // ignore return code here...

    return retCode;  // indicate success or failure
}  // end of sendFile

// ============================================================================
/* Function to receive a file, using the link layer protocol.
   It opens the given file for output, and connects to another computer.
   It receives blocks of data over the connection, and writes each block
   to the output file.  It repeats until - - - .
   It then closes the output file and disconnects.
   If debug is true, it prints progress messages, if false,
   it only prints error messages.
   It returns 0 for success, or a non-zero error code.  */
int receiveFile(char *fName, bool debug)
{
    FILE *fpo;  // file handle for output file
    unsigned char data[BLK_SIZE+1];  // array of bytes
    int nRx, nWrite;  // number of bytes received or written
    int retCode;  // return code from other functions
    long byteCount = 0; // total number of bytes received

    // Open the output file and check for failure
    if (debug) printf("RX: Opening %s for output\n", fName);
    fpo = fopen(fName, "wb");  // open for binary write
    if (fpo == NULL)
    {
        perror("RX: Error opening output file");
        return 2;
    }

    // Connect to other computer
    if (debug) printf("RX: Connecting...\n");
    retCode = LL_connect(debug);  // try to connect
    if (retCode < 0)  // problem connecting
    {
        fclose(fpo);     // close output file
        return retCode;  // pass back the error
    }
    printf("RX: Connected, waiting to receive...\n");

    // Get each block of data and write to file
    do  // loop block by block
    {
        nRx = LL_receive(data, BLK_SIZE, debug);  // try to receive data block
        // nRx will be number of bytes received, or negative if error

        if (nRx > 0)  // got some data, so write it to file
        {
            byteCount += nRx;  // add to byte count
            nWrite = (int) fwrite(data, 1, nRx, fpo);  // write bytes to file
            if (ferror(fpo))  // check for error
            {
                perror("RX: Error writing output file");
                nRx = -9;  // fake value to end loop
            }
            if (debug) printf("RX: Wrote %d bytes to file\n", nWrite);
        }
        else if (nRx < 0 ) printf("RX: Error receiving data, code %d\n",nRx);
        else if (debug) printf("RX: Zero bytes received\n");
   }
    while ( nRx == BLK_SIZE );  // repeat until end of file, however you indicate that
    if (debug) printf("RX: End after %ld bytes\n", byteCount);

    fclose(fpo);  // close output file
    // Ask link layer to disconnect
    if (debug) printf("RX: Disconnecting...\n");
    LL_discon(debug);  // ignore return code here...

    return (nRx < 0) ? nRx : 0;  // indicate success or failure
}  // end of receiveFile
