


#ifndef __OBSERVER_H__
#define __OBSERVER_H__


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>

typedef void (*Print_FN)(void *data);
typedef void (*Handle_FN)(void *node, va_list *params);

typedef struct {
	size_t size;
	void* (*ctor)(void *_self, va_list *params);
	void* (*dtor)(void *_self);
	void (*insert)(const void *_self, void *data);
	void (*remove)(const void *_self, void *data);
	void (*iterator)(const void *_self, Handle_FN handle_fn, va_list *params);
	void (*print)(const void *_self, Print_FN print_fn);
} List;

typedef struct _Node {
	void *data;
	struct _Node *next;
} Node;

typedef struct {
	const void *_;
	Node *head;
} _SingleList;

typedef struct {
	size_t size;
	void* (*ctor)(void *_self, va_list *params);
	void* (*dtor)(void *_self);
} AbstractClass;


typedef struct {
	size_t size;
	void* (*ctor)(void *_self, va_list *params);
	void* (*dtor)(void *_self);
	void (*attach)(void *_self, void *_obv);
	void (*detach)(void *_self, void *_obv);
	void (*notify)(const void *_self);
	void (*setstate)(void *_self, char *_st);
	char* (*getstate)(const void *_self);
} Subject;

typedef struct {
	const void *_;
	void *obvs;
	char *st;
} _DataSubject;


typedef struct {
	size_t size;
	void* (*ctor)(void *_self, va_list *params);
	void* (*dtor)(void *_self);
	void (*update)(void *_self, const void *sub);
	void (*printinfo)(const void *_self);
} Observer;


typedef struct {
	const void *_;
	char *st;
	void *sub;
} _SheetObserver;


typedef struct {
	const void *_;
	char *st;
	void *sub;
} _ChatObserver;

typedef struct {
	const void *_;
	char *st;
	void *sub;
} _WordObserver;

void *New(const void *_class, ...);
void Delete(void *_class);
void SetState(void *_subject, char *_st);
void Notify(const void *_subject);
void Update(void *_observer, const void *_subject);
void Insert(void *_list, void *_item);
void Remove(void *_list, void *item);
void Iterator(const void *_list, Handle_FN handle_fn, ...);
void Print(void *_list, Print_FN print_fn);



#endif



