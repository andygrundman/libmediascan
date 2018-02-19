/*
 * ptw32_tkAssocDestroy.c
 *
 * Description:
 * This translation unit implements routines which are private to
 * the implementation and may be used throughout it.
 *
 * --------------------------------------------------------------------------
 *
 *      Pthreads-win32 - POSIX Threads Library for Win32
 *      Copyright(C) 1998 John E. Bossom
 *      Copyright(C) 1999,2005 Pthreads-win32 contributors
 *
 *      Contact Email: rpj@callisto.canberra.edu.au
 *
 *      The current list of contributors is contained
 *      in the file CONTRIBUTORS included with the source
 *      code distribution. The list can also be seen at the
 *      following World Wide Web location:
 *      http://sources.redhat.com/pthreads-win32/contributors.html
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library in the file COPYING.LIB;
 *      if not, write to the Free Software Foundation, Inc.,
 *      59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include "pthread.h"
#include "implement.h"


void
ptw32_tkAssocDestroy (ThreadKeyAssoc * assoc)
     /*
      * -------------------------------------------------------------------
      * This routine releases all resources for the given ThreadKeyAssoc
      * once it is no longer being referenced
      * ie) either the key or thread has stopped referencing it.
      *
      * Parameters:
      *              assoc
      *                      an instance of ThreadKeyAssoc.
      * Returns:
      *      N/A
      * -------------------------------------------------------------------
      */
{

  /*
   * Both key->keyLock and thread->threadLock are locked on
   * entry to this routine.
   */
  if (assoc != NULL)
    {
      ThreadKeyAssoc * prev, * next;

      /* Remove assoc from thread's keys chain */
      prev = assoc->prevKey;
      next = assoc->nextKey;
      if (prev != NULL)
	{
	  prev->nextKey = next;
	}
      if (next != NULL)
	{
	  next->prevKey = prev;
	}

      if (assoc->thread->keys == assoc)
	{
	  /* We're at the head of the thread's keys chain */
	  assoc->thread->keys = next;
	}
      if (assoc->thread->nextAssoc == assoc)
	{
	  /*
	   * Thread is exiting and we're deleting the assoc to be processed next.
	   * Hand thread the assoc after this one.
	   */
	  assoc->thread->nextAssoc = next;
	}

      /* Remove assoc from key's threads chain */
      prev = assoc->prevThread;
      next = assoc->nextThread;
      if (prev != NULL)
	{
	  prev->nextThread = next;
	}
      if (next != NULL)
	{
	  next->prevThread = prev;
	}

      if (assoc->key->threads == assoc)
	{
	  /* We're at the head of the key's threads chain */
	  assoc->key->threads = next;
	}

      free (assoc);
    }

}				/* ptw32_tkAssocDestroy */
