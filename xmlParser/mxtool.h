/**************************************************** 
 * mxtool.h - public interface for mxtool.c. Uses for Assignment 2 autotester.
 * Note: Nothing added/altered to this for assignment 2
 *
 * Programmed by Craig Lehmann, 0643962
 * last updated by professor: Jan 30, 2012
 ****************************************************/

#ifndef MXTOOL_H_
#define MXTOOL_H_ 1

#include "mxutil.h"


/* command execution functions

   Return values: EXIT_SUCCESS or EXIT_FAILURE (stdlib.h) */

int review( const XmElem *top, FILE *outfile );

int concat( const XmElem *top1, const XmElem *top2, FILE *outfile );

enum SELECTOR { KEEP, DISCARD };
int selects( const XmElem *top, const enum SELECTOR sel, const char *pattern, FILE *outfile );

int libFormat( const XmElem *top, FILE *outfile );

int bibFormat( const XmElem *top, FILE *outfile );


/* helper functions (will be tested individually) */

int match( const char *data, const char *regex );

typedef char *BibData[4];
enum BIBFIELD { AUTHOR=0, TITLE, PUBINFO, CALLNUM };
void marc2bib( const XmElem *mrec, BibData bdata );

void sortRecs( XmElem *collection, const char *keys[] );

#endif