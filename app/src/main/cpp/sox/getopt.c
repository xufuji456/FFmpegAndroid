/* lsx_getopt for SoX
 *
 * (c) 2011 Doug Cook and SoX contributors
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "sox.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

void
lsx_getopt_init(
    LSX_PARAM_IN             int argc,                      /* Number of arguments in argv */
    LSX_PARAM_IN_COUNT(argc) char * const * argv,           /* Array of arguments */
    LSX_PARAM_IN_Z           char const * shortopts,        /* Short option characters */
    LSX_PARAM_IN_OPT         lsx_option_t const * longopts, /* Array of long option descriptors */
    LSX_PARAM_IN             lsx_getopt_flags_t flags,      /* Flags for longonly and opterr */
    LSX_PARAM_IN             int first,                     /* First argument to check (usually 1) */
    LSX_PARAM_OUT            lsx_getopt_t * state)          /* State object to initialize */
{
    assert(argc >= 0);
    assert(argv != NULL);
    assert(shortopts);
    assert(first >= 0);
    assert(first <= argc);
    assert(state);
    if (state)
    {
        if (argc < 0 ||
            !argv ||
            !shortopts ||
            first < 0 ||
            first > argc)
        {
            memset(state, 0, sizeof(*state));
        }
        else
        {
            state->argc = argc;
            state->argv = argv;
            state->shortopts =
                (shortopts[0] == '+' || shortopts[0] == '-') /* Requesting GNU special behavior? */
                ? shortopts + 1 /* Ignore request. */
                : shortopts; /* No special behavior requested. */
            state->longopts = longopts;
            state->flags = flags;
            state->curpos = NULL;
            state->ind = first;
            state->opt = '?';
            state->arg = NULL;
            state->lngind = -1;
        }
    }
}

static void CheckCurPosEnd(
    LSX_PARAM_INOUT lsx_getopt_t * state)
{
    if (!state->curpos[0])
    {
        state->curpos = NULL;
        state->ind++;
    }
}

int
lsx_getopt(
    LSX_PARAM_INOUT lsx_getopt_t * state)
{
    int oerr;
    assert(state);
    if (!state)
    {
        lsx_fail("lsx_getopt called with state=NULL");
        return -1;
    }

    assert(state->argc >= 0);
    assert(state->argv != NULL);
    assert(state->shortopts);
    assert(state->ind >= 0);
    assert(state->ind <= state->argc + 1);

    oerr = 0 != (state->flags & lsx_getopt_flag_opterr);
    state->opt = 0;
    state->arg = NULL;
    state->lngind = -1;

    if (state->argc < 0 ||
        !state->argv ||
        !state->shortopts ||
        state->ind < 0)
    { /* programmer error */
        lsx_fail("lsx_getopt called with invalid information");
        state->curpos = NULL;
        return -1;
    }
    else if (
        state->argc <= state->ind ||
        !state->argv[state->ind] ||
        state->argv[state->ind][0] != '-' ||
        state->argv[state->ind][1] == '\0')
    { /* return no more options */
        state->curpos = NULL;
        return -1;
    }
    else if (state->argv[state->ind][1] == '-' && state->argv[state->ind][2] == '\0')
    { /* skip "--", return no more options. */
        state->curpos = NULL;
        state->ind++;
        return -1;
    }
    else
    { /* Look for the next option */
        char const * current = state->argv[state->ind];
        char const * param = current + 1;

        if (state->curpos == NULL ||
            state->curpos <= param ||
            param + strlen(param) <= state->curpos)
        { /* Start parsing a new parameter - check for a long option */
            state->curpos = NULL;

            if (state->longopts &&
                (param[0] == '-' || (state->flags & lsx_getopt_flag_longonly)))
            {
                size_t nameLen;
                int doubleDash = param[0] == '-';
                if (doubleDash)
                {
                    param++;
                }

                for (nameLen = 0; param[nameLen] && param[nameLen] != '='; nameLen++)
                {}

                /* For single-dash, you have to specify at least two letters in the name. */
                if (doubleDash || nameLen >= 2)
                {
                    lsx_option_t const * pCur;
                    lsx_option_t const * pMatch = NULL;
                    int matches = 0;

                    for (pCur = state->longopts; pCur->name; pCur++)
                    {
                        if (0 == strncmp(pCur->name, param, nameLen))
                        { /* Prefix match. */
                            matches++;
                            pMatch = pCur;
                            if (nameLen == strlen(pCur->name))
                            { /* Exact match - no ambiguity, stop search. */
                                matches = 1;
                                break;
                            }
                        }
                    }

                    if (matches == 1)
                    { /* Matched. */
                        state->ind++;

                        if (param[nameLen])
                        { /* --name=value */
                            if (pMatch->has_arg)
                            { /* Required or optional arg - done. */
                                state->arg = param + nameLen + 1;
                            }
                            else
                            { /* No arg expected. */
                                if (oerr)
                                {
                                    lsx_warn("`%s' did not expect an argument from `%s'",
                                        pMatch->name,
                                        current);
                                }
                                return '?';
                            }
                        }
                        else if (pMatch->has_arg == lsx_option_arg_required)
                        { /* Arg required. */
                            state->arg = state->argv[state->ind];
                            state->ind++;
                            if (state->ind > state->argc)
                            {
                                if (oerr)
                                {
                                    lsx_warn("`%s' requires an argument from `%s'",
                                        pMatch->name,
                                        current);
                                }
                                return state->shortopts[0] == ':' ? ':' : '?'; /* Missing required value. */
                            }
                        }

                        state->lngind = pMatch - state->longopts;
                        if (pMatch->flag)
                        {
                            *pMatch->flag = pMatch->val;
                            return 0;
                        }
                        else
                        {
                            return pMatch->val;
                        }
                    }
                    else if (matches == 0 && doubleDash)
                    { /* No match */
                        if (oerr)
                        {
                            lsx_warn("parameter not recognized from `%s'", current);
                        }
                        state->ind++;
                        return '?';
                    }
                    else if (matches > 1)
                    { /* Ambiguous. */
                        if (oerr)
                        {
                            lsx_warn("parameter `%s' is ambiguous:", current);
                            for (pCur = state->longopts; pCur->name; pCur++)
                            {
                                if (0 == strncmp(pCur->name, param, nameLen))
                                {
                                    lsx_warn("parameter `%s' could be `--%s'", current, pCur->name);
                                }
                            }
                        }
                        state->ind++;
                        return '?';
                    }
                }
            }

            state->curpos = param;
        }

        state->opt = state->curpos[0];
        if (state->opt == ':')
        { /* ':' is never a valid short option character */
            if (oerr)
            {
                lsx_warn("option `%c' not recognized", state->opt);
            }
            state->curpos++;
            CheckCurPosEnd(state);
            return '?'; /* unrecognized option */
        }
        else
        { /* Short option needs to be matched from option list */
            char const * pShortopt = strchr(state->shortopts, state->opt);
            state->curpos++;

            if (!pShortopt)
            { /* unrecognized option */
                if (oerr)
                {
                    lsx_warn("option `%c' not recognized", state->opt);
                }
                CheckCurPosEnd(state);
                return '?';
            }
            else if (pShortopt[1] == ':' && state->curpos[0])
            { /* Return the rest of the parameter as the option's value */
                state->arg = state->curpos;
                state->curpos = NULL;
                state->ind++;
                return state->opt;
            }
            else if (pShortopt[1] == ':' && pShortopt[2] != ':')
            { /* Option requires a value */
                state->curpos = NULL;
                state->ind++;
                state->arg = state->argv[state->ind];
                state->ind++;
                if (state->ind <= state->argc)
                { /* A value was present, so we're good. */
                    return state->opt;
                }
                else
                {  /* Missing required value. */
                    if (oerr)
                    {
                        lsx_warn("option `%c' requires an argument",
                            state->opt);
                    }
                    return state->shortopts[0] == ':' ? ':' : '?';
                }
            }
            else
            { /* Option without a value. */
                CheckCurPosEnd(state);
                return state->opt;
            }
        }
    }
}

#ifdef TEST_GETOPT

#include <stdio.h>

int main(int argc, char const * argv[])
{
    static int help = 0;
    static lsx_option_t longopts[] =
    {
        {"a11",  0, 0, 101},
        {"a12",  0, 0, 102},
        {"a122", 0, 0, 103},
        {"rarg", 1, 0, 104},
        {"oarg", 2, 0, 105},
        {"help", 0, &help, 106},
        {0}
    };

    int ch;
    lsx_getopt_t state;
    lsx_getopt_init(argc, argv, "abc:d:v::0123456789", longopts, sox_true, 1, &state);

    while (-1 != (ch = lsx_getopt(&state)))
    {
        printf(
            "H=%d ch=%d, ind=%d opt=%d lng=%d arg=%s\n",
            help,
            ch,
            state.ind,
            state.opt,
            state.lngind,
            state.arg ? state.arg : "NULL");
    }

    return 0;
}

#endif /* TEST_GETOPT */
