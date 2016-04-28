


#ifndef __ABSTRACT_FACTORY_H__
#define __ABSTRACT_FACTORY_H__

typedef struct {
	size_t size;
	void* (*ctor)(void *_self, va_list *params);
	void* (*dtor)(void *_self);
} AbstractClass;

#endif



