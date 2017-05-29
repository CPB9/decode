/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace decode {

template <typename I, typename E, typename B>
inline void foreachList(I begin, I end, E&& elementFunc, B&& betweenFunc)
{
    if (begin == end) {
        return;
    }
    auto last = end - 1;
    for (I it = begin; it < last; it++) {
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
