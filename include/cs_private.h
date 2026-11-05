#ifndef CS_PRIVATE_H
#define CS_PRIVATE_H

/* CS_PRIVATE marks a function which is an implementation detail of a
   container.  It expands to nothing; it is only a note to the reader that the
   function is not part of the interface, may change or disappear, and may be
   unsafe to call directly (for example without the lock held).  Use the
   documented functions instead. */
#define CS_PRIVATE

/* Marks a variable or expression as used on purpose, to keep the compiler
   quiet about it.  Unlike the other macros of the headers it is not
   undefined at the end of them. */
#define CS_UNUSED(x) ((void) (x))

#endif
