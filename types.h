#ifndef TYPES_H
#define TYPES_H

/* Shared Arg union for function arguments */
typedef union {
	int i;
	unsigned int ui;
	float f;
	const void *v;
} Arg;

#endif /* TYPES_H */
