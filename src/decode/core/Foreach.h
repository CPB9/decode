#pragma once

namespace decode {

template <typename I, typename E, typename B>
inline void foreachList(I begin, I end, E&& elementFunc, B&& betweenFunc)
{
    if (begin == end) {
        return;
    }
    auto last = end - 1;
    for (auto it = begin; it < last; it++) {
        elementFunc(*it);
        betweenFunc(*it);
    }
    elementFunc(*last);
}

template <typename C, typename E, typename B>
inline void foreachList(C&& container, E&& elementFunc, B&& betweenFunc)
{
    foreachList(container.begin(), container.end(), std::forward<E>(elementFunc), std::forward<B>(betweenFunc));
}
}
