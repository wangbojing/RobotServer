


#ifndef __CONCRETE_HANDLE_H__
#define __CONCRETE_HANDLE_H__

#include <stdlib.h>
#include <stdio.h>


typedef struct {
	size_t size;
	void* (*ctor)(void *_self, void* params);
	void* (*dtor)(void *_self);
} AbstractClass;

typedef struct {
	size_t size;
	void* (*ctor)(void *_self, void* params);
	void* (*dtor)(void *_self);
	void (*setSuccessor)(void *_self, void *succ);
	void* (*getSuccessor)(const void *_self);
	void (*handleRequest)(const void *_self, int request);
} Handle;

typedef struct {
	const void *_;
	void *succ;
} _ConcreteHandleA;

extern const void *ConcreteHandleA;


typedef struct {
	const void *_;
	void *succ;
} _ConcreteHandleB;

extern const void *ConcreteHandleB;



typedef struct {
	const void *_;
	void *succ;
} _ConcreteHandleC;

extern const void *ConcreteHandleC;


#endif



