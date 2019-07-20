#ifndef FSTRING_H
#define FSTRING_H

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * $Id: fstring.c,v 1.4 2007/04/24 01:43:05 swift Exp $
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Copyright University of Washington.   Written by Dana Swift.
 *
 * This software was developed at the University of Washington using funds
 * generously provided by the US Office of Naval Research, the National
 * Science Foundation, and NOAA.
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
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
/** RCS log of revisions to the C source code.
 *
 * \begin{verbatim}
 * $Log: fstring.c,v $
 * Revision 1.4  2007/04/24 01:43:05  swift
 * Added acknowledgement and funding attribution.
 *
 * Revision 1.3  2006/04/21 13:45:38  swift
 * Changed copyright attribute.
 *
 * Revision 1.2  2005/02/22 21:54:16  swift
 * Added fstrcmp() to compare two whole strings.
 *
 * Revision 1.1  2004/07/14 22:50:59  swift
 * Changes to reflect the transition to the new stdio library.
 *
 * \end{verbatim}
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#define fstringChangeLog "$RCSfile: fstring.c,v $ $Revision: 1.4 $ $Date: 2007/04/24 01:43:05 $"

#include <stddef.h>

/* function prototypes */
far char *fcpy(far char *dest, const char *src);
far char *fncpy(far char *dest, const char *src, size_t n);
int       fstrcmp(const char *s1, far const char *s2);
char     *fstrcpy(char *dest, far const char *src);
//char     *fstrncpy(char *dest, far const char *src, size_t n);
char     *fstrncpy(char *dest, far char *src, size_t n);//debug far const HM 12/29/2014
//char     *fstrncpy(char *dest, far const char *src, size_t n);
int       fstrncmp(const char *s1, far char *s2, size_t n);//debug removed const HM

#endif /* FSTRING_H */

/*------------------------------------------------------------------------*/
/* function to compare two strings                                        */
/*------------------------------------------------------------------------*/
/**
   This function reproduces the functionality of the C standard library
   function strcmp() except that it allows one of its arguments to be in far
   memory.  It compares s1 to s2 and returns an integer less than, equal to,
   or greater than zero if s1 is found, respectively, to be less than, to
   match, or be greater than s2.
*/
int fstrcmp(const char *s1, far const char *s2)
{
   int status=0;

   for (; (*s1)==(*s2); ++s1, ++s2) {if (*s1=='\0') return 0;}

   status=( (*(unsigned char *)s1) < (*(far unsigned char *)s2)) ? -1 : 1;

   return status;
}

/*------------------------------------------------------------------------*/
/* function to compare the first n characters of two strings              */
/*------------------------------------------------------------------------*/
/**
   This function reproduces the functionality of the C standard library
   function strncmp() except that it allows one of its arguments to be in
   far memory.  It compares the first n characters of s1, s2 and returns an
   integer less than, equal to, or greater than zero if s1 is found,
   respectively, to be less than, to match, or be greater than s2.
*/
//int fstrncmp(const char *s1, far const char *s2, size_t n)
int fstrncmp(const char *s1, far char *s2, size_t n)//debug HM
{
   int status=0;

   for (; n>0; ++s1, ++s2, --n)
   {
      if ((*s1) != (*s2))
      {
         status = (((*(unsigned char *)s1) < (*(far unsigned char *)s2)) ? -1 : 1);
         break;
      }
      else if ((*s1)==0) break;
   }

   return status;
}

/*------------------------------------------------------------------------*/
/* function to copy a string to far memory                                */
/*------------------------------------------------------------------------*/
/**
   This function reproduces the functionality of the C standard library
   function strcpy() except that it allows its first argument to be in far
   memory.  It copies the string pointed to by src (including the
   terminating NULL character) to the array pointed to by dest.  The
   destination string dest must be large enough to receive the copy.
*/
far char *fcpy(far char *dest, const char *src)
{
   far char *ptr=dest;

   for (;; dest++, src++)
   {
      *dest = *src;

      if (!(*dest)) break;
   }

   return ptr;
}

/*------------------------------------------------------------------------*/
/* function to copy a string to far memory                                */
/*------------------------------------------------------------------------*/
/**
   This function reproduces the functionality of the C standard library
   function strncpy() except that it allows its first argument to be in far
   memory.  It copies at most n bytes of the string pointed to by src
   (including the terminating NULL character) to the array pointed to by
   dest.  The destination string dest must be large enough to receive the
   copy.  In the case where the length of src is less than that of n, the
   remainder of dest will be padded with nulls.  If there is no null byte
   among the first n bytes of src, the result wil not be null-terminated.
*/
far char *fncpy(far char *dest, const char *src, size_t n)
{
   size_t i;
   far char *ptr=dest;
   
   for (i=0; i<n && (*src); i++,src++,dest++) {*dest = *src;}

   for (; i<n; i++,dest++) *dest=0;

   return ptr;
}

/*------------------------------------------------------------------------*/
/* function to copy a string from far memory                              */
/*------------------------------------------------------------------------*/
/**
   This function reproduces the functionality of the C standard library
   function strcpy() except that it allows its second argument to be in far
   memory.  It copies the far string pointed to by src (including the
   terminating NULL character) to the array pointed to by dest.  The
   destination string dest must be large enough to receive the copy.
*/
char *fstrcpy(char *dest, far const char *src)
{
   char *ptr=dest;

   for (;; dest++, src++)
   {
      *dest = *src;

      if (!(*dest)) break;
   }

   return ptr;
}

/*------------------------------------------------------------------------*/
/* function to copy a string from far memory                              */
/*------------------------------------------------------------------------*/
/**
   This function reproduces the functionality of the C standard library
   function strncpy() except that it allows its second argument to be in far
   memory.  It copies at most n bytes of the string pointed to by src
   (including the terminating NULL character) to the array pointed to by
   dest.  The destination string dest must be large enough to receive the
   copy.  In the case where the length of src is less than that of n, the
   remainder of dest will be padded with nulls.  If there is no null byte
   among the first n bytes of src, the result wil not be null-terminated.
*/
//char *fstrncpy(char *dest, far const char *src, size_t n)
char *fstrncpy(char *dest, far char *src, size_t n) //remove const HM 12/20/2014
{
   size_t i;
   char *ptr=dest;
   
   for (i=0; i<n && (*src); i++,src++,dest++) {*dest = *src;}

   for (; i<n; i++,dest++) *dest=0;

   return ptr;
}
