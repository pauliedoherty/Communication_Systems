#ifndef TIMEOUT_H_INCLUDED
#define TIMEOUT_H_INCLUDED

/* Provides functions to set or check time limits.
   Up to 10 timers can operate at the same time.
   Resolution is system-dependent, typically 1 ms.  */

// Function to set time limit at a point in the future
bool timeSet (int num, float lim);
/* num    is timer number to set, range 0 to 9
   lim    is time limit in seconds (from now)
   returns true if succeeds, false if fails,
           reason for failure is num out of range */

// Function to check if time limit has elapsed
bool timeUp (int num);
/* num    is timer number to check, range 0 to 9
   returns true if timer has reached or exceeded limit,
           false if timer has not yet reached limit,
           also false if invalid timer.  */

// Function to check if time still within limit
bool timeOK (int num);
/* num    is timer number to check, range 0 to 9
   returns true if timer has not yet reached limit,
           false if timer has reached or exceeded limit,
           also false if invalid timer.  */

#endif // TIMEOUT_H_INCLUDED
