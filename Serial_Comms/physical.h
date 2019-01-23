#ifndef PHYSICAL_H_INCLUDED
#define PHYSICAL_H_INCLUDED
/*  Physical Layer functions using serial port.
       PHY_open        opens and configures the port
       PHY_close       closes the port
       PHY_send        sends bytes
       PHY_receive     gets received bytes
    All functions print explanatory messages if there is
    a problem, and return values to indicate failure. */

/* PHY_open function - to open and configure the serial port.
   Takes no arguments, to keep things simple - we assume that
   we know which port and what settings are needed...
   Returns zero if it succeeds - anything non-zero is a problem.*/
int PHY_open();

/* PHY_close function, to close the serial port.
   Takes no arguments, returns 0 always.  */
int PHY_close();

/* PHY_send function, to send bytes.
   Arguments: unsigned char array holding bytes to be sent;
              number of bytes to send.
   Returns number of bytes sent, or negative value on error.  */
int PHY_send(unsigned char *cTx, int bytesToSend);

/* PHY_get function, to get received bytes.
   Arguments: unsigned char array to hold received bytes;
              maximum number of bytes to get.
   Returns number of bytes actually got, or negative value on error.  */
int PHY_get(unsigned char *cRx, int bytesToGet, double probErr);

/* Function to print informative error messages
   when something goes wrong...  */
void printError(void);

#endif // PHYSICAL_H_INCLUDED
