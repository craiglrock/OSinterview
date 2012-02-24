/**************************************************** 
 * mxutil.h - public interface for mxutil.c, based off of provided stubs
 * Note: As per assginment instructions, mandantory functions headers are
 * excluded. See assignment spec for details:
 * http://www.uoguelph.ca/~gardnerw/courses/cis2750/asmts-7.htm#124301432_pgfId-1426304
 * 
 * Programmed by Craig Lehmann, 0643962
 * last updated Feb 2nd 2012
 ****************************************************/

#ifndef MXUTIL_H
#define MXUTIL_H 1
#define _GNU_SOURCE 1

#include <stdio.h>
#include <libxml/xmlschemastypes.h>

// XmElem is a container for a generic XML element
typedef struct XmElem XmElem;	// lets us avoid coding "struct"
struct XmElem {    // fixed-length container for a generic XML element
    char *tag;			// <tag>
    char *text;			// any text between <tag> and </tag>, or NULL for <tag/>
    int isBlank;		// flag: 0 if text has some non-whitespace
    int nattribs;		// no. of attributes (can be 0)
    char *(*attrib)[][2];	// ->array of attributes (can be NULL)
				//   [0]->attribute; [1]->value
    unsigned long nsubs; 	// no. of subelements (can be 0)
    XmElem *(*subelem)[]; 	// ->array of subelements (can be NULL)
};


// public interface (copied from A1 spec), 
//No headers needed (Dr. Gardner's instructions) see spec for function info
xmlSchemaPtr mxInit( const char *xsdfile );
int mxReadFile( FILE *marcxmlfp, xmlSchemaPtr sp, XmElem **top );
int mxFindField( const XmElem *mrecp, int tag );
int mxFindSubfield( const XmElem *mrecp, int tag, int tnum, char sub );
const char *mxGetData( const XmElem *mrecp, int tag, int tnum, char sub, int snum );
void mxCleanElem( XmElem *top );
void mxTerm( xmlSchemaPtr sp );

// internal function (given here because the autotester will call it)
XmElem *mxMakeElem( xmlDocPtr doc, xmlNodePtr node );

int mxWriteFile( const XmElem *top, FILE *mxfile );

/*************************************************
Pre: top was successfully returned by mxReadFile and mxfile is open for writing
Post: The MarcXML contents of *top has been written to mxfile. Return # of records written
or -1 if error. (e.g. an elements tag or attribs are NULL when they shouldn't be.
**************************************************/
int printElement(const XmElem *top, FILE *mxfile, int depthOffset);

/*************************************************
same as strdup From here: http://cboard.cprogramming.com/c-programming/95462
-compiler-error-warning-implicit-declaration-function-strdup.html
Pre: str contains a string to be copied
Post: returns a pointer to a new string which is a duplicate of str 
*************************************************/
char *customCopy(const char *str);

#endif
