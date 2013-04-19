/* s.c - a segmented, extendable stack */

#include <stdbool.h>
#include <stdlib.h> /* NULL */
#include "m.h" /* mfile mfalloc findtabent */

#include "ob.h" /* object size */
/* typedef long long object; */

#include "s.h"
/*#define STACKSEGSZ 10 */

/*
typedef struct {
	unsigned nextseg;
	unsigned top;
	object data[STACKSEGSZ];
} stack;
*/

unsigned initstack(mfile *mem) {
	unsigned adr = mfalloc(mem, sizeof(stack));
	stack *s = (void *)(mem->base + adr);
	s->nextseg = 0;
	s->top = 0;
	return adr;
}

void dumpstack(mfile *mem, unsigned stackadr) {
	stack *s = (void *)(mem->base + stackadr);
	unsigned i;
	while (1) {
		for (i=0; i < s->top; i++) {
			dumpobject(s->data[i]);
		}
		if (i != STACKSEGSZ) break;
		s = (void *)(mem->base + s->nextseg);
	}
}

/* free a stack segment */
void sfree(mfile *mem, unsigned stackadr) {
	stack *s = (void *)(mem->base + stackadr);
	if (s->nextseg) sfree(mem, s->nextseg);
	mtab *tab;
	unsigned e = mtalloc(mem, 0, 0); /* allocate entry with 0 size */
	findtabent(mem, &tab, &e);
	tab->tab[e].adr = stackadr; /* insert address */
	tab->tab[e].sz = sizeof(stack); /* insert size */
}

unsigned count(mfile *mem, unsigned stackadr) {
	stack *s = (void *)(mem->base + stackadr);
	unsigned ct = 0;
	while (s->top == STACKSEGSZ) {
		ct += STACKSEGSZ;
		s = (void *)(mem->base + s->nextseg);
	}
	return ct + s->top;
}

void push(mfile *mem, unsigned stackadr, object o) {
	stack *s = (void *)(mem->base + stackadr); /* load the stack */

	while (s->top == STACKSEGSZ) { /* find top segment */
		s = (void *)(mem->base + s->nextseg);
	}

	s->data[s->top++] = o; /* push value */

	/* if push maxxed the topmost segment, link a new one */
	if (s->top == STACKSEGSZ) {
		if (s->nextseg == 0) {
			s->nextseg = initstack(mem);
		} else {
			s = (void *)(mem->base + s->nextseg);
			s->top = 0;
		}
	}
}

/* index the stack from top-down */
/* n.b. this code can only reliably access
   STACKSEGSZ elements from the top */
object top(mfile *mem, unsigned stackadr, integer i) {
	stack *s = (void *)(mem->base + stackadr);
	stack *p = NULL;

	/* find top segment */
	while (s->top == STACKSEGSZ) {
		p = s;
		s = (void *)(mem->base + s->nextseg);
	}
	if (s->top == 0) {
		if (p != NULL) s = p;
		else /*error("stack underflow");*/
			return invalid;
	} else if ((integer)s->top <= i) {
		i -= s->top;
		if (p != NULL) s = p;
		else /*error("stack underflow");*/
			return invalid;
	}
	return s->data[s->top-1-i];
}

/* index from top-down and put item there.
   the inverse of top. */
void pot(mfile *mem, unsigned stackadr, integer i, object o) {
	stack *s = (void *)(mem->base + stackadr);
	stack *p = NULL;


	/* find top segment */
	while (s->top == STACKSEGSZ) {
		p = s;
		s = (void *)(mem->base + s->nextseg);
	}
	if (s->top == 0) {
		if (p != NULL) s = p;
		else error("stack underflow");
	} else if ((integer)s->top < i) {
		i -= s->top;
		if (p != NULL) s = p;
		else error("stack underflow");
	}
	s->data[s->top-1-i] = o;
}

/* index from bottom up */
object bot(mfile *mem, unsigned stackadr, integer i) {
	stack *s = (void *)(mem->base + stackadr);

	/* find desired segment */
	while (i > STACKSEGSZ) {
		i -= STACKSEGSZ;
		s = (void *)(mem->base + s->nextseg);
	}
	return s->data[i];
}

object pop(mfile *mem, unsigned stackadr) {
	stack *s = (void *)(mem->base + stackadr);
	stack *p = NULL;

	/* find top segment */
	while (s->top == STACKSEGSZ) {
		p = s;
		s = (void *)(mem->base + s->nextseg);
	}
	if (s->top == 0) {
		if (p != NULL) s = p; /* back up if top is empty */
		else /* error("stack underflow"); */
			return invalid;
	}

	return s->data[--s->top]; /* pop value */
}

#ifdef TESTMODULE

#include <stdio.h>
#include <unistd.h>

mfile mem;
unsigned s, t;

/* initialize everything */
void init(void) {
	pgsz = getpagesize();
	initmem(&mem);
	s = initstack(&mem);
	t = initstack(&mem);
}

void xit(void) {
	exitmem(&mem);
}

int main() {
	init();

	printf("\n^test s.c\n");
	printf("test stack by reversing a sequence\n");
	//object a = { .int_.val = 2 }, b = ( .int_.val = 12 }, c = { .int_.val = 0xF00 };
	object a = consint(2);
	object b = consint(12);
	object c = consint(0xF00);
	object x = a, y = b, z = c;
	printf("x = %d, y = %d, z = %d\n", x.int_.val, y.int_.val, z.int_.val);

	push(&mem, s, a);
	push(&mem, s, b);
	push(&mem, s, c);

	x = pop(&mem, s); /* x = c */
	push(&mem, t, x);
	y = pop(&mem, s); /* y = b */
	push(&mem, t, y);
	z = pop(&mem, s); /* z = a */
	push(&mem, t, z);
	printf("x = %d, y = %d, z = %d\n", x.int_.val, y.int_.val, z.int_.val);

	x = pop(&mem, t); /* x = a */
	y = pop(&mem, t); /* y = b */
	z = pop(&mem, t); /* z = c */
	printf("x = %d, y = %d, z = %d\n", x.int_.val, y.int_.val, z.int_.val);
	//z = pop(&mem, t);

	xit();
	return 0;
}

#endif
