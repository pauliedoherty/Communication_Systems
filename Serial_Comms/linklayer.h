#ifndef LINKLAYER_H_INCLUDED
#define LINKLAYER_H_INCLUDED

// Link Layer Protocol definitions - adjust all these to match your design
#define C_SOH 1     // start of frame marker
#define C_DLE 16    // data link escape (stuff) byte
#define C_EOT 4     // end of frame marker
#define T_DATA 28   // frame type data
#define T_ACK 6     // frame type ACK
#define T_NAK 21    // frame type NAK
#define HDR_SIZE 3  // number of header bytes
#define P_TYPE 0    // position of frame type
#define P_SEQN 1    // position of sequence number
#define P_BCNT 2    // position of byte count
#define MAX_BLK 255 // largest number of data bytes allowed
#define TRL_SIZE 2  // number of trailer bytes
#define P_CKSH 0    // position of high byte of checksum within trailer
#define P_CKSL 1    // position of low byte of checksum within trailer
#define P_EOT  2    // position of end marker within trailer
#define ACK_SIZE HDR_SIZE+TRL_SIZE  // number of bytes in reply
#define MAX_WAIT 10.0  // sender time limit in seconds
#define MAX_TRIES 10  // max attempts to send same block
#define RX_WAIT 60.0  // receiver waiting time in seconds
#define TIMER_TX 0    // timer number for send function
#define TIMER_RX 1    // timer number for receive function
#define PROB_ERR 1.0E-4  // probability of simulated error on receive

/* Functions to implement link layer protocol.
   All functions take a debug argument - if true, they print
   messages explaining what is happening.
   Regardless of debug, functions print messages on errors.
   All functions return negative values on error or failure.  */

// Function to connect to another computer.
int LL_connect(bool debug);

// Function to disconnect from other computer.
int LL_discon(bool debug);

// Function to send a block of data in a frame.
int LL_send(unsigned char *data, int nByte, bool debug);

// Function to receive a frame and extract a block of data.
int LL_receive(unsigned char *data, int maxBytes, bool debug);

/* Function to extract a frame from the received bytes.
   Arguments are pointer to array of bytes to hold frame,
        maximum number of bytes to receive,
        timer number of timer holding time limit.
   Return value is number of bytes recovered, or negative if error.  */
int getFrame(unsigned char *buffer, int maxSize, int timerNum);

/* Function to check a received frame for errors.
   Arguments are pointer to array of bytes holding frame,
        number of bytes to be checked.
   Returns true if frame is good, false otherwise.   */
bool checkFrame(unsigned char *frame, int nByte);

/* Function to send acknowledgement frame.
   Arguments: frame type, sequence number.
   Returns 0 on success, negative on error.  */
int sendAck(int type, int seq);

/* Function to build a frame from a block of data.
   Arguments are array to hold frame, array of data, number of data bytes.
   Return value is size of frame.  */
int buildFrame(unsigned char *frame, unsigned char *data, int nByte);

#endif // LINKLAYER_H_INCLUDED
