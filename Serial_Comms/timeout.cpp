/* Provides functions to set or check time limits.
   Up to 10 timers can operate at the same time.
   Resolution is system-dependent, typically 1 ms.  */

#include <time.h>
#include "timeout.h"

// Create a shared but private array to hold time limits.
// Limits are stored in clock ticks, in array of type clock_t.
// This type is defined in time.h.
static clock_t timers[10] = {0};

/* Function to set time limit at a point in the future.
   num    is timer number to set, range 0 to 9
   lim    is time limit in seconds (from now)
   returns true if succeeds, false if fails,
           reason for failure is num out of range. */
bool timeSet(int num, float lim)
{
    if ((num<0) || (num>9))  // invalid timer
    {
        return false;
    }
    else                    // calculate and save limit
    {
        timers[num] = clock() + (clock_t)(lim * CLOCKS_PER_SEC);
        return true;
    }
}

/* Function to check if time limit has elapsed.
   num    is timer number to check, range 0 to 9
   returns true if timer has reached or exceeded limit,
           false if timer has not yet reached limit,
           also false if invalid timer.   */
bool timeUp(int num)
{
    if ((num<0) || (num>9))  // invalid timer
    {
        return false;
    }
    else if (clock() < timers[num]) // still within limit
    {
        return false;
    }
    else // time limit has been reached or exceeded
    {
        return true;
    }
}

/* Function to check if time still within limit
   num    is timer number to check, range 0 to 9
   returns true if timer has not yet reached limit,
           false if timer has reached or exceeded limit,
           also false if invalid timer.  */
bool timeOK(int num)
{
    if ((num<0) || (num>9))  // invalid timer
    {
        return false;
    }
    else if (clock() < timers[num]) // still within limit
    {
        return true;
    }
    else // time limit has been reached or exceeded
    {
        return false;
    }
}

