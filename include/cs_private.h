#ifndef CS_PRIVATE_H
#define CS_PRIVATE_H

/* CS_PRIVATE marks a function which is an implementation detail of a
   container.  It expands to nothing; it is only a note to the reader that the
   function is not part of the interface, may change or disappear, and may be
   unsafe to call directly (for example without the lock held).  Use the
   documented functions instead. */
#define CS_PRIVATE

#endif
