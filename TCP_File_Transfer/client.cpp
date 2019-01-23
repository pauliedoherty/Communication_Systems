#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <iostream>

#define BLK_SIZE 300

void printError(void);  // function to display error messages
int sendFile(SOCKET cSocket, char *fName, bool debug);
void sendReply(SOCKET cSocket, char* response);

using namespace std;

int main()
{
    WSADATA wsaData;  // create structure to hold winsock data
    int retVal, nRx, nIn;
    bool endLine = false, stop = false;
    char serverIP[20];      // IP address of server
    int serverPort;         // port used by server
    char request[100];      // array to hold user input
    char fName[100];        ///
    int reqLen;             ///

    char reply[100];        // array to hold received bytes
    char responseType;
    int filesize;
    int retval;
    int k;   // for loop with header
    int byteCount;// incremented through to see if it matches filesize
int nWrite=0; //character written to the file
 FILE *fpo;   //opening receiving file
 char *savefile; //saves the bytes to this file name


    //char *search = " "; //used to split string

//     char choice;       //decides if client wants to send or receive


    // Initialise winsock, version 2.2, giving pointer to data structure
    retVal = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (retVal != 0)  // check for error
    {
        printf("*** WSAStartup failed: %d\n", retVal);
        printError();
        return 1;
    }
    else printf("WSAStartup succeeded\n" );

    // Create a handle for a socket, to be used by the client
    SOCKET clientSocket = INVALID_SOCKET;  // handle called clientSocket

    // Create the socket, and assign it to the handle
    // AF_INET means IP version 4,
    // SOCK_STREAM means socket works with streams of bytes,
    // IPPROTO_TCP means TCP transport protocol.
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET)  // check for error
    {
        printf("*** Failed to create socket\n");
        printError();
        stop = true;
    }
    else printf("Socket created\n" );

    // Get the details of the server from the user
    printf("Enter IP address of server: ");
    scanf("%20s", serverIP);  // get IP address as string

    printf("Enter port number: ");
    scanf("%d", &serverPort);     // get port number as integer

    gets(request);  // flush the endline from the input buffer

    // Build a structure to identify the service required
    // This has to contain the IP address and port of the server
    sockaddr_in service;  // IP address and port structure
    service.sin_family = AF_INET;  // specify IP version 4 family
    service.sin_addr.s_addr = inet_addr(serverIP);  // set IP address
    // function inet_addr() converts IP address string to 32-bit number
    service.sin_port = htons(serverPort);  // set port number
    // function htons() converts 16-bit integer to network format

    // Try to connect to the service required
    printf("Trying to connect to %s on port %d\n", serverIP, serverPort);
    retVal = connect(clientSocket, (SOCKADDR *) &service, sizeof(service));
    if( retVal != 0)  // check for error
    {
        printf("*** Error connecting\n");
        printError();
        stop = true;  // make sure we do not go into the while loop
    }
    else printf("Connected!\n");
    // Main loop to send messages and receive responses
    // This example assumes client will send first
    while (!stop)
    {


/*
        // Get user request and send it to the server
        printf("Enter filename you wish to send/request ");
        gets(request);  // read in the string
        // gets() reads until enter (CR), but does not put CR in string
*/

        // Get user request and send it to the server
a:
        char choice[10];
        char newNameYN[10];
        char new_fName[100];
        bool wantsNewName = false;
        cout << "\nWhat would you like to do?\n\t1. Get a file from the server (G)\n\t2. Place a file on the server (P)\n\t3. Close the connection (Q)\nEnter your choice: (G/P/Q): ";
        gets(choice);
        //scanf("%c",&choice);

        switch(choice[0])
        {
            case '1':
            case 'g':
            case 'G':
            {
                cout << "You have chosen GET" << endl;
                cout << "Enter filename, with extension, which you wish to request: ";
                gets(fName);  // read in the string
                cout << "Do you want to specify a new name for saving the file locally? (Y/N): ";
                gets(newNameYN);
                switch(newNameYN[0])
                {
                    case 'Y':
                    case 'y':
                    {
                       cout << "Enter new local filename: ";
                       gets(new_fName);
                       wantsNewName = true;
                       break;
                    }
                    default:
                    {
                        break;
                    }
                }
                reqLen=sprintf(request,"G %s",fName);
                break;
            }
            case '2':
            case 'p':
            case 'P':
            {
                //cout << "Operation not yet supported!\n\tClosing connection..." << endl;
                //reqLen=sprintf(request,"$");
                //break;
                bool debug=true;
                cout << "You have chosen PUT" << endl;
                cout << "Enter filename, with extension, which you wish to upload: ";
                gets(fName);  // read in the string
                cout << "Do you want to specify a new name for saving the file on the server? (Y/N): ";
                gets(newNameYN);
                switch(newNameYN[0])
                {
                    case 'Y':
                    case 'y':
                    {
                       cout << "Enter new name for upload: ";
                       gets(new_fName);
                       reqLen=sprintf(request,"P %s\n",new_fName);
                       break;
                    }
                    default:
                    {
                        reqLen=sprintf(request,"P %s\n",fName);
                        break;
                    }
                }
                sendReply(clientSocket,request);
                sendFile(clientSocket, fName, debug);
                cout << "sendFile function returned successfully." << endl;
                goto a;
                break;
            }
            case '3':
            case 'q':
            case 'Q':
            {
                cout << "You have requested to close the connection.\n\tClosing connection..." << endl;
                reqLen=sprintf(request,"$");
                break;
            }
            default:
            {
                cout <<  "Operation not recognised! Please try again!" << endl;
                goto a;
                break;
            }
        }


//switch(choice)
//{
//case "r" :

//break;

/*case "" : printf("\n Enter filename(with extension) you wish to send to server:");
gets(sending);
break;
}  */


        nIn = strlen(request);  //find the length
        request[nIn] = 13;  // replace null terminator with CR
        request[nIn+1] = 10;  // add LF
        stop = (request[0] == '$');  // set stop flag

        if (stop) printf("Closing connection as requested...\n");
        else  // send the message and try to receive a reply
        {
            // send() arguments: socket handle, array of bytes to send,
            // number of bytes to send, and last argument of 0.
            retVal = send(clientSocket, request, nIn+2, 0);  // send nIn+2 bytes
            // retVal will be number of bytes sent, or error indicator

            if( retVal == SOCKET_ERROR) // check for error
            {
                printf("*** Error sending\n");
                printError();
            }
            else printf("Sent %d bytes, waiting for reply...\n", retVal);

            endLine = false;
///            do  // loop to receive entire reply, terminated by LF
///            {
                // Try to receive some bytes
                // recv() arguments: socket handle, array to hold rx bytes,
                // maximum number of bytes to receive, last argument 0.

                ///receive header from the server
                nRx = recv(clientSocket, reply, BLK_SIZE, 0);
                // nRx will be number of bytes received, or error indicator

                if( nRx == SOCKET_ERROR)  // check for error
                {
                    printf("Problem receiving\n");
                    //printError();
                    stop = true;  // exit the loop if problem
                }
                else if (nRx == 0)  // connection closed
                {
                    printf("Connection closed by server\n");
                    stop = true;
                }

                ///header will contain some of the file, this marks the header as seperate from the file
                for (k=0; k<nRx; k++)
                {
                    if (reply[k] == '*')
                    {
                        reply[k] = 0;
                        break;
                    }
                }

                ///interpret header from server
                retval = sscanf(reply, "%c%d", &responseType, &filesize);
                if(retval==2) printf("Extracted response: %c\tFile size: %d bytes\n", responseType, filesize);

                switch(responseType)
                {
                    case 'P':
                    {
                        cout << "File is availible on server" << endl;

                            // Open the output file and check for failure
                            if (wantsNewName)
                            {
                                printf("RX: Opening %s for output\n", new_fName);
                                fpo = fopen(new_fName, "wb");  // open for binary write
                                if (fpo == NULL)
                                {
                                perror("RX: Error opening output file");
                                    return 2;
                                }
                            }
                            else
                            {
                                printf("RX: Opening %s for output\n", fName);
                                fpo = fopen(fName, "wb");  // open for binary write
                                if (fpo == NULL)
                                {
                                    perror("RX: Error opening output file");
                                    return 3;
                                }
                            }


                            if(nRx > k)
                            {
                                nWrite = (int) fwrite(&reply[k+1], 1, nRx-k-1, fpo);
                                if (ferror(fpo))  // check for error
                                {
                                    perror("RX: Error writing output file");
                                    fclose(fpo);
                                    return -9;
                                }
                                printf("RX: Wrote %d bytes to file\n", nWrite);
                            }

                            byteCount = nWrite; //keeping track of how many bytes are put into the file


                            do{

                                    int recvBytes=0; //holds number of received bytes
                                    char data[BLK_SIZE];  //array to hold file bytes

                                    recvBytes = recv(clientSocket, data, BLK_SIZE, 0);


                                    if (recvBytes > 0)  // got some data, so write it to file
                                    {

                                        nWrite = (int) fwrite(data, 1, recvBytes, fpo);  // write bytes to file

                                        printf("RX: Wrote %d bytes to file\n", nWrite);
                                    }

                                    byteCount += nWrite; //add to byte Count
                                }
                            while(byteCount < filesize);

                            if (byteCount == filesize)//compares what the server told us was in the file to the recieved bytes
                            {
                                printf("Correct number of bytes has been received\n");
                                printf("%d bytes have been written to file\n", byteCount);
                            }
                        //closing file
                        fclose(fpo);
                        break;
                    }

                    case 'N':
                    {
                        printf("File not found on server\n");
                        break;
                    }
                    default:
                        printf("Error in receiving header from server \n"); //if message does not contain P or N it is corrupted
                        break;
                }
                    //stop = 1;//breaks the loop
///break;
///            }
///            while (!endLine && !stop);
///            // continue until endline or error or connection closed
///            // if it was endline, the outer loop should continue

        } // end else (not stop)

    }  // end while not stop
    // When this loop exits, it is time to tidy up and end the program

/********************************/

    // Shut down the sending side of the TCP connection first
    retVal = shutdown(clientSocket, SD_SEND);
    if( retVal != 0)  // check for error
    {
        printf("*** Error shutting down sending\n");
        printError();
    }

    // Then close the socket
    retVal = closesocket(clientSocket);
    if( retVal != 0)  // check for error
    {
        printf("*** Error closing socket\n");
        printError();
    }
    else printf("Socket closed\n");

    // Finally clean up the winsock system
    retVal = WSACleanup();
    printf("WSACleanup returned %d\n",retVal);

    // Prompt for user input, so window stays open when run outside CodeBlocks
    printf("\nPress return to exit:");
    gets(request);
    return 0;
}


/* Function to print informative error messages
   when something goes wrong...  */
void printError(void)
{
	char lastError[1024];
	int errCode;

	errCode = WSAGetLastError();  // get the error code for the last error
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		errCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		lastError,
		1024,
		NULL);  // convert error code to error message
	printf("WSA Error Code %d = %s\n", errCode, lastError);
}

void sendReply(SOCKET cSocket, char* response)
{
    //printf("Enter response (max 90 char, $ to close connection): ");
    //gets(response);  // read in string
    // gets() reads until enter (CR), but does not put CR in string
    int retVal;
    int nIn = strlen(response);  //find the length

/*
    //don't need to do this in this function
    response[nIn] = 13;  // replace null terminator with CR
    response[nIn+1] = 10;  // add LF
*/

    bool stop = (response[0] == '$');  // set stop flag

    if (!stop)
    {
        // send() arguments: socket handle, array of bytes to send,
        // number of bytes to send, and last argument of 0.
        retVal = send(cSocket, response, nIn, 0);  // send nIn instead of nIn+2 bytes for some reason
        // retVal will be number of bytes sent, or error indicator

        if( retVal == SOCKET_ERROR)  // check for error
        {
            printf("*** Error sending response\n");
            printError();
        }
        else printf("Sent %d bytes, waiting for reply...\n", retVal);

    }  // end if not stop
    return;
}

int sendFile(SOCKET cSocket, char *fName, bool debug)
{
    FILE *fpi;  // file handle for input file
    char data[BLK_SIZE+1];  // array of bytes
    int nRead;  // number of bytes read
    int retCode;  // return code from functions
    long byteCount = 0; // total number of bytes read

    // Open the input file and check for failure
    if (debug) printf("\nSend: Opening %s for input\n", fName);
    fpi = fopen(fName, "rb");  // open for binary read
    if (fpi == NULL)
    {
        perror("Send: Error opening input file");
        char notFound[]="N";
        cout << "Reply to be sent is: " << notFound << endl;
        sendReply(cSocket, notFound);
        return 1;
    }

//    // Ask link layer to connect to other computer
//    if (debug) printf("Send: Connecting...\n");
//    retCode = LL_connect(debug);  // try to connect
//    if (retCode < 0)  // problem connecting
//    {
//        fclose(fpi);     // close input file
//        return retCode;  // pass back the error
//    }

    /* We have connected to the other computer, but sent no data yet.
       This is where we might transfer the name or the size of the file,
       if that is part of the design.  */

       int filesize;
       fseek(fpi,0,SEEK_END);
       filesize = ftell(fpi);
       fseek(fpi,0,SEEK_SET);
       cout << endl << "Filesize = " << filesize << endl;


       char reply[10];

       sprintf(reply, "P%d*", filesize);

       cout << "Relpy to be sent is: " << reply << endl;
       sendReply(cSocket, reply);






    // Start sending the contents of the file, one block at a time
    do  // loop block by block
    {
        // Special block read - does not ty to interpret data at all
        nRead = (int) fread(data, 1, BLK_SIZE, fpi); // read bytes from file
        if (ferror(fpi))  // check for error
        {
            perror("Send: Error reading input file");
            fclose(fpi);   // close input file
            return 3;  // we are giving up on this
        }
        if (debug) printf("Send: Read %d bytes, sending...\n", nRead);
        byteCount += nRead;  // add to byte count

        retCode = send(cSocket, data, nRead, 0);
        //retCode = LL_send(data, nRead, debug);  // send bytes to link layer
        // retCode is 0 if succeeded, non-zero if failed
    }
    while ((retCode == nRead) && (feof(fpi) == 0));  // until input file ends or error

    if (retCode < 0) printf("Send: Error sending data\n");
    else {
         printf("Send: End of input file after %ld bytes\n",byteCount);
    }

    fclose(fpi);    // close input file

    // Ask link layer to disconnect
    if (debug) printf("Send: File sent. File Closed\n");

    return retCode;  // indicate success or failure
}  // end of sendFile

