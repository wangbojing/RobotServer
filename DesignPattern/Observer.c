

#include "Observer.h"


static void *chatObserverCtor(void *_self, va_list *params) {
	_ChatObserver *self = _self;
	self->sub = va_arg(*params, void*);
	const Subject **sub = self->sub;

	assert(self->sub && (*sub) && (*sub)->attach);
	(*sub)->attach(self->sub, _self);

	return self;
}

static void *chatObserverDtor(void *_self) {
	_ChatObserver *self = _self;
	const Subject **sub = self->sub;

	assert(self->sub && (*sub) && (*sub)->detach);
	(*sub)->detach(self->sub, _self);
	self->sub = NULL;

	return self;
}

static void chatObserverPrintInfo(const void *_self) {
	const _ChatObserver *self = _self;
	fprintf(stdout, "ChatObserver:%s\n", self->st);
}

static void chatObserverUpdate(void *_self, const void *_sub) {
	_ChatObserver *self = _self;
	const Subject * const *sub = _sub;

	assert(_sub && (*sub) && (*sub)->getstate);
	self->st = (*sub)->getstate(_sub);

	chatObserverPrintInfo(_self);
}


static const Observer _chatObserver = {
	sizeof(_ChatObserver),
	chatObserverCtor,
	chatObserverDtor,
	chatObserverUpdate,
	chatObserverPrintInfo,
};


const void *ChatObserver = &_chatObserver;


static void *sheetObserverCtor(void *_self, va_list *params) {
	_SheetObserver *self = _self;
	self->sub = va_arg(*params, void*);
	Subject **sub = self->sub;

	assert(self->sub && (*sub) && (*sub)->attach);
	(*sub)->attach(self->sub, _self);
	
	return self;
}

static void *sheetObserverDtor(void *_self) {
	_SheetObserver *self = _self;
	const Subject **sub = self->sub;

	assert(self->sub && (*sub) && (*sub)->detach);
	(*sub)->detach(self->sub, _self);

	return self;
}

static void sheetObserverPrintInfo(const void *_self) {
	const _SheetObserver *self = _self;
	fprintf(stdout, "sheetObserver:%s\n", self->st);
}

static void sheetObserverUpdate(void *_self, const void *_sub) {
	_SheetObserver *self = _self;
	const Subject * const *sub = _sub;

	assert(_sub && (*sub) && (*sub)->getstate);
	self->st = (*sub)->getstate(_sub);

	sheetObserverPrintInfo(_self);
}

static const Observer _sheetObserver = {
	sizeof(_SheetObserver),
	sheetObserverCtor,
	sheetObserverDtor,
	sheetObserverUpdate,
	sheetObserverPrintInfo,
};

const void *SheetObserver = &_sheetObserver;


static void *wordObserverCtor(void *_self, va_list *params) {
	_WordObserver *self = _self;

	self->sub = va_arg(*params, void*);
	Subject **sub = self->sub;
	assert(self->sub && (*sub) && (*sub)->attach);
	(*sub)->attach(self->sub, _self);

	return self;
}

static void *wordObserverDtor(void *_self) {
	_WordObserver *self = _self;
	const Subject **sub = self->sub;

	assert(self->sub && (*sub) && (*sub)->detach);
	(*sub)->detach(self->sub, _self);

	return self;
}

static void wordObserverPrintInfo(const void *_self) {
	const _WordObserver *self = _self;
	fprintf(stdout, "wordObserver:%s\n", self->st);
}

static void wordObserverUpdate(void *_self, const void *_sub) {
	_WordObserver *self = _self;
	const Subject * const *sub = _sub;

	assert(_sub && (*sub) && (*sub)->getstate);
	self->st = (*sub)->getstate(_sub);

	wordObserverPrintInfo(_self);
}

static const Observer _wordObserver = {
	sizeof(_SheetObserver),
	wordObserverCtor,
	wordObserverDtor,
	wordObserverUpdate,
	wordObserverPrintInfo,
};

const void *WordObserver = &_wordObserver;


static void *singleListCtor(void *_self, va_list *params) {
	_SingleList *self = _self;
	self->head = (Node*)calloc(1, sizeof(Node));
	self->head->next = NULL;
	return self;
}

static void *singleListDtor(void *_self) {
	_SingleList *self = _self;
	Node **p = &self->head;

	while ((*p) != NULL) {
		Node *node = *p;
		*p = node->next;
		free(node);
	}
}


static void singleListInsert(const void *_self, void *data) {
	const _SingleList *self = _self;
	Node *node = (Node*)calloc(1, sizeof(Node));
	Node **p = (Node**)&self->head;
#if 0
	for (; (*p) != NULL; p = &(*p)->next) ;

	node->data = data;
	node->next = p;
	*p = node;
#else
	node->data = data;
	node->next = (*p)->next;
	(*p)->next = node;
#endif
}


static void singleListRemove(const void *_self, void *data) {
	const _SingleList *self = _self;
	Node **p = (Node **)&self->head;

	while ((*p) != NULL) {
		Node *node = *p;
		if (node->data == data) {
			*p = node->next;

			free(node->data);
			free(node);
		} else {
			p = &(*p)->next;
		}
	}
}

static void singleListIterator(const void *_self, Handle_FN handle_fn, va_list *params) {
	const _SingleList *self = _self;
	Node **p = &self->head->next;

	for (; (*p) != NULL; p = &(*p)->next) {
		//printf("aaaaa\n");
		va_list args;
		va_copy(args, *params);
		handle_fn((*p)->data, &args);
		va_end(args);
	}
}

static void singleListPrint(const void *_self, Print_FN print_fn) {
	const _SingleList *self = _self;
	Node **p = &self->head->next;

	while ((*p) != NULL) {
		print_fn((*p)->data);
		p = &(*p)->next;
	}
}

static const List _singleList = {
	sizeof(_SingleList),
	singleListCtor,
	singleListDtor,
	singleListInsert,
	singleListRemove,
	singleListIterator,
	singleListPrint,
};

const void *SingleList = &_singleList;

static void *dataSubjectCtor(void *_self, va_list *params) {
	_DataSubject *self = _self;
	self->obvs = New(SingleList, NULL);
	return self;
}

static void *dataSubjectDtor(void *_self) {
	_DataSubject *self = _self;
	Delete(self->obvs);
	self->obvs = NULL;
	return self;
}

static void dataSubjectAttach(void *_self, void *_obv) {
	_DataSubject *self = _self;
	Insert(self->obvs, _obv);
}

static void dataSubjectDetach(void *_self, void *_obv) {
	_DataSubject *self = _self;
	Remove(self->obvs, _obv);
}

static void update(void *_obv, va_list *params) {
	Observer **obv = _obv;
	void *sub = va_arg(*params, void*);

	assert(_obv && (*obv) && (*obv)->update);
	(*obv)->update(_obv, sub);
}

static void dataSubjectNotify(const void *_self) {
	const _DataSubject *self = _self;
	Iterator(self->obvs, update, _self);
}

static void dataSubjectSetState(void *_self, char *_st) {
	_DataSubject *self = _self;
	self->st = _st;
}

static char *dataSubjectGetState(const void *_self) {
	const _DataSubject *self = _self;
	return self->st;
}

static const Subject dataSubject = {
	sizeof(_DataSubject),
	dataSubjectCtor,
	dataSubjectDtor,
	dataSubjectAttach,
	dataSubjectDetach,
	dataSubjectNotify,
	dataSubjectSetState,
	dataSubjectGetState,
};

const void *DataSubject = &dataSubject;

void *New(const void *_class, ...) {
	const AbstractClass *class = _class;
	void *p = calloc(1, class->size);

	assert(p);
	*(const AbstractClass **)p = class;

	if (class->ctor) {
		va_list params;
		va_start(params, _class);
		p = class->ctor(p, &params);
		va_end(params);
	}
	return p;
}


void Delete(void *_class) {
	const AbstractClass *class = _class;

	if (_class && class && class->dtor) {
		_class = class->dtor(_class);
	}
	free(_class);
}

void SetState(void *_subject, char *_st) {
	Subject **subject = _subject;

	if (_subject && (*subject) && (*subject)->setstate) {
		(*subject)->setstate(_subject, _st);
	}
}

void Notify(const void *_subject) {
	const Subject * const *subject = _subject;
	if (_subject && (*subject) && (*subject)->notify) {
		(*subject)->notify(_subject);
	}
}

void Update(void *_observer, const void *_subject) {
	Observer **observer = _observer;
	if (_observer && (*observer) && (*observer)->update) {
		(*observer)->update(_observer, _subject);
	}
}

void Insert(void *_list, void *_item) {
	((const List*)SingleList)->insert(_list, _item);
}

void Remove(void *_list, void *_item) {
	((const List*)SingleList)->remove(_list, _item);
}

void Iterator(const void *_list, Handle_FN handle_fn, ...) {
	va_list params;
	va_start(params, handle_fn);
	((const List*)SingleList)->iterator(_list, handle_fn, &params);
	va_end(params);
}

void Print(void *_list, Print_FN print_fn) {
	((const List*)SingleList)->print(_list, print_fn);
}

int main(int argc, char *argv[]) {
	void *sub = New(DataSubject, NULL);
	void *o1 = New(SheetObserver, sub);
	void *o2 = New(ChatObserver, sub);
	void *o3 = New(WordObserver, sub);

	SetState(sub, "old data");
	Notify(sub);

	SetState(sub, "new data");
	Notify(sub);

	//Update(o1, sub);
	//Update(o2, sub);

	return 0;
}

