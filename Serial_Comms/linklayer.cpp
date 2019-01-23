/* Functions to implement link layer protocol.
   LL_connect() connects to another computer;
   LL_discon()  disconnects
   LL_send()    sends a block of data
   LL_receive() waits to receive a block of data
   All functions take a debug argument - if true, they print
   messages explaining what is happening.
   Regardless of debug, functions print messages on errors.
   All functions return negative values on error or failure.
   Definitions of constants are in the header file.  */

#include <stdio.h>     // input-output library: print & file operations
#include <stdlib.h>    // standard library: malloc, free
#include "physical.h"  // physical layer functions
#include "timeout.h"   // timer functions
#include "linklayer.h"  // these functions

// Declare shared variables (private to functions in this file)
static bool connected = false;  // keep track of state of connection
static int seqNum = 0;          // sequence number for sending data blocks
static int lastSeq = -1;        // sequence number for receiving data blocks
static int framesSent = 0;      // count of frames sent
static int acksRx = 0;          // count of ACKs received
static int naksRx = 0;          // count of NAKs received
static int badFrames = 0;       // count of bad frames received
static int goodFrames = 0;      // count of good frames received
static int timeouts = 0;        // count of timeouts

/* Function to connect to another computer.
   It just calls PHY_open() and reports any error.  */
int LL_connect(bool debug)
{
    int retCode = PHY_open();  // try to connect
    if (retCode == 0)   // check if succeeded
    {
        connected = true;  // record that we are connected
        seqNum = 0;        // set sequence number to zero
		lastSeq = -1;		// set receive seq. number
        framesSent = 0;     // initialise counters for debug
        acksRx = 0;
        naksRx = 0;
        badFrames = 0;
        goodFrames = 0;
        timeouts = 0;
        if (debug) printf("LL: Connected\n");
        return 0;
    }
    else  // failed
    {
        connected = false;  // record lack of connection
        printf("LL: Failed to connect, PHY returned code %d\n",retCode);
        return -retCode;  // return negative error code
    }
}

/* Function to disconnect from other computer.
   It just calls PHY_close() and prints debug info.  */
int LL_discon(bool debug)
{
    int retCode = PHY_close();  // try to disconnect
    connected = false;  // assume no longer connected
    if (retCode == 0)   // check if succeeded
    {
        if (debug)
        {
            printf("LL: Disconnected.  Sent %d frames\n", framesSent);
            printf("LL: Received %d good and %d bad frames, had %d timeouts\n",
                   goodFrames, badFrames, timeouts);
            printf("LL: Received %d ACKs, %d NAKs\n", acksRx, naksRx);
        }
        return 0;

    }
    else  // failed
    {
        printf("LL: Failed to disconnect, PHY returned code %d\n", retCode);
        return -retCode;  // return negative error code
    }
}

/* Function to send a block of data in a frame.
   Arguments:  Data block as array of bytes,
               number of bytes to send.
   Return value is 0 on success, negative on failure.
   If connected, builds header and trailer to form frame, then sends
   the bytes of the frame to PHY_send, and waits for a reply, up to
   time limit.  Then - - -  */
int LL_send(unsigned char *data, int nByte, bool debug)
{
    unsigned char *frame;            // pointer to frame array
    unsigned char ackFrame[ACK_SIZE]; // array of bytes for reply
    int frameSize = 0;                // size of transmit frame
    int retVal;  // return value from other functions
    int tryCount = 0;  // number of attempts to send
    bool success = false; // flag to indicate block sent and ACKed

    // First check if connected
    if (connected == false)
    {
        printf("LL: Attempt to send while not connected\n");
        return -10;  // error code
    }

    // Then check if block size OK - adjustlimit for your design
    if (nByte > MAX_BLK)
    {
        printf("LL: Cannot send block of %d bytes, max %d\n", nByte, MAX_BLK);
        return -11;  // error code
    }

    /* Create an array big enough to hold the frame.  The example just adds
       the header and trailer lengths to the data block size.
       If using byte stuffing, this should be HDR_SIZE+(2*nByte)+TRL_SIZE
       to guarantee that it will always be big enough.  */
    frame = (unsigned char *) malloc(HDR_SIZE+nByte+TRL_SIZE);
    if (frame == NULL) // problem allocating memory
    {
        printf("LL: Failed to allocate memory for tx frame\n");
        return -19;  // return error code
    }
    /* Now that you have got some memory, you must be careful to free it
       before returning from this function - no matter what error occurs! */

    // Build the frame in the new array
    frameSize = buildFrame(frame, data, nByte);

    // Sending loop - we may have to re-send this frame...
    do  // always do this once, then check and see if we need to repeat
    {
        // Send the frame, then check for problems
        retVal = PHY_send(frame, frameSize);  // send frame bytes
        if (retVal != frameSize)  // problem!
        {
            printf("LL: Block %d, failed to send frame\n", seqNum);
            free(frame);  // release the memory before returning
            return -12;  // error code
        }
        if (debug) printf("LL: Sent frame %d bytes, block %d\n", frameSize, seqNum);

        tryCount++;    // increment attempt counter - we will only try so often
        framesSent++;  // increment frame counter (for debug)

        // Wait for reply, up to time limit specified
        timeSet(TIMER_TX, MAX_WAIT);  // set time limit, using timer specified

        // Receiving loop - keep trying to get a response until time limit
        do
        {
            // Get a frame from the physical layer, but don't spend too long
            retVal = getFrame(ackFrame, ACK_SIZE, TIMER_TX);
            if (retVal < 0)  // problem
            {
                free(frame);  // release the memory before returning
                return retVal;  // quit if fatal error
            }

            if (retVal > 0)  // we received something, so check it
            {
                // Check frame for errors
                if (checkFrame(ackFrame, retVal))  // frame is good
                {
                    goodFrames++;  // increment the counter
                    // Check if this reply relates to the block we just sent
                    if (ackFrame[P_SEQN] == seqNum)
                        success = true;  // we have a response
                }
                else // bad frame
                {
                    if (debug) printf("LL: Bad frame received\n");
                    badFrames++;  // increment bad frame counter
                }
            }
            /* The only option remaining is retVal == 0, meaning we
               received nothing - this will happen a lot in Stop & Wait.
               Just loop and try again, until the waiting time expires.  */

            /* If using enquiry frames, check the time limit here.  If waited
               long enough and no reply, send enquiry frame.  Then set a new
               time limit, increment counter and loop back to wait for
               the response.  */
        }
        // This version for no enquiry, so if time up just re-send frame:
        // while (!success && timeOK(TIMER_TX)); // until reply found or time up
        // This version for enquiry - exit loop after a few enquiry frames:
        // while (!success && (enqCount < MAX_ENQ ));  // until reply or limit

        while (success); // we got a reply - need to check what type
        {
            if (ackFrame[P_TYPE] == T_ACK)  // positive acknowledgement
            {
                if (debug) printf("LL: ACK received, seq. %d\n", seqNum);
                acksRx++;       // increment ACK counter
            }
            else if (ackFrame[P_TYPE] == T_NAK) // negative acknowledgement
            {
                if (debug) printf("LL: NAK received, seq. %d\n", seqNum);
                naksRx++;       // increment NAK counter
                success = false;  // and mark as failure
            }
            // Extend this for WACK or other response if used
            else // some other type of frame

            {
                if (debug) printf("LL: Unexpected frame type received\n");
                success = false;  // mark as failure
            }
        }
        // This code for option with no enquiry only...
         // no response, must have been timeout
        {
            if (debug) printf("LL: Block %d, timeout waiting for reply\n", seqNum);
            timeouts++;  // increment counter
        }
    }
    while (!success && (tryCount < MAX_TRIES)); // repeat until succeed or reach limit

    free(frame);  // release the memory
    if (success)
    {
        seqNum = (seqNum + 1) % 256;  // increment sequence number for next time
        return 0;  // success at last!
    }
    else
    {
        if (debug) printf("LL: Block %d, tried %d times, failed\n", seqNum, MAX_TRIES);
        return -15;  // tried enough times, giving up
    }
}  // end of LL_send

/* Function to receive a frame and extract a block of data.
   Arguments:  array to hold data block,
               max size of data block.
   Return value is actual size of data block, or negative on error.
   If connected, tries to get frame from received bytes.
   If frame found, check if good frame - send ACK or NAK in reply.
   If good, extract data and return nunber of data bytes.
   If bad, keep trying until time limit expires.  */
int LL_receive(unsigned char *data, int maxBytes, bool debug)
{
    unsigned char *frame;  // pointer to frame array
    int nRx = 0;  // number of data bytes received
    int retVal = 0;  // return value - error code
    int expected = (lastSeq + 1) % 256; // sequence number expected
    bool success = false;  // flag for success
	//static int lastAck = T_NAK;  // to remember last response, for enquiry

    // First check if connected
    if (connected == false)
    {
        printf("LL: Attempt to receive while not connected\n");
        return -10;  // error code
    }

    // Create an array big enough to hold the frame
    // See notes on this in LL_send...
    frame = (unsigned char *) malloc(HDR_SIZE+maxBytes+TRL_SIZE);
    if (frame == NULL) // problem allocating memory
    {
        printf("LL: Failed to allocate memory for rx frame\n");
        return -19;  // return error code
    }

    timeSet(TIMER_RX, RX_WAIT);  // set time limit to wait for frame
    // Now try to receive a frame - repeat as necessary
    do  // receiving loop
    {
        // Get a frame, up to maximum size specified.
        // function returns  number of bytes in frame, or negative if error
        retVal = getFrame(frame, HDR_SIZE+maxBytes+TRL_SIZE, TIMER_RX);
        if (retVal < 0)  // some problem receiving
        {
            free(frame);  // release the memory
            return retVal;  // quit if fatal error
        }
        if (retVal > 0)  // we received something, so check it
        {
            if (debug) printf("LL: Got frame, %d bytes\n", retVal);
            if (checkFrame(frame, retVal)) // frame is good
            {
                goodFrames++;  // increment good frame counter

				/* If using enquiry frames, need to check frame type here.
				   This example assumes all incoming frames are data frames.
				   For an enquiry frame, re-send last ACK or NAK, but do not
				   report as success.  Maybe re-start the timer?  */

                if (frame[P_SEQN] == expected)  // if expected sequence number
                {
                    success = true;  // good data frame with expected seq. no.
					lastSeq = expected;  // update the sequence number
                    sendAck(T_ACK, lastSeq);  // send ACK for this block
					//lastAck = T_ACK;	// remember what we sent, in case of enquiry
                }
                else if (frame[P_SEQN] <= lastSeq)  // duplicate data block
                {
                    if (debug) printf("LL: Duplicate rx seq. %d, expected %d\n",
                                  frame[P_SEQN], expected);
                    sendAck(T_ACK, frame[P_SEQN]);  // send ACK for duplicate block
					//lastAck = T_ACK;	// remember what we sent, in case of enquiry
                    // Maybe re-start the timer here, to give sender more time?
                }
                else  // good frame out of sequence - should never happen?
                {
                    if (debug) printf("LL: Frame rx seq. %d, expected %d\n",
                                  frame[P_SEQN], expected);
                    sendAck(T_NAK, expected);  // send NAK for block expected
					//lastAck = T_NAK;	// remember what we sent, in case of enquiry
                }
            }
            else // bad frame
            {
                if (debug) printf("LL: Bad frame received\n");
                badFrames++;  // increment bad frame counter
                sendAck(T_NAK, expected);  // send NAK for block expected
				//lastAck = T_NAK;	// remember what we sent, in case of enquiry
            }
        }
        // else if received nothing, loop and try again
    }
    while (!success && timeOK(TIMER_RX)); // until frame found or time up

    if (success)  // we received a good frame with expected sequence number
    {
        nRx = frame[P_BCNT];  // extract byte count from frame header (if used)
        if (nRx > maxBytes) nRx = maxBytes;  // limit it just in case...
        /* Now extract the data bytes from the frame.  Here we just copy
           the bytes into the data array.  If you use byte stuffing, you
           need to remove the stuffed bytes.  If there is no byte count
           in the header, you need to continue until you find the end marker.*/
        for (int i = 0; i < nRx; i++)
        {
            data[i] = frame[HDR_SIZE + i];  // copy each byte
        }
        free(frame);  // release the memory
        return nRx;  // return size of block received
    }
    else  // we failed - no good frame received inside time limit
    {
        if (debug) printf("LL: Timeout waiting for block, last %d\n", lastSeq);
        free(frame);  // release the memory
        return -15;  // tried long enough, giving up
    }
}  // end of LL_receive


/* Function to extract a frame from the received bytes.
   Arguments: pointer to array of bytes to hold frame,
              maximum number of bytes to receive,
              timer number holding time limit.
   Return value is number of   bytes recovered, or negative if error. */
int getFrame(unsigned char *buffer, int maxSize, int timerNum)
{
    int nRx = 0;  // number of bytes received so far
    int byteCount = 0; // byte count from header (if used)
    int retVal = 0;  // return value from other functions

    // First search for the start of frame marker
    do
    {
        retVal = PHY_get(buffer, 1, PROB_ERR); // get one byte at a time
        // If there is an error, report it and give up
        if (retVal < 0) return retVal;  // check for error and give up
     }
    while (((retVal < 1) || (buffer[0] != C_SOH)) && timeOK(timerNum));
    // until we get a byte which is start of frame marker, or timeout

    // If we are out of time, return 0 - no useful bytes received
    if (timeUp(timerNum)) return 0;

    /* Two ways to proceed next.  If using byte count, get a few more bytes,
       just the rest of the header.  Then extract the byte count and get the
       rest of the frame (now that you can calculate how many bytes to get).
       Alternative, if using byte stuffing and end marker, is to get one byte at
       a time, until the end marker is found (without a stuff byte before it.
       Either way, take care not to receive more bytes than buffer can hold -
       check against maxSize.   */

    retVal = PHY_get(&buffer[1], 2, PROB_ERR);
    if (retVal < 0) return retVal;

    nRx = retVal+1;

    byteCount = buffer[2];

    retVal = PHY_get(&buffer[nRx], byteCount+2, PROB_ERR);
    if (retVal < 0) return retVal;

    nRx += retVal;




    return nRx;  // return number of bytes received
}  // end of getFrame

/* Function to check a received frame for errors.
   Arguments: pointer to array of bytes holding frame,
              number of bytes supposed to be in frame.
   As a minimum, should check checksum.  For extra safety, could
   also check byte count or end marker, whichever is used.
   Returns true if frame is good, false otherwise.   */
bool checkFrame(unsigned char *frame, int nByte)
{
    int checksum, i;
    for(i=0; i<nByte-2; i++)
    {
        checksum += frame[i];
    }
        checksum = checksum%256;
    if
    (
        frame[nByte-2]== checksum%256
    )



    // If all tests passed, return true
    return true;
    else{return false;}
}  // end of checkFrame

/* Function to send acknowledgement frame.
   Arguments: frame type, sequence number.
   Returns 0 on success, negative on error.  */
int sendAck(int type, int seq)
{


    unsigned char ackframe[ACK_SIZE];  // reply frame - size is known
    int checkSum = 0;  // checkSum
    int retVal = 0;  // return value from PHY function

    /* Put the header and trailer bytes into
       the ackFrame array, then call PHY_send to send it.  */


    ackframe[0] = C_SOH;
    ackframe[P_SEQN] = seq;
    ackframe[P_BCNT] = 0;
    checkSum = C_SOH+seq;
    ackframe[3] = checkSum%256;
    ackframe[4] = C_EOT;
    retVal = PHY_send(ackframe, ACK_SIZE);

    if(retVal < 0)
    {
        return -1;
    }



    return 0;  // success
}

/* Function to build a frame from a block of data.
   Arguments: array to hold frame,
              array of data,
              number of data bytes.
   Return value is number of bytes in the frame.  */
int buildFrame(unsigned char *frame, unsigned char *data, int nByte)
{
    /* Put the header bytes into the frame,
       then copy in the data bytes, adding stuff bytes if necessary.
       While handling all these bytes, calculate the
       checksum.  Then add the trailer bytes.
       Keep track of the total number of bytes in the frame, so
       LL_send knows how many bytes to transmit.  */

    //Put header bytes into frame.
    frame[0] = C_SOH;
    frame[P_SEQN] = seqNum;
    frame[P_BCNT] = nByte;

    for(int j = 0; j < nByte;j++)

        {
                frame[j+3]=data[j];
        }
    }//need to put some error checking here incase assignments dont work
    int checksum, i,nByte;
    for(i=0; i<nByte+3; i++)
    {
        checksum += frame[i];
    }
    frame[nByte+3] = checksum%256;

        //end of frame byte
    frame[nByte+4] = C_EOT;
    // Return the size of the frame
    return HDR_SIZE+nByte+TRL_SIZE;
}
