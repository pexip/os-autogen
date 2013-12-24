
/**
 * @file loadPseudo.c
 *
 *  Find the start and end macro markers.  In btween we must find the
 *  "autogen" and "template" keywords, followed by any suffix specs.
 *
 *  Time-stamp:        "2011-06-25 09:11:38 bkorb"
 *
 *  This module processes the "pseudo" macro
 *
 *  This file is part of AutoGen.
 *  AutoGen Copyright (c) 1992-2011 by Bruce Korb - all rights reserved
 *
 * AutoGen is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * AutoGen is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define DEFINE_FSM
#include "pseudo-fsm.h"

static char const zAgName[] = "autogen5";
static char const zTpName[] = "template";

/* = = = START-STATIC-FORWARD = = = */
static char const *
do_scheme_expr(char const * pzData, char const * pzFileName);

static char const *
handle_hash_line(char const * pz);

static te_pm_event
next_pm_token(char const ** ppzData, te_pm_state fsm_state, char const * fnm);

static char const *
copy_mark(char const * pzData, char* pzMark, size_t * pCt);
/* = = = END-STATIC-FORWARD = = = */


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *  do_scheme_expr
 *
 *  Process a scheme specification
 */
static char const *
do_scheme_expr(char const * pzData, char const * pzFileName)
{
    char*   pzEnd = (char*)pzData + strlen(pzData);
    char    ch;
    tMacro* pCM = pCurMacro;
    tMacro  mac = { (teFuncType)~0, 0, 0, 0, 0, 0, 0, NULL };

    mac.lineNo  = templLineNo;
    pzEnd       = (char*)skipScheme(pzData, pzEnd);
    ch          = *pzEnd;
    *pzEnd      = NUL;
    pCurMacro   = &mac;

    ag_scm_c_eval_string_from_file_line(
          (char*)pzData, pzFileName, templLineNo );

    pCurMacro = pCM;
    *pzEnd    = ch;
    while (pzData < pzEnd)
        if (*(pzData++) == NL)
            templLineNo++;
    return (char const *)pzEnd;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *  do_suffix
 *
 *  Process a suffix specification
 */
LOCAL char const *
do_suffix(char const * const pzData, char const * pzFileName, int lineNo)
{
    /*
     *  The following is the complete list of POSIX required-to-be-legal
     *  file name characters.  These are the only characters we allow to
     *  appear in a suffix.  We do, however, add '=' and '%' because we
     *  also allow a format specification to follow the suffix,
     *  separated by an '=' character.
     */
    static char const zEmptySpec[] = "Empty suffix format";

    tOutSpec*  pOS;
    char const *       pzSfxFmt;
    char const *       pzResult;
    size_t     spn;
    static tOutSpec**  ppOSList = &pOutSpecList;

    /*
     *  Skip over the suffix construct
     */
    pzSfxFmt = pzData;
    while (IS_SUFFIX_CHAR(*pzSfxFmt))  pzSfxFmt++;

    if (*pzSfxFmt != '=') {
        pzResult = pzSfxFmt;
        pzSfxFmt = NULL;

    } else {
        pzSfxFmt++;

        if (*pzSfxFmt == '(') {
            char const *pe  = pzSfxFmt + strlen(pzSfxFmt);
            pzResult = skipScheme(pzSfxFmt, pe);

        } else {
            pzResult = pzSfxFmt;
            while (IS_SUFFIX_FMT_CHAR(*pzResult)) pzResult++;

            if (pzSfxFmt == pzResult)
                AG_ABEND(zEmptySpec);
        }
    }

    /*
     *  If pzFileName is NULL, then we are called by --select-suffix.
     *  Otherwise, the suffix construct is saved only for the main template,
     *  and only when the --select-suffix option was not specified.
     */
    if (  (pzFileName != NULL)
       && (  (procState != PROC_STATE_LOAD_TPL)
          || HAVE_OPT(SELECT_SUFFIX)))
        return pzResult;

    /*
     *  Allocate Output Spec and link into the global list.  Copy all the
     *  "spanned" text, including any '=' character, scheme expression or
     *  file name format string.
     */
    spn = pzResult - pzData;
    {
        size_t sz = sizeof(*pOS) + spn + 1;
        pOS = AGALOC(sz, "Output Specification");
        memset(pOS, NUL, sz);
    }
    *ppOSList  = pOS;
    ppOSList   = &pOS->pNext;
    pOS->pNext = NULL;
    memcpy(pOS->zSuffix, pzData, spn);
    pOS->zSuffix[spn] = NUL;

    /*
     *  IF the suffix contains its own formatting construct,
     *  THEN split it off from the suffix and set the formatting ptr.
     *  ELSE supply a default.
     */
    if (pzSfxFmt != NULL) {
        size_t sfx_len = pzSfxFmt - pzData;
        pOS->zSuffix[sfx_len-1] = NUL;
        pOS->pzFileFmt = pOS->zSuffix + sfx_len;

        if (*pOS->pzFileFmt == '(') {
            SCM str =
                ag_scm_c_eval_string_from_file_line(
                    pOS->pzFileFmt, pzFileName, lineNo );
            size_t str_length;
            char const * pz;

            pzSfxFmt = pz = resolveSCM(str);
            str_length = strlen(pzSfxFmt);

            if (str_length == 0)
                AG_ABEND(zEmptySpec);
            while (IS_SUFFIX_FMT_CHAR(*pz))  pz++;

            if ((pz - pzSfxFmt) != str_length)
                AG_ABEND(aprf("invalid chars in suffix format:  %s", pz));

            /*
             *  IF the scheme replacement text fits in the space, don't
             *  mess with allocating another string.
             */
            if (str_length < spn - sfx_len)
                strcpy(pOS->zSuffix + sfx_len, pzSfxFmt);
            else {
                AGDUPSTR(pOS->pzFileFmt, pzSfxFmt, "suffix format");
                pOS->deallocFmt = AG_TRUE;
            }
        }

    } else {
        /*
         *  IF the suffix does not start with punctuation,
         *  THEN we will insert a '.' of our own.
         */
        if (IS_VAR_FIRST_CHAR(pOS->zSuffix[0]))
             pOS->pzFileFmt = zFileFormat + 5;
        else pOS->pzFileFmt = zFileFormat;
    }

    return pzResult;
}

static char const *
handle_hash_line(char const * pz)
{
    char const * res = strchr(pz, NL);
    if (res == NULL)
        AG_ABEND("Invalid template file");

    /*
     *  If the comment starts with "#!/", then see if it names
     *  an executable.  If it does, it is specifying a shell to use.
     */
    if ((pz[1] == '!') && (pz[2] == '/')) {
        char const * pzScn = pz + 3;
        char * nmbuf;
        size_t len;

        while (IS_FILE_NAME_CHAR(*pzScn))   pzScn++;

        len   = pzScn - (pz + 2);
        nmbuf = ag_scribble(len);
        memcpy(nmbuf, pz+2, len);
        nmbuf[len] = NUL;

        /*
         *  If we find the executable, then change the configured shell and
         *  the SHELL environment variable to this executable.
         */
        if (access(nmbuf, X_OK) == 0) {
            static char const sh_nm[] = "SHELL";
            char * sp = malloc(sizeof(sh_nm) + 1 + len);
            sprintf(sp, "%s=%s", sh_nm, nmbuf);
            putenv(sp);
            AGDUPSTR(pzShellProgram, nmbuf, "shell name");
            AGDUPSTR(serverArgs[0],  nmbuf, "shell name");
        }
    }

    return res;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *  next_pm_token
 *
 *  Skiping leading white space, figure out what sort of token is under
 *  the scan pointer (pzData).
 */
static te_pm_event
next_pm_token(char const ** ppzData, te_pm_state fsm_state, char const * fnm)
{
    char const * pzData = *ppzData;

    /*
     *  At the start of processing in this function, we can never be at
     *  the "start of a line".  A '#' type comment before the initial
     *  start macro marker is illegal.  Otherwise, our scan pointer is
     *  after some valid token, which won't be the start of a line, either.
     */
    ag_bool line_start = AG_FALSE;

 skipWhiteSpace:
    while (IS_WHITESPACE_CHAR(*pzData)) {
        if (*(pzData++) == NL) {
            line_start = AG_TRUE;
            templLineNo++;

            /*
             *  IF we are done with the macro markers,
             *  THEN we skip white space only thru the first new line.
             */
            if (fsm_state == PM_ST_END_MARK) {
                *ppzData = pzData;
                return PM_EV_END_PSEUDO;
            }
        }
    }

    if (line_start && (*pzData == '#')) {
        pzData = handle_hash_line(pzData);
        goto skipWhiteSpace;
    }

    *ppzData = pzData; /* in case we return */

    /*
     *  After the end marker has been found,
     *  anything else is really the start of the data.
     */
    if (fsm_state == PM_ST_END_MARK)
        return PM_EV_END_PSEUDO;

    /*
     *  IF the token starts with an alphanumeric,
     *  THEN it must be "autogen5" or "template" or a suffix specification
     */
    if (IS_VAR_FIRST_CHAR(*pzData)) {
        if (strneqvcmp(pzData, zAgName, (int)sizeof(zAgName) - 1) == 0) {
            if (IS_WHITESPACE_CHAR(pzData[ sizeof(zAgName) - 1 ])) {
                *ppzData = pzData + sizeof(zAgName);
                return PM_EV_AUTOGEN;
            }

            return PM_EV_SUFFIX;
        }

        if (  (strneqvcmp(pzData, zTpName, (int)sizeof(zTpName)-1) == 0)
           && (IS_WHITESPACE_CHAR(pzData[ sizeof(zTpName)-1 ])) ) {
            *ppzData = pzData + sizeof(zTpName)-1;
            return PM_EV_TEMPLATE;
        }

        return PM_EV_SUFFIX;
    }

    /*
     *  Handle emacs mode markers and scheme expressions only once we've
     *  gotten past "init" state.
     */
    if (fsm_state > PM_ST_INIT)
        switch (*pzData) {
        case '-':
            if ((pzData[1] == '*') && (pzData[2] == '-'))
                return PM_EV_ED_MODE;
            break;

        case '(':
            return PM_EV_SCHEME;
        }

    /*
     *  Alphanumerics and underscore are already handled.  Thus, it must be
     *  a punctuation character that may introduce a suffix:  '.' '-' '_'
     */
    if (IS_SUFFIX_CHAR(*pzData))
        return PM_EV_SUFFIX;

    /*
     *  IF it is some other punctuation,
     *  THEN it must be a start/end marker.
     */
    if (IS_PUNCTUATION_CHAR(*pzData))
        return PM_EV_MARKER;

    /*
     *  Otherwise, it is just junk.
     */
    AG_ABEND(aprf("Invalid template file: %s", fnm));
    /* NOTREACHED */
    return PM_EV_INVALID;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/**
 *  Some sort of marker is under the scan pointer.  Copy it for as long
 *  as we find punctuation characters.
 */
static char const *
copy_mark(char const * pzData, char* pzMark, size_t * pCt)
{
    int ct = 0;

    for (;;) {
        char ch = *pzData;
        if (! IS_PUNCTUATION_CHAR(ch))
            break;
        *(pzMark++) = ch;
        if (++ct >= sizeof(zStartMac))
            return NULL;

        pzData++;
    }

    *pCt = ct;
    *pzMark = NUL;

    if (OPT_VALUE_TRACE >= TRACE_EXPRESSIONS)
        fprintf(pfTrace, "marker ``%s'' loaded\n", pzMark - ct);

    return pzData;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/**
 *  Using a finite state machine, scan over the tokens that make up the
 *  "pseudo macro" at the start of every template.
 */
LOCAL char const *
loadPseudoMacro(char const * pzData, char const * pzFileName)
{
    static char const zMarkErr[] =
        "start/end macro mark too long";
    static char const zBadMark[] =
        "bad template marker in %s on line %d:\n\t%s";

    char const * pzBadness;
#   define BAD_MARKER(t) { pzBadness = t; goto abort_load; }

    te_pm_state fsm_state  = PM_ST_INIT;

    templLineNo  = 1;

    while (fsm_state != PM_ST_DONE) {
        te_pm_event fsm_tkn = next_pm_token(&pzData, fsm_state, pzFileName);
        te_pm_state nxt_state;
        te_pm_trans trans;

        nxt_state  = pm_trans_table[ fsm_state ][ fsm_tkn ].next_state;
        trans      = pm_trans_table[ fsm_state ][ fsm_tkn ].transition;

        /*
         *  There are only so many "PM_TR_<state-name>_<token-name>"
         *  transitions that are legal.  See which one we got.
         *  It is legal to alter "nxt_state" while processing these.
         */
        switch (trans) {
        case PM_TR_SKIP_ED_MODE:
        {
            char* pzEnd = strstr(pzData + 3, "-*-");
            char* pzNL  = strchr(pzData + 3, NL);
            if ((pzEnd == NULL) || (pzNL < pzEnd))
                BAD_MARKER("invalid edit mode marker");

            pzData = pzEnd + 3;
            break;
        }

        case PM_TR_INIT_MARKER:
            pzData = copy_mark(pzData, zStartMac, &startMacLen);
            if (pzData == NULL)
                BAD_MARKER(zMarkErr);

            break;

        case PM_TR_TEMPL_MARKER:
            pzData = copy_mark(pzData, zEndMac, &endMacLen);
            if (pzData == NULL)
                BAD_MARKER(zMarkErr);

            /*
             *  IF the end macro seems to end with the start macro and
             *  it is exactly twice as long as the start macro, then
             *  presume that someone ran the two markers together.
             */
            if (  (endMacLen == 2 * startMacLen)
               && (strcmp(zStartMac, zEndMac + startMacLen) == 0))  {
                pzData -= startMacLen;
                zEndMac[ startMacLen ] = NUL;
                endMacLen = startMacLen;
            }

            if (strstr(zEndMac, zStartMac) != NULL)
                BAD_MARKER("start marker contained in end marker");
            if (strstr(zStartMac, zEndMac) != NULL)
                BAD_MARKER("end marker contained in start marker");
            break;

        case PM_TR_TEMPL_SUFFIX:
            pzData = do_suffix(pzData, pzFileName, templLineNo);
            break;

        case PM_TR_TEMPL_SCHEME:
            pzData = do_scheme_expr(pzData, pzFileName);
            break;

        case PM_TR_INVALID:
            pm_invalid_transition(fsm_state, fsm_tkn);
            switch (fsm_state) {
            case PM_ST_INIT:     BAD_MARKER("need start marker");
            case PM_ST_ST_MARK:  BAD_MARKER("need autogen5 marker");
            case PM_ST_AGEN:     BAD_MARKER("need template marker");
            case PM_ST_TEMPL:    BAD_MARKER("need end marker");
            case PM_ST_END_MARK: BAD_MARKER("need end of line");
            default:             BAD_MARKER("BROKEN FSM STATE");
            }

        case PM_TR_NOOP:
            break;

        default:
            BAD_MARKER("broken pseudo-macro FSM");
        }

        fsm_state = nxt_state;
    }

    return pzData;

 abort_load:
    AG_ABEND(aprf(zBadMark, pzFileName, templLineNo, pzBadness));
#   undef BAD_MARKER
    return NULL;
}
/*
 * Local Variables:
 * mode: C
 * c-file-style: "stroustrup"
 * indent-tabs-mode: nil
 * End:
 * end of agen5/loadPseudo.c */
