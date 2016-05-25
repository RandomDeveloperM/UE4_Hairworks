/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


/*
** prrng.h -- NSPR Random Number Generator
** 
**
** lth. 29-Oct-1999.
*/

#ifndef prrng_h___ 
#define prrng_h___

#include "prtypes.h"

PR_BEGIN_EXTERN_C

/*
** PR_GetRandomNoise() -- Get random noise from the host platform
**
** Description:
** PR_GetRandomNoise() provides, depending on platform, a random value.
** The length of the random value is dependent on platform and the
** platform's ability to provide a random value at that moment.
**
** The intent of PR_GetRandomNoise() is to provide a "seed" value for a
** another random number generator that may be suitable for
** cryptographic operations. This implies that the random value
** provided may not be, by itself, cryptographically secure. The value
** generated by PR_GetRandomNoise() is at best, extremely difficult to
** predict and is as non-deterministic as the underlying platfrom can
** provide.
**
** Inputs:
**   buf -- pointer to a caller supplied buffer to contain the
**          generated random number. buf must be at least as large as
**          is specified in the 'size' argument.
**
**   size -- the requested size of the generated random number
**
** Outputs:
**   a random number provided in 'buf'.
**
** Returns:
**   PRSize value equal to the size of the random number actually
**   generated, or zero. The generated size may be less than the size
**   requested. A return value of zero means that PR_GetRandomNoise() is
**   not implemented on this platform, or there is no available noise
**   available to be returned at the time of the call.
**
** Restrictions:
**   Calls to PR_GetRandomNoise() may use a lot of CPU on some platforms.
**   Some platforms may block for up to a few seconds while they
**   accumulate some noise. Busy machines generate lots of noise, but
**   care is advised when using PR_GetRandomNoise() frequently in your
**   application.
**
** History:
**   Parts of the model dependent implementation for PR_GetRandomNoise()
**   were taken in whole or part from code previously in Netscape's NSS
**   component.
**
*/
NSPR_API(PRSize) PR_GetRandomNoise( 
    void    *buf,
    PRSize  size
);

PR_END_EXTERN_C

#endif /* prrng_h___ */
/* end prrng.h */
