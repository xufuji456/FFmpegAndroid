/* libSoX SGI/Amiga AIFF format.
 * Copyright 1991-2007 Guido van Rossum And Sundry Contributors
 *
 * This source code is freely redistributable and may be used for
 * any purpose.  This copyright notice must be maintained.
 * Guido van Rossum And Sundry Contributors are not responsible for
 * the consequences of using this software.
 *
 * Used by SGI on 4D/35 and Indigo.
 * This is a subformat of the EA-IFF-85 format.
 * This is related to the IFF format used by the Amiga.
 * But, apparently, not the same.
 * Also AIFF-C format output that is defined in DAVIC 1.4 Part 9 Annex B
 * (usable for japanese-data-broadcasting, specified by ARIB STD-B24.)
 */

int lsx_aiffstartread(sox_format_t * ft);
int lsx_aiffstopread(sox_format_t * ft);
int lsx_aiffstartwrite(sox_format_t * ft);
int lsx_aiffstopwrite(sox_format_t * ft);
int lsx_aifcstartwrite(sox_format_t * ft);
int lsx_aifcstopwrite(sox_format_t * ft);
