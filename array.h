#ifndef __ARRAY_H__
#define __ARRAY_H__

#include <assert.h>

template <class T>
class Array {
	int _size;
	T *_p;
public:
	Array(int size):
		_size(size),
		_p(new T[size]) {}
	~Array() { delete [] _p; }
	T& operator[](int i) { assert(i>=0 && i<_size); return _p[i]; }
	operator T*() { return _p; }
};

#endif
