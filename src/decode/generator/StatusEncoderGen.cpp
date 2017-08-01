/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/generator/StatusEncoderGen.h"
#include "decode/generator/TypeReprGen.h"
#include "decode/generator/IncludeCollector.h"
#include "decode/ast/Component.h"

#include <unordered_set>
#include <string>
#include <cassert>

namespace decode {

StatusEncoderGen::StatusEncoderGen(TypeReprGen* reprGen, SrcBuilder* output)
    : _typeReprGen(reprGen)
    , _output(output)
    , _inlineSer(reprGen, output)
{
}

StatusEncoderGen::~StatusEncoderGen()
{
}

void StatusEncoderGen::generateHeader(CompAndMsgVecConstRange messages)
{
    _output->startIncludeGuard("PRIVATE", "STATUS_ENCODER");
    _output->appendEol();

    _output->appendLocalIncludePath("core/Error");
    _output->appendLocalIncludePath("core/Writer");
    _output->appendEol();
    _output->startCppGuard();
    _output->appendEol();

    std::size_t n = 0;
    for (const ComponentAndMsg& msg : messages) {
        _output->appendModIfdef(msg.component->moduleName());
        appendStatusMessageGenFuncDecl(msg.component.get(), n);
        _output->append(";\n");
        _output->appendEndif();
        n++;
    }

    _output->appendEol();
    _output->endCppGuard();
    _output->appendEol();
    _output->endIncludeGuard();
}

void StatusEncoderGen::generateSource(CompAndMsgVecConstRange messages)
{
    IncludeCollector coll;
    std::unordered_set<std::string> includes;
    std::unordered_set<Rc<Component>> comps;

    for (const char* inc : {"core/Writer", "core/Error", "core/Try"}) {
        _output->appendLocalIncludePath(inc);
    }
    for (const ComponentAndMsg& msg : messages) {
        //TODO: remove duplicates
        comps.insert(msg.component);

    }
    for (const Rc<Component>& comp : comps) {
        coll.collectStatuses(comp->statusesRange(), &includes);

        _output->appendModIfdef(comp->moduleName());
        for (const std::string& inc : includes) {
            _output->appendLocalIncludePath(inc);
        }
        _output->append("#include \"photon/");
        _output->append(comp->moduleName());
        _output->append('/');
        _output->appendWithFirstUpper(comp->moduleName());
        _output->append(".Component.h\"\n");
        _output->appendEndif();

        includes.clear();
    }
    _output->appendEol();

    std::size_t n = 0;
    for (const ComponentAndMsg& msg : messages) {
        _output->appendModIfdef(msg.component->moduleName());
        appendStatusMessageGenFuncDecl(msg.component.get(), n);
        _output->append("\n{\n");

        for (const StatusRegexp* part : msg.msg->partsRange()) {
            appendInlineSerializer(msg.component.get(), part);
        }
        _output->append("    return PhotonError_Ok;\n}\n");
        _output->appendEndif();
        _output->appendEol();
        n++;
    }
}

void StatusEncoderGen::appendInlineSerializer(const Component* comp, const StatusRegexp* part)
{
    if (!part->hasAccessors()) {
        return;
    }
    assert(part->accessorsBegin()->accessorKind() == AccessorKind::Field);
    AccessorKind lastKind = AccessorKind::Field;

    InlineSerContext ctx;
    StringBuilder currentField("_photon");
    currentField.appendWithFirstUpper(comp->moduleName());
    const Type* lastType;
    for (const Accessor* acc : part->accessorsRange()) {
        switch (acc->accessorKind()) {
        case AccessorKind::Field: {
            auto facc = static_cast<const FieldAccessor*>(acc);
            currentField.append('.');
            currentField.append(facc->field()->name());
            lastType = facc->field()->type();
            break;
        }
        case AccessorKind::Subscript: {
            auto sacc = static_cast<const SubscriptAccessor*>(acc);
            const Type* type = sacc->type();
            if (type->isSlice()) {
                _output->appendIndent(ctx);
                _output->append("PHOTON_TRY(PhotonWriter_WriteVaruint(dest, ");
                _output->append(currentField.view());
                _output->append(".size));\n");
            }
            //TODO: add bounds checking for slices
            if (type->isSlice()) {
                lastType = type->asSlice()->elementType();
            } else if (type->isArray()) {
                lastType = type->asArray()->elementType();
            } else {
                assert(false);
            }
            if (sacc->isRange()) {
                const Range& range = sacc->asRange();
                _output->appendIndent(ctx);
                _output->append("for (size_t ");
                _output->append(ctx.currentLoopVar());
                _output->append(" = ");
                if (range.lowerBound.isSome()) {
                    _output->appendNumericValue(range.lowerBound.unwrap());
                } else {
                    _output->appendNumericValue(0);
                }
                _output->append("; ");
                _output->append(ctx.currentLoopVar());
                _output->append(" < ");
                if (range.upperBound.isSome()) {
                    _output->appendNumericValue(range.upperBound.unwrap());
                } else {
                    if (type->isArray()) {
                        _output->appendNumericValue(type->asArray()->elementCount());
                    } else if (type->isSlice()) {
                        _output->append(currentField.view());
                        _output->append(".size");
                    } else {
                        assert(false);
                    }
                }
                _output->append("; ");
                _output->append(ctx.currentLoopVar());
                _output->append("++) {\n");

                if (type->isSlice()) {
                    currentField.append(".data");
                }
                currentField.append('[');
                currentField.append(ctx.currentLoopVar());
                currentField.append(']');
                ctx = ctx.incLoopVar().indent();
            } else {
                if (type->isSlice()) {
                    currentField.append(".data");
                }
                uintmax_t i = sacc->asIndex();
                currentField.append('[');
                currentField.appendNumericValue(i);
                currentField.append(']');
            }

            break;
        }
        default:
            assert(false);
        }
        lastKind = acc->accessorKind();
    }
    _inlineSer.inspect(lastType, ctx, currentField.view());
    for (std::size_t indent = ctx.indentLevel; indent > 1; indent--) {
        _output->appendIndent(indent - 1);
        _output->append("}\n");
    }
}

void StatusEncoderGen::genTypeRepr(const Type* type, bmcl::StringView fieldName)
{
    _typeReprGen->genTypeRepr(type, fieldName);
}
}
