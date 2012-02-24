/**************************************************** 
 * mxutil.c - small library of functions to make it easy for an application
 * to extract MARCXML data from a MARCXML file. Uses libxml2 for low-level
 * parsing of xml data. 
 *
 * Programmed by Craig Lehmann, 0643962
 * last updated Feb 2nd 2012
 ****************************************************/

/*define the following to avoid fileno implicit declaration error 
as discussed here: http://moodle.socs.uoguelph.ca/mod/forum/discuss.php?d=4412 */
#define _POSIX_SOURCE 1

#include "mxutil.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

xmlSchemaPtr mxInit( const char *xsdfile ){
  
  /*set and return the previous value for enabling line numbers in 
  elements contents. Returns last value,0 for no sub or 1 for sub.*/
  LIBXML_TEST_VERSION
  xmlLineNumbersDefault(1);
  
  /*creates an xml schemas parse context for that file/resourse expected to 
  contain an XML Schemas file. Return: parser contents or Null in case of 
  error. */
  xmlSchemaParserCtxtPtr ParserContentPtr = xmlSchemaNewParserCtxt ( xsdfile );
  if (ParserContentPtr == NULL) return NULL;
  
  /*parse a schema definition resourse (from second hint function) and build an 
  internal XML Shema structure which can be used to validate instances. Return:
  the internal XML Schema Structure built from the resource or NULL in case of 
  error*/
  xmlSchemaPtr schemaPtr = xmlSchemaParse (ParserContentPtr);
  
  /*Fourth helper function
  free the resources associated to the schema parser context */
  xmlSchemaFreeParserCtxt (ParserContentPtr);

 return (schemaPtr);
}

void mxTerm( xmlSchemaPtr sp ){
  /*Recommended Func: deallocates a schema structure*/
  if ( sp == NULL) return;
  xmlSchemaFree (sp);  
  
  /*recommened function: Cleanup the default XML Schemas type library
  no return */
  xmlSchemaCleanupTypes ();
  
  /*Recommended function: A cleanup library of the xml library, It tries to 
  reclaim all of the related global memory allocated for the library processing 
  see http://xmlsoft.org/html/libxml-parser.html#xmlCleanupParser details*/
  xmlCleanupParser ();
}

int mxReadFile( FILE *marcxmlfp, xmlSchemaPtr sp, XmElem **top ){
  
  /*Recommend func: parse an XML file from a file descriptor and build a tree 
  **Note file descriptor will not be closed by function. Returns
  resulting document tree or NULL if failure*/
  xmlDocPtr xmlTree = xmlReadFd(fileno(marcxmlfp),"",NULL,0);
  if (xmlTree == NULL){
    return 1;//failed to parse xml file
  }
  
  /*Recommended func: create an xml schemas validation context based on the 
  given schema, return NULL if error or validation context */
  xmlSchemaValidCtxtPtr validSchemaPtr = xmlSchemaNewValidCtxt (sp);
  int isvalid = xmlSchemaValidateDoc ( validSchemaPtr, xmlTree );
  
  if (validSchemaPtr == NULL || isvalid != 0){
  	if (xmlTree != NULL) xmlFreeDoc (xmlTree);
    return 2; //xml did not match schema
  }
  /*Recommended Func: free the resources associated to the ValidSchemaPtr
  No return*/
  if (validSchemaPtr != NULL) xmlSchemaFreeValidCtxt (validSchemaPtr);
  
  //make the xmElem struct version of the DOM
  *top = mxMakeElem(xmlTree, xmlDocGetRootElement (xmlTree));
  
  /*hint function 4 - free up all the structures used by a document, tree included
  Return: VOID*/
  xmlFreeDoc (xmlTree);
  
  if (top==NULL) return 1;//safeguard in case mxMakeElem fails
  return 0;
}

/****************************************************
scrape out text stored in subordinate text node. elem isBlank flag = 1 if 
text is only white space
*****************************************************/
static void addElemText( xmlDocPtr doc, xmlNodePtr node, XmElem *elem){
  
  /*Recommended func: Returns:  a pointer to the string copy,
  the caller must free it with xmlFree(). */
  elem->text = (char *)xmlNodeListGetString ( doc, node->children,1);
  
  int i = 0;
  if (elem->text != NULL) i = strlen( elem->text );
  elem->isBlank = 1; //assume text is blank unless otherwise proven
  
  if (i > 0){ 
    for (int g = 0; g < i; g++){
      if ( isspace( elem->text[g] ) == 0 ){ //if it is not whitespace
		elem->isBlank = 0; //set isBlank to 0, meaning text is not whitespace
      }
    }
  }
}

/****************************************************
find number of attributes in a node
****************************************************/
static int getNumAttributes( xmlNodePtr node ){
  
  int sum = 0;
  xmlAttrPtr nextAttPtr = node->properties;
  if ( nextAttPtr == NULL ) {
    return 0;
  }
  
  while( nextAttPtr != NULL ){
   sum +=1;
   nextAttPtr = nextAttPtr->next;
  }
  
  return sum;
}

/****************************************************
same as strdup, returns a pointer to a new string which is a duplicate of str 
From here: http://cboard.cprogramming.com/c-programming/95462-compiler-error
-warning-implicit-declaration-function-strdup.html
****************************************************/
char *customCopy(const char *str){
    int n = strlen(str) + 1;
    char *dup = malloc(n);
    if(dup)
    {
        strcpy(dup, str);
    }
    return dup;
}

/****************************************************
loop through each attribute and add it's value and content to new element
****************************************************/
static void addAttribs( xmlNodePtr node, XmElem *newElem ){
	
	xmlAttrPtr a = node->properties;
	for (int i = 0; i < newElem->nattribs; i++){
		(*newElem->attrib)[i][0] = customCopy( (char *)a->name );
        assert( (*newElem->attrib)[i][0] );
		(*newElem->attrib)[i][1] = (char*)xmlGetProp(node, a->name ); //needs to be freed with xmlFree
		a = a->next;
	}
}

XmElem *mxMakeElem( xmlDocPtr doc, xmlNodePtr node ){
	
	//check for comment node, note this only happens at the begining 
	while ( node->type == XML_COMMENT_NODE ){
		node = node->next;
	}
	
	XmElem *newElem = malloc(sizeof(XmElem));
  	assert(newElem);
    
  	//copy node->name into newElem, memory needs to be freed later on!
  	newElem->tag = customCopy( (char *)node->name );
  	assert(newElem->tag);
  	addElemText( doc, node, newElem );
  
  	newElem->nattribs = getNumAttributes( node );
  	if (newElem->nattribs > 0){
  		newElem->attrib = malloc ( newElem->nattribs * sizeof (char*[2]) );//allocate the attribute array
        assert(newElem->attrib);
  	}else{
  		newElem->attrib = NULL;
  	}
  	addAttribs( node, newElem ); 
	
  	//Get Number of SubElements
  	newElem->nsubs = xmlChildElementCount (node);
  	
  	xmlNodePtr e = xmlFirstElementChild (node); //returns ptr to first child or NULL
  if (newElem->nsubs > 0){
    
    	//allocate space for an array of child elements
    	newElem->subelem = malloc( newElem->nsubs * sizeof(XmElem *));
        assert(newElem->subelem);
    
    	for (int i = 0; i < newElem->nsubs; i++){
      		(*newElem->subelem)[i] = mxMakeElem( doc, e );
      		e = xmlNextElementSibling (e);
    	}
	}
	return newElem;
}

/****************************************************
Personal note: go through and recursively free each element in turn. Remember!
subelements are just stored in an array fashion.
****************************************************/
void mxCleanElem( XmElem *top ){
    
  if (top->nsubs > 0){
    for (int i = 0; i < top->nsubs; ++i){
      mxCleanElem( (*top->subelem)[i] ); //recursive call to each sub child element
    }
    if ( (top->subelem) != NULL) free( top->subelem );//free array of structure pointers
  }

  if ( (top->tag) != NULL ) free (top->tag);
  if ( (top->text) != NULL ) xmlFree(top->text);
  
  //free attributes, loop through each attribute and free it's value and content fields
 for (int i = 0; i < top->nattribs; i++){
    if (((*top->attrib)[i][0])!=NULL)free( (*top->attrib)[i][0] );
    if (((*top->attrib)[i][1]) !=NULL)xmlFree( (*top->attrib)[i][1] ); 
 }
 if ( (top->attrib)!=NULL) free( top->attrib );
  if ( top !=NULL ) free (top); //free actual structure
  
}

/****************************************************
- first, check if we are looking for a code or a tag attrib
 - For tag attribute, ensure element attributes are "tag" and match int tag
 - For code attribute ensure element attributes are "code" and match int tag.
    - Note: this makes use of ability pass to compare the ascii codes of characters
Pre: mrecp contains a valid element to be have it's children checked for tag attribute
post: return the number of tags found within mrecp
****************************************************/
static int findTag(const XmElem *mrecp, int tag, char *strToMatch){
   for ( int i=0; i < mrecp->nattribs; i++ ){
    if ( ( strcmp( "tag", strToMatch ) == 0) && 
      atoi( (*mrecp->attrib)[i][1] ) == tag && ( strcmp( strToMatch, (*mrecp->attrib)[i][0]) == 0) ){
      return 1;
    }else if ( ( strcmp( "code", strToMatch ) == 0) && 
      *(*mrecp->attrib)[i][1] == tag && ( strcmp( strToMatch, (*mrecp->attrib)[i][0]) == 0) ){
      return 1;
    }
   }
  
  return 0;
}

/****************************************************
this function acts just as mxFindField would, however is needed to assert from 
caller that the record in question is in fact a record element.
****************************************************/
static int mxFindFieldAfterAssert( const XmElem *mrecp, int tag ){
   int count = 0;

   if (mrecp->nsubs > 0){
    for (int i = 0; i < mrecp->nsubs; i++){
      count += mxFindFieldAfterAssert( (*mrecp->subelem)[i], tag ); //recursive call to each sub child element
    }
  }
  
  //check for occurence of 'tag', must compare characters to ints
   for ( int i=0; i < mrecp->nattribs; i++ ){   
     /*compare each attribute, if it is == tag and of element "tag", add onto count
     this assumes only 1 tag attribute per set.*/
    if ( atoi( (*mrecp->attrib)[i][1] ) == tag && ( strcmp("tag", (*mrecp->attrib)[i][0]) == 0) ){
      return (count + 1);
    }
   }
  return count;
}

/****************************************************
note, two functions used due to recursive nature of searching for "tag", and
complications arising from asserting the passed in element is of type record.
Designed this way to adhere to assignment spec
****************************************************/
int mxFindField( const XmElem *mrecp, int tag ){
  assert( strcmp(mrecp->tag, "record") == 0 );
  
  return ( mxFindFieldAfterAssert(mrecp, tag) );
}

/****************************************************
internal function called by mxFindSubField,did not use recursion this time,
see mxFindFieldAfterAssert for a similar function using recursion
Pre: elem points to a record child
Post: returns the number of sub occurances in elems children
****************************************************/
static int getSubCount(const XmElem *elem, char sub){
  
   int count = 0;
  
    if (elem->nsubs > 0){
     for (int i = 0; i < elem->nsubs; i++){
       count += getSubCount( (*elem->subelem)[i], sub ); //recursive call to each sub child element
     }
   }
   
   //check for occurence of 'sub', must compare characters to string
    for ( int i=0; i < elem->nattribs; i++ ){   
      /*compare each attribute, if it is == sub and of element "code", add onto count
      this assumes only 1 tag attribute per set.*/
     if ( ( strcmp("code", (*elem->attrib)[i][0]) == 0) && *(*elem->attrib)[i][1] == sub  ){
       return (count + 1);
     }
    }
   return count;
}

/****************************************************
  traverse to the tnum'th occurance of strToMatch - will be either code or tag, 
  for loop will traverse through each element, breaking out after finding 
  tnum'th tag, that element will be the ith child.
  Pre: mrecp points to a valid record structure and tag is within a valid 
  range i.e, 1 < tag < Max
  Post: returns an integer representing child element position containing 
  tnum'th tag
****************************************************/
static int findNthAttrib(const XmElem *mrecp, int tag, int tnum, char *strToMatch ){

  int tagCount = 0;
  int i = 0;
  for (i=0; i < mrecp->nsubs; i++){ //this for loop goes through each child of the mrecp record
     if ( findTag( (*mrecp->subelem)[i], tag, strToMatch ) ) { //find tag returns 1 if the passed in child has the tag attribute
       tagCount+=1;
     }
     if (tagCount == tnum){
       break;
     }
  }
  return i;
}

int mxFindSubfield( const XmElem *mrecp, int tag, int tnum, char sub ){  
    
  int maxRange = mxFindField( mrecp, tag);
  if ( tnum < 1 || tnum > maxRange ) return 0; //tnum is out of range
  
  int i = findNthAttrib(mrecp, tag, tnum, "tag");
  return getSubCount((*mrecp->subelem)[i], sub ); //pass tnum'th( i ) child into getSubelem
}

const char *mxGetData( const XmElem *mrecp, int tag, int tnum, char sub, int snum ){
  assert( strcmp(mrecp->tag, "record") == 0 );//assert mrecp is of type record
  
  int maxTnumRange = mxFindField( mrecp, tag); 
  if ( tnum < 1 || tnum > maxTnumRange ) return NULL; //tnum is out of range
       
    //if 000<= tag <= 009, ignore subfield and return text from target tag child
    if ( 0 <= tag && tag <= 9 ){
      int tagChildLocation = findNthAttrib (mrecp, tag, tnum, "tag");
      
      XmElem *targetChild = (*mrecp->subelem)[tagChildLocation];
      return targetChild->text;
    }
    
    //*************check for invald sub tag here e.g = ''
      
    int maxSnumRange = mxFindSubfield(mrecp, tag, tnum, sub);
    if ( snum < 1 || snum > maxSnumRange ) return NULL; //ensure snum is within valid range
    
    //traverse to specific element and return data
    int tagChildLocation = findNthAttrib (mrecp, tag, tnum, "tag");
    XmElem *targetTagChild = (*mrecp->subelem)[tagChildLocation];
    
    int subChildLocation = findNthAttrib (targetTagChild, (int)sub, snum, "code");
    
    XmElem *targetSubChild = (*targetTagChild->subelem)[subChildLocation];
    
    return targetSubChild->text;
}

/****************************************************
Helper function for mxWriteFile, allows use of this function for other cases when 
printing a function header out each time is not ideal
Pre: top was successfully returned by mxReadFile and mxfile is open for writing
Post: The MarcXML contents of *top has been written to mxfile. Return # of records written
or EXIT_FAILURE if error. (e.g. an elements tag or attribs are NULL when they shouldn't be.
****************************************************/
int printElement(const XmElem *top, FILE *mxfile, int depth){
  int numElements = 0;
  
  for ( int i=0; i<depth; i++ ) fprintf(mxfile, "\t" );
 
    // print tag + attributes
   if ( top->tag != NULL && strcmp(top->tag, "collection") != 0 ){
     if ( fprintf( mxfile, "<marc:%s", top->tag ) < 1 ){
       fprintf (stderr, "\nError, could not write to file\n");
       return -1;
     }
     numElements += 1;
   }
  
  for ( int i=0; i < top->nattribs; i++ ){
    if ( (*top->attrib)[i][0] != NULL && (*top->attrib)[i][1] != NULL ){
      if ( strcmp(top->tag, "collection") != 0 ){
        fprintf( mxfile, " %s=\"%s\"", (*top->attrib)[i][0], (*top->attrib)[i][1] );
      }
    }
  }
  if (strcmp(top->tag, "collection") != 0){
    fprintf (mxfile, ">");
  }
  
  //print out element text
  int closeTagAfterKids = 0; 
  if (top->nsubs > 0){
    if ( strcmp(top->tag, "collection") != 0 ){
      fprintf(mxfile, "\n");
    }
    closeTagAfterKids = 1;
  }else{
     /*replace any entity special characters as discussed here:
     http://moodle.socs.uoguelph.ca/mod/forum/discuss.php?d=4544 */
     xmlChar *text = xmlEncodeSpecialChars (NULL, (xmlChar*)top->text);
    fprintf(mxfile, "%s</marc:%s>\n",text,top->tag);
    if (text !=NULL){
      free(text);
    }
    
  }
 
  // print subelements
  for ( int i=0; i < top->nsubs; i++ ) {
    ++depth;
    numElements += printElement( (*top->subelem)[i] , mxfile, depth);
    --depth;
  }
  
  if (closeTagAfterKids==1 && strcmp(top->tag, "collection") != 0){
    for ( int i=0; i<depth; i++ ) fprintf(mxfile, "\t" );
    fprintf(mxfile, "</marc:%s>\n", top->tag);
  }
  return numElements; 
}

int mxWriteFile( const XmElem *top, FILE *mxfile ){
  if (top != NULL && top->tag !=NULL && strcmp(top->tag, "collection") == 0){
    if ( fprintf (mxfile, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n") < 1 ){
      fprintf(stderr, "\nError, could not write to output file\n");
      return -1;
    }
    fprintf (mxfile,  "<!-- Output by mxutil library ( Craig Lehmann ) -->\n");
    fprintf( mxfile, "<marc:%s", top->tag );
    fprintf(mxfile, " xmlns:marc=\"http://www.loc.gov/MARC21/slim\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.loc.gov/MARC21/slim http://www.loc.gov/standards/marcxml/schema/MARC21slim.xsd\">\n");
  }else{
    fprintf(stderr, "\nError, invalid root node\n");
    return -1;
  }
  
  int temp = printElement( top, mxfile, 0 );
  fprintf (mxfile, "</marc:collection>\n");

  return temp;
}

