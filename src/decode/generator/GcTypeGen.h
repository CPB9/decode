/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"

#include <bmcl/Fwd.h>

namespace decode {

class Type;
class TopLevelType;
class EnumType;
class StructType;
class VariantType;
class NamedType;
class SrcBuilder;
class EnumConstant;
class GenericType;

class GcTypeGen {
public:
    GcTypeGen(SrcBuilder* output);
    ~GcTypeGen();

    void generateHeader(const TopLevelType* type);

private:
    void generateEnum(const EnumType* type, bmcl::OptionPtr<const GenericType> parent);
    void generateStruct(const StructType* type, bmcl::OptionPtr<const GenericType> parent);
    void generateVariant(const VariantType* type, bmcl::OptionPtr<const GenericType> parent);

    void appendTemplatePrefix(bmcl::OptionPtr<const GenericType> parent);
    void appendFullTypeName(const NamedType* type);
    void appendEnumConstantName(const EnumType* type, const EnumConstant* constant);

    void appendSerPrefix(const NamedType* type, bmcl::OptionPtr<const GenericType> parent, const char* prefix = "inline");
    void appendDeserPrefix(const NamedType* type, bmcl::OptionPtr<const GenericType> parent, const char* prefix = "inline");
    void appendDeserPrototype(const NamedType* type, bmcl::OptionPtr<const GenericType> parent, const char* prefix = "inline");

    void beginNamespace(bmcl::StringView modName);
    void endNamespace();

    SrcBuilder* _output;
};
}
