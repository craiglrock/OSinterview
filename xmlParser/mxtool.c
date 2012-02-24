/**************************************************** 
 * mxtool.c - a linux command line tool for manipulating MARCXML files. Designed
 * traditinal UNIX style, taking input from stdin and output to stdout. Ideal for 
 * use with shell features such as pipes and redirection.
 *
 * Programmed by Craig Lehmann, 0643962
 * last updated Feb 12th 2012
 ****************************************************/

#include "mxtool.h"
#include "mxutil.h"
#include <stdlib.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <termios.h>
#include <regex.h>

/*******************************************
Print out a collection header, avoids repetitive code
Pre: top is a collection element
Post: Returns 1 for successfull print, 0 for any issue/error
********************************************/
static int printCollectionHeader(const XmElem *top, FILE *outfile){
  if ( fprintf (outfile, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n") < 1 ){
   fprintf (stderr, "\n Error, could not write to outfile\n");
   return 0;
  }
  fprintf (outfile,  "<!-- Output by mxutil library ( Craig Lehmann ) -->\n");
  fprintf( outfile, "<marc:%s", top->tag );
  fprintf(outfile, " xmlns:marc=\"http://www.loc.gov/MARC21/slim\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.loc.gov/MARC21/slim http://www.loc.gov/standards/marcxml/schema/MARC21slim.xsd\">\n");
  
  return 1;
}


/*******************************************
Check input arguments
Pre: argv's contain 1 of the valid valid arguments
Post: checks for validity of arguments, returns a number corresponding to each argument
review = 1, cat = 2, keep = 3, discard = 4, lib = 5, bib = 6, if error return 0
********************************************/
static int checkArgs( int args, char *argv[]){
  
  if (args <2){
    fprintf(stderr, "\nError, no option found\n");
    return 0;
  }
  
  if ( strcmp(argv[1], "-review")==0){
    if (args>2){
      fprintf (stderr, "\nErronius usage, excess arguments with review request\n");
      return 0;
    }
  return 1; 
  }else if ( strcmp(argv[1], "-cat")==0){
    return 2;
  }else if ( strcmp(argv[1], "-keep")==0){
    return 3;
  }else if ( strcmp(argv[1], "-discard")==0){
    return 4;
  }else if ( strcmp(argv[1], "-lib")==0){
    return 5;
  }else if ( strcmp(argv[1], "-bib")==0){
    return 6;
  }
  
  fprintf (stderr, "\nError invalid command option\n");
  return 0;
}

int concat( const XmElem *top1, const XmElem *top2, FILE *outfile ){
  
  if ( printCollectionHeader(top1, outfile) == 0 ){
    return EXIT_FAILURE;
  }
  
  int t1 = printElement( top1, outfile, 0 );
  int t2 = printElement( top2, outfile, 0 );
  fprintf (outfile, "</marc:collection>\n");

  if (t1 == EXIT_FAILURE || t2 == EXIT_FAILURE){
    return EXIT_FAILURE;
  }
  
  return EXIT_SUCCESS;
}

/*******************************************
Open a marcXMLFile and parse into its tree
Pre: marcXMLfp contains a pointer to a an xmlFile 
Post: Returns pointer to a parsed tree datastructure, Caller is responsible for
freeing marcXMLfp
*******************************************/
static XmElem * openXmElemTree( FILE *marcXMLfp ){
  char *schemaPath = getenv("MXTOOL_XSD"); //get the pathname of marc21 schema  
  xmlSchemaPtr schemaPtr = mxInit( schemaPath );
  if (schemaPtr==NULL){
    fprintf(stderr, "Error, check MXTOOL_XSD environment variable\n");
    return NULL;
  }
  
  if (marcXMLfp==NULL){
    fprintf(stderr, "Error, could not open xml file\n");
    mxTerm(schemaPtr);
    return NULL;
  }
  
  XmElem *top = NULL;
  int mxReadFileError = mxReadFile( marcXMLfp, schemaPtr, &top );
  mxTerm(schemaPtr);
  
  if (mxReadFileError == 1){
    fprintf(stderr, "\nFailed to parse XML file\n");
    return NULL;
  }else if (mxReadFileError == 2){
    fprintf(stderr, "\nXml did not match schema\n");
    return NULL;
  }
  
  return (top);
}

void marc2bib( const XmElem *mrec, BibData bdata ){
  
  /*get author info*/
  const char *author = mxGetData(mrec, 100, 1,'a', 1);
  if (author == NULL){
    author = mxGetData(mrec, 130, 1, 'a', 1);
  }
  if (author != NULL){
    bdata[AUTHOR] = customCopy( author );
  }else{
    bdata[AUTHOR] = customCopy( "na" );
  }
  
  /*get Title info*/
  const char *title1 = mxGetData(mrec, 245, 1, 'a', 1);
  const char *title2 = mxGetData(mrec, 245, 1, 'p', 1);
  const char *title3 = mxGetData(mrec, 245, 1, 'b', 1);
  
  if (title1 != NULL || title2 != NULL || title3 != NULL){
    assert ( asprintf(&bdata[TITLE], "%s%s%s", (title1==NULL ? "": title1), 
                      (title2==NULL ? "": title2), 
                      (title3==NULL ? "": title3)) != -1 );
  }else{
    bdata[TITLE] = customCopy( "na" );
  }
  
  /*get Publication info*/
  const char *pub1 = mxGetData(mrec, 260, 1, 'a', 1);
  const char *pub2 = mxGetData(mrec, 260, 1, 'b', 1);
  const char *pub3 = mxGetData(mrec, 260, 1, 'c', 1);
  const char *pub4 = mxGetData(mrec, 250, 1, 'a', 1);
  if (pub1 != NULL || pub2 != NULL || pub3 != NULL || pub4 != NULL){
    assert ( asprintf(&bdata[PUBINFO], "%s%s%s%s", 
                      (pub1==NULL ? "": pub1), 
                      (pub2==NULL ? "": pub2), 
                      (pub3==NULL ? "": pub3), 
                      (pub4==NULL ? "": pub4)) != -1 ); 
  }else{
    bdata[PUBINFO] = customCopy( "na" );
  }
  
  /*get Call number */
  const char *call1 = mxGetData(mrec, 90, 1, 'a', 1);
  const char *call2 = mxGetData(mrec, 90, 1, 'b', 1);
  if (call1 != NULL || call2 != NULL){
    assert ( asprintf(&bdata[CALLNUM], "%s%s", (call1==NULL ? "": call1), 
                      (call2==NULL ? "": call2)) != -1);
  }else{
    const char *call3 = mxGetData(mrec, 50, 1, 'a', 1);
    const char *call4 = mxGetData(mrec, 50, 1, 'b', 1);
    if (call3 != NULL || call2 != NULL){
      assert ( asprintf(&bdata[CALLNUM], "%s%s", 
                        (call3==NULL ? "": call3), 
                        (call4==NULL ? "": call4)) != -1);
    }else{
      bdata[CALLNUM] = customCopy( "na" );
    }
  }
}


int review( const XmElem *top, FILE *outfile ){
  
  if ( printCollectionHeader(top, outfile) == 0 ){
    return EXIT_FAILURE;
  }
  
  FILE *input = fopen("/dev/tty", "r");
  FILE *output = fopen("/dev/tty", "w");
  if (input==NULL || output==NULL){
    fprintf (stderr, "\nError, could not open /dev/tty\n");
    return EXIT_FAILURE;
  }
  
  //Page 195, Begining Linux Programming 4th Ed. 
  struct termios initial_settings, new_settings;
  tcgetattr(fileno(input) ,&initial_settings);
  new_settings = initial_settings;
  new_settings.c_lflag &= ~ICANON;
  new_settings.c_lflag &= ~ECHO;
  new_settings.c_cc[VMIN] = 1;
  new_settings.c_cc[VTIME] = 0;
  new_settings.c_lflag &= ~ISIG;
  if(tcsetattr(fileno(input), TCSANOW, &new_settings) != 0){
    fprintf(stderr,"could not set attributes\n");
    return EXIT_FAILURE;
  }
  
  BibData bibinfo;
  for (int i = 0; i < top->nsubs; i++){
    if ( (*top->subelem)[i] != NULL && strcmp( (*top->subelem)[i]->tag, "record") == 0 ){
      marc2bib( (*top->subelem)[i], bibinfo );
      fprintf (output, "%d. %s %s %s %s", i+1, bibinfo[AUTHOR], bibinfo[TITLE],
              bibinfo[PUBINFO], bibinfo[CALLNUM]);
      
      if ( bibinfo[CALLNUM][ strlen( bibinfo[CALLNUM])-1 ] != '.'){
        fprintf(output, "%c\n", '.');
      }else{
        fprintf(output, "\n");
      }
      //get input and act on it
      char c = fgetc( input );
      if ( c != ' ' && c != '\n' && c != 'd' && c != 'k' ){
        fprintf (output, "\nInvalid input:");
        fprintf (output, "\n< enter > : keep record");
        fprintf (output, "\n< space > : skip record");
        fprintf (output, "\n< k > : keep remaining records");
        fprintf (output, "\n< d > : discard remaining records\n");
        i --; //this will cause the last record to be displayed again
      }else if (c == ' '){
        //skip record, therefor do nothing
      }else if (c == '\n'){
        if ( printElement( (*top->subelem)[i] , outfile, 1) == -1 ){
          return EXIT_FAILURE;
        }
      }else if (c == 'd'){
        i = top->nsubs; //this will break the loop and 'discard' the rest of the records
      }else if (c == 'k'){
        for (int g = i; g < top->nsubs; g++){
          if ( printElement( (*top->subelem)[g] , outfile, 1) == -1 ){
          return EXIT_FAILURE;
          }
        }
        i = top->nsubs; //break i forloop
      }
        
      free(bibinfo[AUTHOR]);
      free(bibinfo[TITLE]);
      free(bibinfo[PUBINFO]);
      free(bibinfo[CALLNUM]);
    }
  }
  
  tcsetattr(fileno(input), TCSANOW, &initial_settings); //Page 195, Begining Linux Programming 4th ed
  
  fclose (input);
  fclose (output);
  
  fprintf (outfile, "</marc:collection>\n");

  return 0;
}

/*******************************************
Helper function for concat system, opens two files and sends them to concat function
Pre: args contains the number of strings in argv, argv[2] contains the file to be 
combined with stdin,outfile contains the file to write combined files to
Post: outfile contains the combined marcXML files, Return EXIT_FAILURE for any 
problem
*******************************************/
static int combineFiles(int args, char *argv[], FILE *outfile){
  
  XmElem *top1 = openXmElemTree( stdin );
  if (top1 == NULL){
    fprintf(stderr, "\nError, could not open file on stdin\n");
    return (EXIT_FAILURE);
  }
  
  FILE *marcXMLfp1 = fopen (argv[2], "r");
  if (marcXMLfp1 == NULL){
    fprintf(stderr, "\nError, could not open file \"%s\"\n",argv[2] ); 
    return EXIT_FAILURE; 
  }
  XmElem *top2 = openXmElemTree( marcXMLfp1 );
  if (top2 == NULL){
    return(EXIT_FAILURE);
  }
  fclose (marcXMLfp1);
  
  int returnVal = concat( top1, top2, outfile );
  mxCleanElem (top1);
  mxCleanElem (top2);
  
  return returnVal;
}

int match( const char *data, const char *regex ){
  
  //Note, coded from example located here: http://www.peope.net/old/regex.html
  regex_t regexVar;
  if ( regcomp( &regexVar, regex, 0 ) != 0 ){
    fprintf(stderr, "\nRegex compilation failed\n");
  }
  
  if ( regexec(&regexVar, data, 0, NULL, 0) == 0 ){
    regfree(&regexVar);
    return 1; //match was found
  }else{
    regfree(&regexVar);
    return 0;
  }
}

int selects( const XmElem *top, const enum SELECTOR sel, const char *pattern, FILE *outfile ){
  
  //check for valid input pattern
  if ( (pattern[0] != 'a' && pattern[0] != 't' && pattern[0] != 'p') || pattern[1] != '='){
    fprintf (stderr, "\nIncorrect string match pattern. Should be: <field>=<regex>\n");
    return EXIT_FAILURE;
  }
  
  if ( printCollectionHeader(top, outfile) == 0 ){
    return EXIT_FAILURE;
  }
  
  char const *reggie = &pattern[2];
  
  //search each child for string reggie it it's specified tag
  BibData bibinfo;
  if (sel==KEEP){
    for (int i = 0; i < top->nsubs; i++){
      
      if ( (*top->subelem)[i] != NULL ){
        marc2bib( (*top->subelem)[i], bibinfo );
      }
      
      switch (pattern[0]){
        case 'a':{
          if ( match(bibinfo[AUTHOR], reggie) ){         
            if ( printElement( (*top->subelem)[i] , outfile, 1) == -1 ){
              return EXIT_FAILURE;
            }
          }
          break;
        }
        case 't':{
          if ( match(bibinfo[TITLE], reggie) ){         
            if ( printElement( (*top->subelem)[i] , outfile, 1) == -1 ){
              return EXIT_FAILURE;
            }
          }
          break;
        }
        case 'p':{
          if ( match(bibinfo[PUBINFO], reggie) ){         
            if ( printElement( (*top->subelem)[i] , outfile, 1) == -1 ){
              return EXIT_FAILURE;
            }
          }
          break;
        }
        default:;
      }
      
      free(bibinfo[AUTHOR]);
      free(bibinfo[TITLE]);
      free(bibinfo[PUBINFO]);
      free(bibinfo[CALLNUM]);
    }
  }else if (sel == DISCARD){
    for (int i = 0; i < top->nsubs; i++){
      
      if ( (*top->subelem)[i] != NULL ){
        marc2bib( (*top->subelem)[i], bibinfo );
      }
    
      switch (pattern[0]){
        case 'a':{
          if ( match(bibinfo[AUTHOR], reggie) == 0 ){         
            if ( printElement( (*top->subelem)[i] , outfile, 1) == -1 ){
              return EXIT_FAILURE;
            }
          }
          break;
        }
        case 't':{
          if ( match(bibinfo[TITLE], reggie) == 0 ){         
            if ( printElement( (*top->subelem)[i] , outfile, 1) == -1 ){
              return EXIT_FAILURE;
            }
          }
          break;
        }
        case 'p':{
          if ( match(bibinfo[PUBINFO], reggie) ==0 ){         
            if ( printElement( (*top->subelem)[i] , outfile, 1) == -1 ){
              return EXIT_FAILURE;
            }
          }
          break;
        }
        default:;
      }
      
      free(bibinfo[AUTHOR]);
      free(bibinfo[TITLE]);
      free(bibinfo[PUBINFO]);
      free(bibinfo[CALLNUM]);
    }
  }
  
  fprintf (outfile, "</marc:collection>\n");
  return EXIT_SUCCESS;
}

/********************************************
Helper function for GetOrder. (solves issue with duplicate records)
Pre: length corresponds to the length of array. val contains an int to
check the array for.
Post: returns val if found, else 0
********************************************/
static int checkArrayForInt ( int *array, int val, int length ){
  
  for (int i = 0; i < length; i++){
    if ( array[i] == val ){
      return val;
    }
  }
  return -1;
}

/*********************************************
Helper function for sortRecs
Pre: collections contains a collection root node, keysOriginal is an array of
strings corresponding to collection elements, keysSorted is keysOriginal in a 
sorted order
Post: Return a pointer an integer array corresponding to correct order of elements
nodes. Caller must free returned array
**********************************************/
static int *getOrder( XmElem *collection, const char ** keysOriginal, char **keysSorted ){
  int *order = malloc( sizeof(int) * collection->nsubs );
  int length = 0;
  
  for (int i = 0; i < collection->nsubs; i++){
    
    for (int g = 0; g < collection->nsubs; g++){
      if ( strcmp( keysSorted[i], keysOriginal[g] ) == 0 ){
        if (checkArrayForInt(order, g, length) != g){
          order[i] = g;
          length++;
          g = collection->nsubs; //break out g for loop
        }
      }
    }
    
  }
  
  return order;
}


/*********************************************
qsort compare helper function
Pre: a and b contain trwo char arrays to be compared
Post: returns -1, 0, 1 for a less than, equal to or greater to b
*********************************************/
static int compare (const void *a, const void *b){
  return strcmp ((char*)a, (char*)b);
}

void sortRecs( XmElem *collection, const char *keys[] ){
  if (collection->nsubs < 2){
    return;
  }
  
  //copy keys into an array and sort new array
  char **keysSorted = malloc ( collection->nsubs * sizeof(char*) );
  assert(keysSorted);
  for (int i = 0; i < collection->nsubs; i++){
    keysSorted[i] = customCopy( keys[i] );
  }
  
  qsort(keysSorted, collection->nsubs, sizeof *keysSorted, compare);
  
  int *order = getOrder( collection, keys, keysSorted );
  
  //backup collection kid pointer addresses
  XmElem **backUpPtrs = malloc (sizeof ( XmElem *) * collection->nsubs );
  for (int i = 0; i < collection->nsubs; i++){
    backUpPtrs[i] = (*collection->subelem)[i];
  }
  
  //reasign collection's kids based on backUpPtrs and order array.
  for (int i = 0; i < collection->nsubs; i++){
    (*collection->subelem)[i] = backUpPtrs[ order[i] ];
  }
  
  free (order);
  free (backUpPtrs);
  
  //free keys
  for (int i = 0; i < collection->nsubs; i++){
    if (keysSorted[i] != NULL){
      free ((char*)keysSorted[i]);
    }
  }
  if (keysSorted != NULL){
    free ( keysSorted );
  }
  
}

int libFormat( const XmElem *top, FILE *outfile ){
  
  //prepare array of keys
  const char **keys = keys = malloc ( top->nsubs * sizeof(char*) );
  assert (keys != NULL);
  
  BibData bibinfo;
  for (int i = 0; i < top->nsubs; i++){
    
    if ( (*top->subelem)[i] != NULL ){
      marc2bib( (*top->subelem)[i], bibinfo );
      keys[i] = customCopy( bibinfo[CALLNUM] );
      free(bibinfo[AUTHOR]);
      free(bibinfo[TITLE]);
      free(bibinfo[PUBINFO]);
      free(bibinfo[CALLNUM]);  
    }
    
  }
  //sort records
  XmElem collection = *top;
  sortRecs( &collection, keys);
  
  //free keys
  for (int i = 0; i < top->nsubs; i++){
    if (keys[i] != NULL){
      free ((char*)keys[i]);
    }
  }
  if (keys != NULL){
    free ( keys );
  }
  
  //print in library format
  XmElem *temp = &collection;
  for (int i = 0; i < temp->nsubs; i++){
    marc2bib( (*temp->subelem)[i], bibinfo );
    fprintf(outfile, "\n%s %s %s %s", bibinfo[CALLNUM], bibinfo[AUTHOR],
            bibinfo[TITLE], bibinfo[PUBINFO]);
    if ( bibinfo[PUBINFO][ strlen( bibinfo[PUBINFO])-1 ] != '.'){
        fprintf(outfile, "%c\n", '.');
    }else{
        fprintf(outfile, "\n");
    }
    free(bibinfo[AUTHOR]);
    free(bibinfo[TITLE]);
    free(bibinfo[PUBINFO]);
    free(bibinfo[CALLNUM]);
  }
  
  return EXIT_SUCCESS;
}

int bibFormat( const XmElem *top, FILE *outfile ){
  
  //prepare array of keys
  const char **keys = keys = malloc ( top->nsubs * sizeof(char*) );
  assert (keys != NULL);
  
  BibData bibinfo;
  for (int i = 0; i < top->nsubs; i++){
    
    if ( (*top->subelem)[i] != NULL ){
      marc2bib( (*top->subelem)[i], bibinfo );
      keys[i] = customCopy( bibinfo[AUTHOR] );
      free(bibinfo[AUTHOR]);
      free(bibinfo[TITLE]);
      free(bibinfo[PUBINFO]);
      free(bibinfo[CALLNUM]);  
    }
  }
  //sort records
  XmElem collection = *top;
  sortRecs( &collection, keys);
  
  //free keys
  for (int i = 0; i < top->nsubs; i++){
    if (keys[i] != NULL){
      free ((char*)keys[i]);
    }
  }
  if (keys != NULL){
    free ( keys );
  }
  
  //print in library format
  XmElem *temp = &collection;
  for (int i = 0; i < temp->nsubs; i++){
    marc2bib( (*temp->subelem)[i], bibinfo );
    fprintf(outfile, "\n%s %s %s %s", bibinfo[AUTHOR], bibinfo[CALLNUM],
            bibinfo[TITLE], bibinfo[PUBINFO]);
    if ( bibinfo[PUBINFO][ strlen( bibinfo[PUBINFO])-1 ] != '.'){
        fprintf(outfile, "%c\n", '.');
    }else{
      fprintf(outfile, "\n");
    }
            
    free(bibinfo[AUTHOR]);
    free(bibinfo[TITLE]);
    free(bibinfo[PUBINFO]);
    free(bibinfo[CALLNUM]);
  }
  
  return EXIT_SUCCESS;
}

int main(int args, char *argv[]){
  
  int option = checkArgs(args, argv);
  int returnVal = 0;
  switch (option){
    case 1:{ //-review
        XmElem *top = openXmElemTree( stdin );
        if (top == NULL){
          return EXIT_FAILURE;
        }
        returnVal = review(top, stdout);
        mxCleanElem (top);
      break;
    }
    case 2:{ //-cat
      returnVal = combineFiles(args, argv, stdout);
      break;
    }
    case 3:{ //-keep 
      XmElem *top = openXmElemTree( stdin );
      if (top == NULL){
        return EXIT_FAILURE;
      }
      returnVal = selects(top, KEEP, argv[2], stdout);
      mxCleanElem(top);
      break;
    }
    case 4:{ //-discard
      XmElem *top = openXmElemTree( stdin );
      if (top == NULL){
        return EXIT_FAILURE;
      }
      returnVal = selects(top, DISCARD, argv[2], stdout);
      mxCleanElem(top);
      break;
    }
    case 5:{ //-lib
      XmElem *top = openXmElemTree( stdin );
      if (top == NULL){
        return EXIT_FAILURE;
      }
      returnVal = libFormat(top, stdout);
      mxCleanElem(top);
      break;
    }
    case 6:{ //-bib
      XmElem *top = openXmElemTree( stdin );
      if (top == NULL){
        return EXIT_FAILURE;
      }
      returnVal = bibFormat(top, stdout);
      mxCleanElem(top);
      break;
    }
    default://invalid command 
      return EXIT_FAILURE;
  }
  
  return returnVal;
}