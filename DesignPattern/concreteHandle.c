

#include <assert.h>
#include "concreteHandle.h"


static void *concreteHandleACtor(void *_self, void* params) {
	_ConcreteHandleA *self = _self;
	return self;
}

static void *concreteHandleADtor(void *_self) {
	_ConcreteHandleA *self = _self;
	self->succ = NULL;
	return self;
}

static void concreteHandleASetSuccessor(void *_self, void *_succ) {
	_ConcreteHandleA *self = _self;
	self->succ = _succ;
}

static void* concreteHandleAGetSuccessor(const void *_self) {
	const _ConcreteHandleA *self = _self;
	return self->succ;
}

static void concreteHandleAhandleRequest(const void *_self, int request) {
	if (request >= 0 && request < 500) {
		fprintf(stdout, "ConcreteHandleA deal with: %d\n", request);
	} else if (concreteHandleAGetSuccessor(_self) != NULL) {
		const Handle * const *succ = concreteHandleAGetSuccessor(_self);
		(*succ)->handleRequest(succ, request);
	} else {
		fprintf(stderr, "Can't deal with: %d\n", request);
	}
}

static const Handle _concreteHandleA = {
	sizeof(_ConcreteHandleA),
	concreteHandleACtor,
	concreteHandleADtor,
	concreteHandleASetSuccessor,
	concreteHandleAGetSuccessor,
	concreteHandleAhandleRequest,
};

const void *ConcreteHandleA = &_concreteHandleA;



static void *concreteHandleBCtor(void *_self, void* params) {
	_ConcreteHandleB *self = _self;
	return self;
}

static void *concreteHandleBDtor(void *_self) {
	_ConcreteHandleB *self = _self;
	self->succ = NULL;
	return self;
}

static void concreteHandleBSetSuccessor(void *_self, void *_succ) {
	_ConcreteHandleB *self = _self;
	self->succ = _succ;
}

static void* concreteHandleBGetSuccessor(const void *_self) {
	const _ConcreteHandleB *self = _self;
	return self->succ;
}

static void concreteHandleBhandleRequest(const void *_self, int request) {
	if (request >= 500 && request < 1000) {
		fprintf(stdout, "ConcreteHandleB deal with: %d\n", request);
	} else if (concreteHandleBGetSuccessor(_self) != NULL) {
		const Handle * const *succ = concreteHandleBGetSuccessor(_self);
		(*succ)->handleRequest(succ, request);
	} else {
		fprintf(stderr, "Can't deal with: %d\n", request);
	}
}

static const Handle _concreteHandleB = {
	sizeof(_ConcreteHandleB),
	concreteHandleBCtor,
	concreteHandleBDtor,
	concreteHandleBSetSuccessor,
	concreteHandleBGetSuccessor,
	concreteHandleBhandleRequest,
};

const void *ConcreteHandleB = &_concreteHandleB;


static void *concreteHandleCCtor(void *_self, void* params) {
	_ConcreteHandleC *self = _self;
	return self;
}

static void *concreteHandleCDtor(void *_self) {
	_ConcreteHandleC *self = _self;
	self->succ = NULL;
	return self;
}

static void concreteHandleCSetSuccessor(void *_self, void *_succ) {
	_ConcreteHandleC *self = _self;
	self->succ = _succ;
}

static void* concreteHandleCGetSuccessor(const void *_self) {
	const _ConcreteHandleC *self = _self;
	return self->succ;
}

static void concreteHandleChandleRequest(const void *_self, int request) {
	if (request >= 1000) {
		fprintf(stdout, "ConcreteHandleC deal with: %d\n", request);
	} else if (concreteHandleCGetSuccessor(_self) != NULL) {
		const Handle * const *succ = concreteHandleCGetSuccessor(_self);
		(*succ)->handleRequest(succ, request);
	} else {
		fprintf(stderr, "Can't deal with: %d\n", request);
	}
}

static const Handle _concreteHandleC = {
	sizeof(_ConcreteHandleA),
	concreteHandleCCtor,
	concreteHandleCDtor,
	concreteHandleCSetSuccessor,
	concreteHandleCGetSuccessor,
	concreteHandleChandleRequest,
};

const void *ConcreteHandleC = &_concreteHandleC;



void *New(const void *_class, void *params) {
	const AbstractClass *class = _class;
	void *p = calloc(1, class->size);

	assert(p);
	*(const AbstractClass **)p = class;
	if (class->ctor) {
		p = class->ctor(p, params);
	}
	return p;
}

void Delete(void *_class) {
	const AbstractClass **class = _class;
	if (_class && (*class) && (*class)->dtor) {
		_class = (*class)->dtor(_class);
	}
	free(_class);
}

void SetSuccessor(void *_handle, void *_succ) {
	Handle **handle = _handle;
	if (_handle && (*handle) && (*handle)->setSuccessor) {
		(*handle)->setSuccessor(_handle, _succ);
	}
}

void HandleRequest(void *_handle, int request) {
	Handle **handle = _handle;
	if (_handle && (*handle) && (*handle)->handleRequest) {
		(*handle)->handleRequest(_handle, request);
	}
}

int main(int argc, char *argv[]) {
	void *h1 = New(ConcreteHandleA, NULL);
	void *h2 = New(ConcreteHandleB, NULL);
	void *h3 = New(ConcreteHandleC, NULL);

	SetSuccessor(h1, h2);
	SetSuccessor(h2, h3);

	HandleRequest(h1, 300);
	HandleRequest(h1, 600);

	HandleRequest(h1, 800);
	HandleRequest(h1, 1500);

	Delete(h1);
	Delete(h2);
	Delete(h3);

	return 0;
}


