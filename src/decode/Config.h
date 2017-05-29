/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#ifdef _MSC_VER
# define DECODE_DECL_EXPORT __declspec(dllexport)
# define DECODE_DECL_IMPORT __declspec(dllimport)
#else
# define DECODE_DECL_EXPORT
# define DECODE_DECL_IMPORT
#endif

#ifdef BUILDING_DECODE
# define DECODE_EXPORT DECODE_DECL_EXPORT
#else
# define DECODE_EXPORT DECODE_DECL_IMPORT
#endif
