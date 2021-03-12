/*
 * strrpl.c:
 *    go thru the files specified on the command line
 *    and replace strings therein.
 *    Type "strrpl" for more info.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>

PSZ pszFileMask=NULL,pszReplaceWith=NULL,pszSearchFor=NULL;
ULONG ulTotalFound=0,ulTotalSize=0;

/*
 * Explain:
 *
 */

void Explain(void)
{
printf("strrpl V0.84 (w) (c) 1998 Ulrich M”ller (modified release of 25-11-2020)\n");
printf("Part of the XFolder source package.\n");
printf("This is free software under the GNU General Public Licence (GPL).\n");
printf("Refer to the COPYING file in the XFolder installation dir for details.\n");
printf("Goes thru the specified files and replaces strings therein.\n");
printf("Usage: strrpl <filemask> <search> <replace>\n");
printf("with:\n");
printf("   <filemask>  the files to search for (e.g. \"*.html\")\n");
printf("   <search>    the string to search for\n");
printf("   <replace>   the string to replace all occurences of <search> with\n");
}

/*
 * doshQueryFileSize:
 *    returns the size of an already opened file
 */

ULONG doshQueryFileSize(HFILE hFile)
{
FILESTATUS3 fs3;
if (DosQueryFileInfo(hFile,FIL_STANDARD,&fs3,sizeof(fs3)))
   return 0;
else
   return fs3.cbFile;
}

/*
 * doshReadTextFile:
 *    reads a text file from disk, allocates memory
 *    via malloc() and returns a pointer to this
 *    buffer (or NULL upon errors). Specify in
 *    ulExtraMemory how much extra memory (in addition
 *    to the file's size) should be allocated to
 *    allow text manipulation.
 *    Returns NULL if an error occured.
 */

PSZ doshReadTextFile(PSZ pszFile,ULONG ulExtraMemory)
{
APIRET rc;
HFILE hFile;
PSZ pszContent=NULL;
ULONG ulAction,ulBytesRead=0,ulLocal,ulSize;

rc=DosOpen(pszFile,
           &hFile,
           &ulAction,                      /* action taken */
           5000L,                          /* primary allocation size */
           FILE_ARCHIVED | FILE_NORMAL,    /* file attribute */
           OPEN_ACTION_OPEN_IF_EXISTS,     /* open flags */
           OPEN_FLAGS_NOINHERIT |
           OPEN_SHARE_DENYNONE  |
           OPEN_ACCESS_READONLY,           /* read-write mode */
           NULL);                          /* no EAs */

if (rc==NO_ERROR)
   {
   ulSize=doshQueryFileSize(hFile);
   ulTotalSize=ulSize+10+ulExtraMemory;
   pszContent=(PSZ)malloc(ulTotalSize);
   DosSetFilePtr(hFile,0L,FILE_BEGIN,&ulLocal);
   DosRead(hFile,pszContent,ulSize,&ulBytesRead);
   DosClose(hFile);
   *(pszContent+ulBytesRead)='\0';
   }

return pszContent;
}

/*
 * doshWriteTextFile:
 *    writes a text file to disk; pszFile must contain the
 *    whole path and filename.
 *    Returns the number of bytes written.
 */

ULONG doshWriteTextFile(PSZ pszFile,PSZ pszContent)
{
APIRET rc;
HFILE hFile;
ULONG ulAction,ulLocal,ulWritten=0;

rc=DosOpen(pszFile,
           &hFile,
           &ulAction,                      /* action taken */
           5000L,                          /* primary allocation size */
           FILE_ARCHIVED | FILE_NORMAL,    /* file attribute */
           OPEN_ACTION_CREATE_IF_NEW |
           OPEN_ACTION_REPLACE_IF_EXISTS,  /* open flags */
           OPEN_FLAGS_NOINHERIT |
           OPEN_SHARE_DENYNONE  |
           OPEN_ACCESS_WRITEONLY,          /* read-write mode */
           NULL);                          /* no EAs */

if (rc==NO_ERROR)
   {
   ULONG ulSize=strlen(pszContent);      /* exclude 0 byte */

   rc=DosSetFilePtr(hFile,0L,FILE_BEGIN,&ulLocal);
   if (rc==NO_ERROR)
      {
      rc=DosWrite(hFile,pszContent,ulSize,&ulWritten);
      if (rc==NO_ERROR)
         rc=DosSetFileSize(hFile,ulSize);
      }

   DosClose(hFile);

   if (rc!=NO_ERROR)
      ulWritten=0;
    }

return ulWritten;
}

/*
 * strhReplace:
 *    replace oldStr by newStr in str.
 *
 *    str should have enough allocated space for the replacement, no check
 *    is made for this. str and OldStr/NewStr should not overlap.
 *    The empty string ("") is found at the beginning of every string.
 * 
 *    Returns: pointer to first location behind where NewStr was inserted
 *    or NULL if OldStr was not found.
 *    This is useful for multiple replacements also.
 *    (be careful not to replace the empty string this way !)
 *
 *    Author:     Gilles Kohl
 *    Started:    09.06.1992   12:16:47
 *    Modified:   09.06.1992   12:41:41
 *    Subject:    Replace one string by another in a given buffer.
 *                This code is public domain. Use freely.
 */

char* strhReplace(char* str,char* oldStr,char* newStr)
{
char *p,*q;
int OldLen,NewLen;

if (NULL==(p=strstr(str,oldStr)))
   return p;

OldLen=strlen(oldStr);
NewLen=strlen(newStr);
memmove(q=p+NewLen,p+OldLen,strlen(p+OldLen)+1);
memcpy(p,newStr,NewLen);

return q;
}

/*
 * ProcessFile:
 *
 */

void ProcessFile(PSZ pszFileName)
{
BOOL fChanged=FALSE;
LONG lExtra;
PSZ pszContent;

lExtra=strlen(pszReplaceWith)-strlen(pszSearchFor);
if (lExtra<0)
   lExtra=-lExtra;

pszContent=doshReadTextFile(pszFileName,lExtra+10);
if (pszContent)
   {
   BOOL fCont=TRUE;
   PSZ pszFound;
   while (fCont)
      {
      ulTotalSize+=lExtra;
      pszContent=realloc(pszContent,ulTotalSize);
      pszFound=strhReplace(pszContent,pszSearchFor,pszReplaceWith);
      fCont=(pszFound!=NULL);
      if (pszFound)
         {
         printf("  \"%s\": replaced string @ pos %d\n",pszFileName,
                                                       pszFound-pszContent);
         fChanged=TRUE;
         ulTotalFound++;
         }
      }

   if (fChanged)
      {
      if (doshWriteTextFile(pszFileName,pszContent)==0)
         {
         printf("Error writing file \"%s\".\n",pszFileName);
         exit(1);
         }
      }
   }
else
   {
   printf("Error reading file \"%s\".\n",pszFileName);
   exit(1);
   }
}

/*
 * main:
 *
 */

int main(int argc,char *argv[])
{
APIRET rc=NO_ERROR; /* Return code                  */
BOOL fProceed=FALSE;
FILEFINDBUF3 FindBuffer={0};      
HDIR hdirFindHandle=HDIR_CREATE;
ULONG ulFindCount=1,ulResultBufLen,ulTotalFiles=0;
    
ulResultBufLen=sizeof(FILEFINDBUF3);

/* parse parameters on cmd line */
if (argc>1)
   {
   SHORT i=0;
   while (i++<argc-1)
      {
      if (pszFileMask==NULL)
         pszFileMask=strdup(argv[i]);
      else if (pszSearchFor==NULL)
         pszSearchFor=strdup(argv[i]);
      else if (pszReplaceWith==NULL)
         {
         pszReplaceWith=strdup(argv[i]);
         fProceed=TRUE;
         }
      else
         fProceed=FALSE;
      }
   }

if (!fProceed)
   {
   Explain();
   return 1;
   }

if (strstr(pszReplaceWith,pszSearchFor)!=NULL)
   {
   printf("Error: \"%s\" contains \"%s\" (known restriction).\n",pszReplaceWith,
                                                                pszSearchFor);
   return 1;
   }

/* find files */
rc=DosFindFirst(pszFileMask,           /* File pattern */
                &hdirFindHandle,      /* Directory search handle */
                FILE_NORMAL,          /* Search attribute */
                &FindBuffer,          /* Result buffer */
                ulResultBufLen,       /* Result buffer length */
                &ulFindCount,         /* Number of entries to find */
                FIL_STANDARD);        /* Return level 1 file info */

if (rc!=NO_ERROR)
   printf("Error: No files found for file mask \"%s\".\n",pszFileMask);
else
   {
   /* no error: */
   printf("Searching for \"%s\" in \"%s\"...\n",pszSearchFor,pszFileMask);

   /* process first found file */
   ProcessFile(FindBuffer.achName);
   ulTotalFiles++;

   /* keep finding the next file until there are no more files */
   while (rc!=ERROR_NO_MORE_FILES)
      {
      ulFindCount=1;                      /* Reset find count. */

      rc=DosFindNext(hdirFindHandle,      /* Directory handle */
                     &FindBuffer,         /* Result buffer */
                     ulResultBufLen,      /* Result buffer length */
                     &ulFindCount);       /* Number of entries to find */

      if (rc==NO_ERROR)
         {
         ProcessFile(FindBuffer.achName);
         ulTotalFiles++;
         }
      } /* endwhile */

   rc=DosFindClose(hdirFindHandle);    /* close our find handle */
   if (rc!=NO_ERROR)
      printf("DosFindClose error\n");

   printf("Files processed:        %d.\n",ulTotalFiles);
   printf("Total strings replaced: %d.\n",ulTotalFound);
   }

return 0;
}

