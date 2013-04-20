#include <stdbool.h>

#include "m.h"
#include "ob.h"
#include "s.h"
#include "itp.h"
#include "nm.h"
#include "di.h"
#include "op.h"
#include "ops.h"

/* any  pop  -
   discard top element */
void Apop (context *ctx, object x) {
	(void)ctx;
	(void)x;
}

/* any1 any2  exch  any2 any1
   exchange top two elements */
void AAexch (context *ctx, object x, object y) {
	push(ctx->lo, ctx->os, y);
	push(ctx->lo, ctx->os, x);
}

/* any  dup  any any
   duplicate top element */
void Adup (context *ctx, object x) {
	push(ctx->lo, ctx->os, x);
	push(ctx->lo, ctx->os, x);
}

/* any1..anyN N  copy  any1..anyN any1..anyN
   duplicate top n elements */
void Icopy (context *ctx, object n) {
	int i;
	if (n.int_.val < 0) error("rangecheck");
	if ((unsigned)n.int_.val >= count(ctx->lo, ctx->os)) error("stackunderflow");
	for (i=0; i < n.int_.val; i++)
		push(ctx->lo, ctx->os, top(ctx->lo, ctx->os, n.int_.val - 1));
	//TODO limited to stack segment
}

/* anyN..any0 N  index  anyN..any0 anyN
   duplicate arbitrary element */
void Iindex (context *ctx, object n) {
	if (n.int_.val < 0) error("rangecheck");
	if ((unsigned)n.int_.val >= count(ctx->lo, ctx->os)) error("stackunderflow");
	push(ctx->lo, ctx->os, top(ctx->lo, ctx->os, n.int_.val));
}

/* a(n-1)..a(0) n j  roll  a((j-1)mod n)..a(0) a(n-1)..a(j mod n)
   roll n elements j times */
void IIroll (context *ctx, object n, object j) {
	//?
}

/* |- any1..anyN  clear  |-
   discard all elements */
void Zclear (context *ctx) {
	stack *s = (void *)(ctx->lo->base + ctx->os);
	s->top = 0;
}

/* |- any1..anyN  count  |- any1..anyN N
   count elements on stack */
void Zcount (context *ctx) {
	push(ctx->lo, ctx->os, consint(count(ctx->lo, ctx->os)));
}


/*
   -  currentcontext  context
   return curent context identifier

   mark obj1..objN proc  fork  context
   create context executing proc with obj1..objN as operands

   context  join  mark obj1..objN
   await context termination and return its results

   context  detach  -
   enable context to terminate immediately when done

   -  lock  lock
   create lock object

   lock proc  monitor  -
   execute proc while holding lock

   -  condition  condition
   create condition object

   local condition  wait  -
   release lock, wait for condition, reacquire lock

   condition  notify  -
   resume contexts waiting for condition

   -  yield  -
   suspend current context momentarily
   */

void initops(context *ctx, object sd) {
	oper *optab = (void *)(ctx->gl->base + adrent(ctx->gl, OPTAB));
	object n,op;
	op = consoper(ctx, "pop", Apop, 0, 1, anytype); INSTALL;
	op = consoper(ctx, "exch", AAexch, 2, 2, anytype, anytype); INSTALL;
	op = consoper(ctx, "dup", Adup, 2, 1, anytype); INSTALL;
	op = consoper(ctx, "copy", Icopy, 0, 1, integertype); INSTALL;
	op = consoper(ctx, "index", Iindex, 1, 1, integertype); INSTALL;
	op = consoper(ctx, "clear", Zclear, 0, 0); INSTALL;
	op = consoper(ctx, "count", Zcount, 1, 0); INSTALL;
	bdcput(ctx, sd, consname(ctx, "mark"), mark);
}
