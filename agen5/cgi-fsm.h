/*   -*- buffer-read-only: t -*- vi: set ro:
 *
 *  DO NOT EDIT THIS FILE   (cgi-fsm.h)
 *
 *  It has been AutoGen-ed
 *  From the definitions    cgi.def
 *  and the template file   fsm
 *
 *  Automated Finite State Machine
 *
 *  Copyright (C) 1992-2016 Bruce Korb - all rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name ``Bruce Korb'' nor the name of any other
 *    contributor may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * AutoFSM IS PROVIDED BY Bruce Korb ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bruce Korb OR ANY OTHER CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 *  This file enumerates the states and transition events for a FSM.
 *
 *  te_cgi_state
 *      The available states.  FSS_INIT is always defined to be zero
 *      and FSS_INVALID and FSS_DONE are always made the last entries.
 *
 *  te_cgi_event
 *      The transition events.  These enumerate the event values used
 *      to select the next state from the current state.
 *      CGI_EV_INVALID is always defined at the end.
 */
#ifndef AUTOFSM_CGI_FSM_H_GUARD
#define AUTOFSM_CGI_FSM_H_GUARD 1
/**
 *  Finite State machine States
 *
 *  Count of non-terminal states.  The generated states INVALID and DONE
 *  are terminal, but INIT is not  :-).
 */
#define CGI_STATE_CT  3
typedef enum {
    CGI_ST_INIT,    CGI_ST_NAME,    CGI_ST_VALUE,   CGI_ST_INVALID,
    CGI_ST_DONE
} te_cgi_state;

/**
 *  Finite State machine transition Events.
 *
 *  Count of the valid transition events
 */
#define CGI_EVENT_CT 8
typedef enum {
    CGI_EV_ALPHA,     CGI_EV_NAME_CHAR, CGI_EV_EQUAL,     CGI_EV_SPACE,
    CGI_EV_ESCAPE,    CGI_EV_OTHER,     CGI_EV_SEPARATOR, CGI_EV_END,
    CGI_EV_INVALID
} te_cgi_event;

/**
 *  Run the FSM.  Will return CGI_ST_DONE or CGI_ST_INVALID
 */
extern te_cgi_state
cgi_run_fsm(
    char const * pzSrc,
    int inlen,
    char * pzOut,
    int outlen );

#endif /* AUTOFSM_CGI_FSM_H_GUARD */
/*
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * indent-tabs-mode: nil
 * End:
 * end of cgi-fsm.h */
