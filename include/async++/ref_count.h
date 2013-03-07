// Copyright (c) 2013 Amanieu d'Antras
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef ASYNCXX_H_
# error "Do not include this header directly, include <async++.h> instead."
#endif

namespace async {
namespace detail {

// Reference-counted object base class
template<typename T> struct ref_count_base {
	std::atomic<unsigned int> ref_count;

	// By default the reference count is initialized to 1
	explicit ref_count_base(unsigned int count = 1): ref_count(count) {}

	void add_ref(unsigned int count = 1)
	{
		ref_count.fetch_add(count, std::memory_order_relaxed);
	}
	void release(unsigned int count = 1)
	{
		if (ref_count.fetch_sub(count, std::memory_order_release) == count) {
			std::atomic_thread_fence(std::memory_order_acquire);
			delete static_cast<T*>(this);
		}
	}
	void add_ref_unlocked()
	{
		ref_count.store(ref_count.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
	}
};

// Pointer to reference counted object, based on boost::intrusive_ptr
template<typename T> class ref_count_ptr {
	T* p;

public:
	// Note that this doesn't increment the reference count, instead it takes
	// ownership of a pointer which you already own a reference to.
	explicit ref_count_ptr(T* t): p(t) {}

	ref_count_ptr(): p(nullptr) {}
	ref_count_ptr(std::nullptr_t): p(nullptr) {}
	ref_count_ptr(const ref_count_ptr& other)
		: p(other.p)
	{
		if (p)
			p->add_ref();
	}
	ref_count_ptr(ref_count_ptr&& other) noexcept
		: p(other.p)
	{
		other.p = nullptr;
	}
	ref_count_ptr& operator=(std::nullptr_t)
	{
		if (p)
			p->release();
		p = nullptr;
		return *this;
	}
	ref_count_ptr& operator=(const ref_count_ptr& other)
	{
		if (p)
			p->release();
		p = other.p;
		if (p)
			p->add_ref();
		return *this;
	}
	ref_count_ptr& operator=(ref_count_ptr&& other) noexcept
	{
		std::swap(p, other.p);
		return *this;
	}
	~ref_count_ptr()
	{
		if (p)
			p->release();
	}

	T& operator*() const
	{
		return *p;
	}
	T* operator->() const
	{
		return p;
	}
	T* get() const
	{
		return p;
	}

	explicit operator bool() const
	{
		return p != nullptr;
	}
};

} // namespace detail
} // namespace async