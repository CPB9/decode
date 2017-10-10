/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/core/ProgressPrinter.h"

#include <iomanip>

namespace decode {

ProgressPrinter::ProgressPrinter(bool verbose)
    : _isVerbose(verbose)
{
    (void)_isVerbose;
}

void ProgressPrinter::printActionProgress(bmcl::StringView actionMsgPart, bmcl::StringView additionalMsgPart)
{
    constexpr const char* prefix = "> ";
    constexpr std::size_t actionSize = 10;

    _dest << bmcl::ColorAttr::Reset;
    _dest << prefix;
    _dest << bmcl::ColorAttr::FgGreen << bmcl::ColorAttr::Bright;
    _dest << std::setw(actionSize) << std::left;
    _dest << actionMsgPart.toStdString();
    _dest << bmcl::ColorAttr::Reset;
    _dest << ' ';
    _dest << additionalMsgPart.toStdString();
    _dest << std::endl;
}
}
