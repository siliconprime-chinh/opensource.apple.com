/*-
 * Copyright (c) 2000 Doug Rabson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	$FreeBSD: src/sys/sys/kobj.h,v 1.3 2000/05/01 10:45:14 dfr Exp $
 */

#ifndef _SYS_KOBJ_H_
#define _SYS_KOBJ_H_

/*
 * Forward declarations
 */
typedef struct kobj		*kobj_t;
typedef struct kobj_class	*kobj_class_t;
typedef struct kobj_method	kobj_method_t;
typedef int			(*kobjop_t)(void);
typedef struct kobj_ops		*kobj_ops_t;
typedef struct kobjop_desc	*kobjop_desc_t;
struct malloc_type;

struct kobj_method {
	kobjop_desc_t	desc;
	kobjop_t	func;
};

/*
 * A class is simply a method table and a sizeof value. When the first
 * instance of the class is created, the method table will be compiled 
 * into a form more suited to efficient method dispatch. This compiled 
 * method table is always the first field of the object.
 */
#define KOBJ_CLASS_FIELDS						\
	const char	*name;		/* class name */		\
	kobj_method_t	*methods;	/* method table */		\
	size_t		size;		/* object size */		\
	u_int		refs;		/* reference count */		\
	kobj_ops_t	ops		/* compiled method table */

struct kobj_class {
	KOBJ_CLASS_FIELDS;
#ifdef APPLE
/* B4BP (7/23/01 sent to BP) kobj_class fix */
	/*
	 * these are so casting to iconc_converter_class etc won't
	 * stomp the 8 bytes following instances of kobj_class
	 */
	void	*filler1;
	void	*filler2;
	void	*filler3;
#endif
};

/*
 * Implementation of kobj.
 */
#define KOBJ_FIELDS				\
	kobj_ops_t	ops;

struct kobj {
	KOBJ_FIELDS;
};

/*
 * The ops table is used as a cache of results from kobj_lookup_method().
 */

#define KOBJ_CACHE_SIZE	256

struct kobj_ops {
	kobj_method_t	cache[KOBJ_CACHE_SIZE];
	kobj_class_t	cls;
};

struct kobjop_desc {
	unsigned int	id;	/* unique ID */
	kobjop_t	deflt;	/* default implementation */
};

/*
 * Shorthand for constructing method tables.
 */
#define KOBJMETHOD(NAME, FUNC) { &NAME##_desc, (kobjop_t) FUNC }

#define DEFINE_CLASS(name, methods, size)	\
						\
struct kobj_class name ## _class = {		\
	#name, methods, size			\
}

/*
 * Compile the method table in a class.
 */
void		kobj_class_compile(kobj_class_t cls);

/*
 * Free the compiled method table in a class.
 */
void		kobj_class_free(kobj_class_t cls);

/*
 * Allocate memory for and initalise a new object.
 */
kobj_t		kobj_create(kobj_class_t cls,
#ifdef APPLE
			    int mtype,
#else
			    struct malloc_type *mtype,
#endif
			    int mflags);

/*
 * Initialise a pre-allocated object.
 */
void		kobj_init(kobj_t obj, kobj_class_t cls);

/*
 * Delete an object. If mtype is non-zero, free the memory.
 */
#ifdef APPLE
void		kobj_delete(kobj_t obj, int mtype);
#else
void		kobj_delete(kobj_t obj, struct malloc_type *mtype);
#endif

/*
 * Maintain stats on hits/misses in lookup caches.
 */
#ifdef KOBJ_STATS
extern int kobj_lookup_hits;
extern int kobj_lookup_misses;
#define KOBJOPHIT	do { kobj_lookup_hits++; } while (0)
#define KOBJOPMISS	do { kobj_lookup_misses++; } while (0)
#else
#define KOBJOPHIT	do { } while (0)
#define KOBJOPMISS	do { } while (0)
#endif

/*
 * Lookup the method in the cache and if it isn't there look it up the
 * slow way.
 */
#define KOBJOPLOOKUP(OPS,OP) do {					\
	kobjop_desc_t _desc = &OP##_##desc;				\
	kobj_method_t *_ce =						\
	    &OPS->cache[_desc->id & (KOBJ_CACHE_SIZE-1)];		\
	if (_ce->desc != _desc) {					\
		KOBJOPMISS;						\
		kobj_lookup_method(OPS->cls->methods, _ce, _desc);	\
	} else {							\
		KOBJOPHIT;						\
	}								\
	_m = _ce->func;							\
} while(0)

void kobj_lookup_method(kobj_method_t *methods,
			kobj_method_t *ce,
			kobjop_desc_t desc);

#endif /* !_SYS_KOBJ_H_ */
