#include <sysdep.h>
#include <pointer_guard.h>
#include <jmpbuf-offsets.h>
#include <stap-probe.h>

        /* Keep traditional entry points in with sigsetjmp(). */
ENTRY (setjmp)
	mov	x1, #1
	b	1f
END (setjmp)

ENTRY (_setjmp)
	mov	x1, #0
	b	1f
END (_setjmp)
libc_hidden_def (_setjmp)

ENTRY (__sigsetjmp)
	PTR_ARG (0)

1:
	stp	x19, x20, [x0, 0<<3]
	stp	x21, x22, [x0, 2<<3]
	stp	x23, x24, [x0, 4<<3]
	stp	x25, x26, [x0, 6<<3]
	stp	x27, x28, [x0, 8<<3]

stp	x29, x30, [x0, 10<<3]
mov	x2,  sp
str	x2,  [x0, 13<<3]
mov	w0, #0
RET
END (__sigsetjmp)
hidden_def (__sigsetjmp)