/*  Physical Layer functions using serial port.
       PHY_open    opens and configures the port
       PHY_close   closes the port
       PHY_send    sends bytes
       PHY_get     gets received bytes
    All functions print explanatory messages if there is
    a problem, and return values to indicate failure.
    This version uses standard C functions.  */

#include <stdio.h>   // needed for printf
#include <windows.h>  // needed for port functions
#include <stdlib.h>  // for random number functions
#include <time.h>    // for time function, used to seed rand
#include "physical.h"  // header file for these functions

using namespace std;

/* Creating a variable this way allows it to be shared
   by the functions in this file only.  */
static HANDLE serial = INVALID_HANDLE_VALUE;  // handle for serial port

/* PHY_open function - to open and configure the serial port.
   Takes no arguments, to keep things simple - we assume that
   we know which port and what settings are needed...
   Returns zero if it succeeds - anything non-zero is a problem.*/
int PHY_open()
{
    // Try to open the port
    serial = CreateFile("COM1", GENERIC_READ | GENERIC_WRITE,
                        0, 0, OPEN_EXISTING, 0, 0);
    // Check for failure
    if (serial == INVALID_HANDLE_VALUE)
    {
        printf("PHY: Failed to open port\n");
        printError();  // give details of the error
        return 1;  // non-zero return value indicates error
    }

    // Make a Device Control Block structure, then set its length
    DCB serialParams = {0};
    serialParams.DCBlength = sizeof(serialParams);

    // Fill the DCB with the parameters of the port, and check for failure
    if (!GetCommState(serial, &serialParams))
    {
        printf("PHY: Error getting port parameters\n");
        printError();  // give details of the error
        CloseHandle(serial);
        return 2;
    }

    /* Change the parameters to configure the port to carry 8-bit
       data, with no parity, without interpreting or substituting
       any characters, and with no flow control.  */
    serialParams.BaudRate = CBR_4800;    // bit rate
    serialParams.ByteSize = 8;           // number of data bits in group
    serialParams.Parity = NOPARITY;      // parity mode
    serialParams.StopBits = ONESTOPBIT;  // just one stop bit
    serialParams.fOutxCtsFlow = false;   // ignore CTS signal
    serialParams.fOutxDsrFlow = false;   // ignore DSR signal on transmit
    serialParams.fDsrSensitivity = false; // ignore DSR signal on receive
    serialParams.fOutX = false;           // ignore XON/XOFF on transmit
    serialParams.fInX = false;           // do not send XON/XOFF on receive
    serialParams.fParity = false;        // ignore parity on receive
    serialParams.fNull = false;   // do not discard null bytes on receive

    // Apply the new parameters to the port
    if (!SetCommState(serial, &serialParams))
    {
        printf("PHY: Error setting port parameters\n");
        printError();  // give details of the error
        CloseHandle(serial);
        return 3;
    }

    // Make a COMMTIMEOUTS structure
    COMMTIMEOUTS serialTimeLimits = {0};

    /* Set the timeout values. At 4800 bit/s, each byte should take ~ 2.1 ms.
       So set the multipliers to 5 ms, with a small constant value.
       On receive, use interval timeout to stop quickly on a short frame. */
    serialTimeLimits.ReadIntervalTimeout = 20;
    serialTimeLimits.ReadTotalTimeoutConstant = 10;
    serialTimeLimits.ReadTotalTimeoutMultiplier = 5;
    serialTimeLimits.WriteTotalTimeoutMultiplier = 5;
    serialTimeLimits.WriteTotalTimeoutConstant = 10;

    // Apply the time limits to the port
    if (!SetCommTimeouts(serial, &serialTimeLimits))
    {
        printf("PHY: Error setting timeouts\n");
        printError();  // give details of the error
        CloseHandle(serial);
        return 4;
    }

    // Set the seed for random number generator
    srand(time(NULL));  // get time and use as seed

    // If we get this far, the port is open and configured
    return 0;
}

//===================================================================
/* PHY_close function, to close the serial port.
   Takes no arguments, returns 0 always.  */
int PHY_close()
{
    CloseHandle(serial);
    return 0;
}

//===================================================================
/* PHY_send function, to send bytes.
   Arguments: unsigned char array holding bytes to be sent;
              number of bytes to send.
   Returns number of bytes sent, or negative value on error.  */
int PHY_send(unsigned char *cTx, int bytesToSend)
{
     DWORD bytesSent;  // double-word - number of bytes actually sent

    // First check if the port is open
    if (serial == INVALID_HANDLE_VALUE)
    {
        printf("PHY: Port not valid\n");
        return -9;  // negative return value indicates error
    }

    // Try to send the bytes as requested
    if (!WriteFile(serial, cTx, bytesToSend, &bytesSent, NULL ))
    {
        printf("PHY: Error sending data\n");
        printError();  // give details of the error
        CloseHandle(serial);
        return -5;
    }
    else if ((int)bytesSent != bytesToSend)  // check for timeout
    {
        printf("PHY: Timeout in transmission, sent %d of %d bytes\n",
               (int) bytesSent, bytesToSend);
    }
    return (int)bytesSent; // if no error, return number of bytes sent
}

//===================================================================
/* PHY_get function, to get received bytes.
   Arguments: unsigned char array to hold received bytes;
              maximum number of bytes to get.
   Returns number of bytes actually got, or negative value on error.  */
int PHY_get(unsigned char *cRx, int bytesToGet, double  probErr)
{
     DWORD bytesGot;  // double-word - number of bytes actually got
     int nBytes;      // number of bytes received
     int threshold = 0;  // threshold for error simulation

    // First check if the port is open
    if (serial == INVALID_HANDLE_VALUE)
    {
        printf("PHY: Port not valid\n");
        return -9;  // negative return value indicates error
    }
    // Try to get bytes as requested
    if (!ReadFile(serial, cRx, bytesToGet, &bytesGot, NULL ))
    {
        printf("PHY: Error receiving data\n");
        printError();  // give details of the error
        CloseHandle(serial);
        return -4;
    }
    // No need to complain about timeout here - will happen regularly

    nBytes = (int) bytesGot;  // convert to integer

    // Add an error, with specified probability
    if (probErr != 0.0)
    {
        // set threshold as fraction of max, scaling for 8 bit bytes
        threshold = 1 + int(8.0 * (double)RAND_MAX * probErr);
        for (int i = 0; i < nBytes; i++)
        {
            if (rand() < threshold)  // we want to cause an error
            {
                cRx[i] ^= 34;  // invert a couple of bits
                printf("PHY_get: Simulated error..\n");
            }
        }
    }

    return nBytes; // if no problem, return number of bytes we got
}

/* Function to print informative error messages
   when something goes wrong...  */
void printError(void)
{
	char lastError[1024];
	int errCode;
	errCode = GetLastError();  // get the error code for the last error
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		lastError, 1024, NULL);  // convert error code to error message
	printf("PHY: Code %d = %s\n", errCode, lastError);
}
