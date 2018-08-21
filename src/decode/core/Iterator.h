/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <decode/Config.h>

#include <type_traits>
#include <iterator>
#include <algorithm>

namespace decode {

template <typename R, typename T>
struct AddConst {}; //TODO: rename

template <typename R, typename T>
struct AddConst<const R&, T> { using type = const T; };

template <typename R, typename T>
struct AddConst<R&, T> { using type = T; };

template <typename I, typename B>
class IteratorAdaptor : public I {
public:
    using size_type = std::size_t;

#ifdef _MSC_VER
    using _Unchecked_type = B;

    constexpr B _Unwrapped() const noexcept
    {
        return *this;
    }
#endif

    IteratorAdaptor()
    {
    }

    IteratorAdaptor(const I& it)
        : I(it)
    {
    }

    IteratorAdaptor(const IteratorAdaptor& it)
        : I(it.asBase())
    {
    }

    B& operator++()
    {
        ++asBase();
        return *static_cast<B*>(this);
    }

    B operator++(int)
    {
        return asBase()++;
    }

    B& operator+=(size_type size)
    {
        asBase() += size;
    }

    B& operator-=(size_type size)
    {
        asBase() -= size;
    }

    friend bool operator==(const B& left, const B& right)
    {
        return left.asBase() == right.asBase();
    }

    friend bool operator!=(const B& left, const B& right)
    {
        return left.asBase() != right.asBase();
    }

    friend bool operator<(const B& left, const B& right)
    {
        return left.asBase() < right.asBase();
    }

    friend bool operator>(const B& left, const B& right)
    {
        return left.asBase() > right.asBase();
    }

    friend bool operator<=(const B& left, const B& right)
    {
        return left.asBase() <= right.asBase();
    }

    friend bool operator>=(const B& left, const B& right)
    {
        return left.asBase() >= right.asBase();
    }

    friend B operator+(const B& it, typename I::difference_type size)
    {
        return it.asBase() + size;
    }

    friend B operator+(size_type size, const B& it)
    {
        return size + it.asBase();
    }

    friend B operator-(const B& it, typename I::difference_type size)
    {
        return it.asBase() - size;
    }

    friend typename I::difference_type operator-(const B& left, const B& right)
    {
        return left.asBase() - right.asBase();
    }

    friend void swap(B& left, B& right)
    {
        std::swap(left.asBase(), right.asBase());
    }

protected:
    I& asBase()
    {
        return *static_cast<I*>(this);
    }

    const I& asBase() const
    {
        return *static_cast<const I*>(this);
    }
};
}

namespace std {

template<typename I, typename B>
typename decode::IteratorAdaptor<I, B>::difference_type
distance(decode::IteratorAdaptor<I, B> first, decode::IteratorAdaptor<I, B> last)
{
    return distance(first.iterator(), last.iterator());
}

template<typename I, typename B>
struct iterator_traits<decode::IteratorAdaptor<I, B>> {
    using difference_type = typename B::difference_type;
    using value_type = typename B::value_type;
    using pointer = typename B::pointer;
    using reference = typename B::reference;
    using iterator_category = typename B::iterator_category;
};
}

namespace decode {

template <typename I>
class SmartPtrIteratorAdaptor : public IteratorAdaptor<I, SmartPtrIteratorAdaptor<I>> {
private:
    using PtrType = typename AddConst<typename I::reference, typename I::value_type::value_type>::type*;
    using Base = IteratorAdaptor<I, SmartPtrIteratorAdaptor<I>>;

public:
    //TODO: value_type
    using reference = PtrType;
    using pointer = PtrType;
    using size_type = typename Base::size_type;
    using iterator_category = typename I::iterator_category;

    SmartPtrIteratorAdaptor()
    {
    }

    template <typename T>
    SmartPtrIteratorAdaptor(const T& it)
        : Base(it)
    {
    }

    SmartPtrIteratorAdaptor(const SmartPtrIteratorAdaptor& it)
        : Base(it.asBase())
    {
    }

    PtrType operator*() const
    {
        return Base::operator*().get();
    }

    PtrType operator->() const
    {
        return Base::operator*().get();
    }

    PtrType operator[](size_type size) const
    {
        return Base::operator[](size).get();
    }
};

template <typename I>
class PairSecondIteratorAdaptor : public IteratorAdaptor<I, PairSecondIteratorAdaptor<I>> {
private:
    using Base = IteratorAdaptor<I, PairSecondIteratorAdaptor<I>>;
    using SecondType = typename I::value_type::second_type;

public:
    using value_type = typename AddConst<typename I::reference, SecondType>::type;
    using size_type = typename Base::size_type;
    using reference = value_type&;
    using pointer = value_type*;
    using iterator_category = typename I::iterator_category;

    PairSecondIteratorAdaptor()
    {
    }

    template <typename T>
    PairSecondIteratorAdaptor(const T& it)
        : Base(it)
    {
    }

    PairSecondIteratorAdaptor(const PairSecondIteratorAdaptor& it)
        : Base(it.asBase())
    {
    }

    reference operator*() const
    {
        return Base::operator*().second;
    }

    pointer operator->() const
    {
        return Base::operator->().second;
    }

    reference operator[](size_type size) const
    {
        return Base::operator[](size).second;
    }
};

template <typename I>
class IteratorRange {
public:
    IteratorRange()
    {
    }

    template <typename C>
    IteratorRange(C&& container)
        : _begin(container.begin())
        , _end(container.end())
    {
    }

    IteratorRange(I begin, I end)
        : _begin(begin)
        , _end(end)
    {
    }

    I begin() const
    {
        return _begin;
    }

    I end() const
    {
        return _end;
    }

    bool empty() const
    {
        return _begin == _end;
    }

    bool isEmpty() const
    {
        return empty();
    }

    std::size_t size() const
    {
        return std::distance(_begin, _end);
    }

    template <typename C>
    I find(const C& value)
    {
        return std::find(_begin, _end, value);
    }

    template <typename C>
    I findIf(C&& callable)
    {
        return std::find_if(_begin, _end, std::forward<C>(callable));
    }

private:
    I _begin;
    I _end;
};
}
