#pragma once

/* 迭代器、仿函数、配接器、空间配置器、算法、容器*/

#include <iostream>



/* vector 特性
	1. 支持随机访问 只要知道元素类型大小和起始地址 访问vector[i] O(1)时间
	2. 缓存友好 vector对象存放在一片内存的连续区域上，局部性原理，缓存命中率大大增加
 */



 /* vector剖析和源码实现
 *
 */


namespace lowindow {

	template <class T>
	class myvector {

	public:
		myvector()
			:_capacity(0)
			, _size(0)
			, _start(nullptr)
		{
		}


		myvector(const size_t& size, const T& t = T())
			:_capacity(size)
			, _size(size)
		{
			_start = new T[_size];

			for (size_t i = 0; i < _size; i++) {
				_start[i] = t;
			}

		}

		myvector(const myvector<T>& a)
			:_capacity(0)
			, _size(0)
		{
			reserve(a.size());
			_size = a.size();
			for (size_t i = 0; i < a.size(); i++) {
				_start[i] = a.getstart()[i];
			}

		}

		myvector& operator=(const myvector& other) {
			if (this != &other) {
				myvector tmp(other);      // 复拷贝构造
				std::swap(_start, tmp._start);
				std::swap(_size, tmp._size);
				std::swap(_capacity, tmp._capacity);
			}
			return *this;
		}

		size_t size() const {
			return _size;
		}

		size_t capacity() const {
			return _capacity;
		}

		bool empty() const {
			return _size == 0;
		}

		T& operator[](size_t idx) {

			if (idx >= _size)
				throw std::runtime_error("myvector::operator[] idx invalidation!");
			return _start[idx];
		}

		const T& operator[](size_t idx) const {
			if (idx >= _size)
				throw std::runtime_error("myvector::operator[] idx invalidation!");
			return _start[idx];
		}

		void reserve(size_t n) {
			if (n > _capacity) {
				size_t oldsize = size();
				T* tmp = new T[n];
				for (size_t i = 0; i < oldsize; i++) {
					tmp[i] = std::move(_start[i]);
				}
				delete[] _start;
				_start = tmp;
				_capacity = n;
			}
		}

		void resize(size_t n, const T& t = T()) {
			size_t oldsize = size();

			if (n <= _capacity) {
				_size = n;
			}
			else {
				reserve(n);

			}
			_size = n;
			for (size_t i = oldsize; i < n; i++) {
				_start[i] = t;
			}


		}

		void push_back(const T& t) {
			if (size() == capacity()) {
				reserve(capacity() == 0 ? 4 : capacity() * 2);
			}
			_start[_size] = t;
			++_size;
		}

		void pop_back() {
			if (empty())
				throw std::runtime_error("myvector::pop_back() called on empty vector");
			--_size;
		}

		void insert(size_t pos, const T& t) {
			if (pos > _size)
				throw std::runtime_error("myvector::insert() pos invalidation!");

			push_back(t);

			for (size_t i = _size - 1; i > pos; i--) {
				_start[i] = _start[i - 1];
			}

			_start[pos] = t;


		}

		void erase(size_t pos) {
			if (pos >= _size)
				throw std::runtime_error("myvector::erase() pos invalidation!");

			for (size_t i = pos; i < _size - 1; i++) {
				_start[i] = _start[i + 1];
			}

			pop_back();
		}


		const T& front()  const {
			if (empty())
				throw std::runtime_error("myvector::front() called on empty vector");
			return _start[0];
		}

		const T& back()  const {
			if (empty())
				throw std::runtime_error("myvector::back() called on empty vector");
			return _start[_size - 1];
		}

		T& front() {
			if (empty())
				throw std::runtime_error("myvector::front() called on empty vector");
			return _start[0];
		}

		T& back() {
			if (empty())
				throw std::runtime_error("myvector::back() called on empty vector");
			return _start[_size - 1];
		}



		T* getstart() const {
			return _start;
		}






		~myvector() {
			_capacity = 0;
			_size = 0;
			delete[] _start;
			_start = nullptr;
		}


	private:
		size_t _capacity;
		size_t _size;
		T* _start;
	};

}