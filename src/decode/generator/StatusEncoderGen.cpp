/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/generator/StatusEncoderGen.h"
#include "decode/core/HashSet.h"
#include "decode/parser/Project.h"
#include "decode/parser/Package.h"
#include "decode/generator/TypeReprGen.h"
#include "decode/generator/IncludeGen.h"
#include "decode/generator/TypeDependsCollector.h"
#include "decode/generator/Utils.h"
#include "decode/generator/FuncPrototypeGen.h"
#include "decode/ast/Component.h"
#include "decode/ast/ModuleInfo.h"
#include "decode/parser/Containers.h"

#include <string>
#include <cassert>

namespace decode {

StatusEncoderGen::StatusEncoderGen(SrcBuilder* output)
    : _output(output)
    , _inlineInspector(output)
    , _prototypeGen(output)
{
}

StatusEncoderGen::~StatusEncoderGen()
{
}

static void appendTmDeserializerPrototype(SrcBuilder* dest)
{

    dest->append("PhotonError Photon_DeserializeTelemetry("
                    "PhotonReader* src, void (*handler)(uint8_t compNum, uint8_t msgNum, "
                    "const void* msg, void* userData), void* userData)");
}

void StatusEncoderGen::generateStatusDecoderHeader(const Project* project)
{
    _output->startIncludeGuard("PRIVATE", "STATUS_DECODER");

    _output->appendOnboardIncludePath("core/Error");
    _output->appendOnboardIncludePath("core/Reader");
    _output->appendEol();

    _output->startCppGuard();

    appendTmDeserializerPrototype(_output);
    _output->append(";\n\n");

    _output->endCppGuard();

    _output->endIncludeGuard();
}

void StatusEncoderGen::generateStatusEncoderSource(const Project* project)
{
    TypeDependsCollector coll;
    TypeDependsCollector::Depends includes;
    IncludeGen includeGen(_output);

    for (const char* inc : {"core/Writer", "core/Error", "core/Try", "core/Logging"}) {
        _output->appendOnboardIncludePath(inc);
    }

    for (const Component* comp : project->package()->components()) {
        coll.collectStatuses(comp->statusesRange(), &includes);

        _output->appendModIfdef(comp->moduleName());
        includeGen.genOnboardIncludePaths(&includes);
        _output->appendComponentInclude(comp->moduleName(), ".h");
        _output->appendEndif();

        includes.clear();
    }
    _output->appendEol();
    _output->append("#define _PHOTON_FNAME \"StatusEncoder.c\"\n\n");

    for (const ComponentAndMsg& msg : project->package()->statusMsgs()) {
        _output->appendModIfdef(msg.component->moduleName());
        _prototypeGen.appendStatusEncoderFunctionPrototype(msg.component.get(), msg.msg.get());
        _output->append("\n{\n");

        for (const StatusRegexp* part : msg.msg->partsRange()) {
            SrcBuilder currentField("_photon");
            currentField.appendWithFirstUpper(msg.component->moduleName());
            appendInlineSerializer(part, &currentField, true);
        }
        _output->append("    return PhotonError_Ok;\n}\n");
        _output->appendEndif();
        _output->appendEol();
    }
    _output->append("#undef _PHOTON_FNAME\n");
}

static void appendInfix(SrcBuilder* dest, const StatusMsg*)
{
    dest->append("_StatusMsg_");
}

static void appendInfix(SrcBuilder* dest, const EventMsg*)
{
    dest->append("_EventMsg_");
}

static void appendPrototype(FuncPrototypeGen* dest, const Component* comp, const StatusMsg* msg)
{
    dest->appendStatusDecoderFunctionName(comp, msg);
}

static void appendPrototype(FuncPrototypeGen* dest, const Component* comp, const EventMsg* msg)
{
    dest->appendEventDecoderFunctionName(comp, msg);
}

template <typename T>
void StatusEncoderGen::appendMsgSwitch(const Component* comp, const T* msg)
{
    _output->append("            case ");
    _output->appendNumericValue(msg->number());
    _output->append(": {\n"
                    "                Photon");

    _output->appendWithFirstUpper(comp->moduleName());
    appendInfix(_output, msg);
    _output->appendWithFirstUpper(msg->name());

    _output->append(" msg;\n"
                    "                PHOTON_TRY(");

    appendPrototype(&_prototypeGen, comp, msg);

    _output->append("(src, &msg));\n"
                    "                handler(id, num, &msg, userData);\n"
                    "                continue;\n"
                    "            }\n");
}

void StatusEncoderGen::generateStatusDecoderSource(const Project* project)
{
    _output->append("#include \"photon/StatusDecoder.h\"\n\n");
    _output->appendOnboardIncludePath("core/Try");
    _output->appendOnboardIncludePath("core/Logging");
    _output->appendEol();

    for (const Component* comp : project->package()->components()) {
        _output->appendSourceModIfdef(comp->moduleName());
        _output->appendComponentInclude(comp->moduleName(), ".h");
        _output->appendEndif();
    }
    _output->appendEol();

    _output->append("#define _PHOTON_FNAME \"StatusDecoder.c\"\n\n");

    StringBuilder fieldName;
    fieldName.reserve(31);
    InlineSerContext ctx;

    for (const Component* comp : project->package()->components()) {
        if (!comp->hasStatuses()) {
            continue;
        }
        _output->appendSourceModIfdef(comp->moduleName());

        for (const StatusMsg* msg : comp->statusesRange()) {
            _prototypeGen.appendStatusDecoderFunctionPrototype(comp, msg);
            _output->append("\n{\n");

            for (const StatusRegexp* part : msg->partsRange()) {
                fieldName.assign("dest->");
                part->buildFieldName(&fieldName);
                _inlineInspector.inspect<true, false>(part->type(), ctx, fieldName.view());
                fieldName.resize(6);
            }
            _output->append("    return PhotonError_Ok;\n}\n\n");
        }

        for (const EventMsg* msg : comp->eventsRange()) {
            _prototypeGen.appendEventDecoderFunctionPrototype(comp, msg);
            _output->append("\n{\n");
            for (const Field* part : msg->partsRange()) {
                fieldName.assign("dest->");
                fieldName.append(part->name());
                _inlineInspector.genOnboardDeserializer(part->type(), ctx, fieldName.view());
            }

            _output->append("    return PhotonError_Ok;\n}\n\n");
        }

        _output->appendEndif();
        _output->appendEol();
    }

    appendTmDeserializerPrototype(_output);

    _output->append("\n{\n");
    _output->append("    uint64_t id;\n");
    _output->append("    uint64_t num;\n");
    _output->append("    while (PhotonReader_ReadableSize(src) != 0) {\n");
    _output->append("        PHOTON_TRY(PhotonReader_ReadVaruint(src, &id));\n");
    _output->append("        PHOTON_TRY(PhotonReader_ReadVaruint(src, &num));\n");
    _output->append("        switch (id) {\n");

    for (const Component* comp : project->package()->components()) {
        _output->append("        case ");
        _output->appendNumericValue(comp->number());
        _output->append(": {\n");

        _output->appendSourceModIfdef(comp->moduleName());
        _output->append("            switch (num) {\n");
        for (const StatusMsg* msg : comp->statusesRange()) {
            appendMsgSwitch(comp, msg);
        }
        for (const EventMsg* msg : comp->eventsRange()) {
            appendMsgSwitch(comp, msg);
        }

        _output->append("            default:\n"
                        "                PHOTON_CRITICAL(\"Recieved invalid message id\");\n"
                        "                return PhotonError_InvalidMessageId;\n"
                        "            }\n"
                        "#else\n"
                        "            PHOTON_CRITICAL(\"Recieved invalid component id\");\n"
                        "            return PhotonError_InvalidComponentId;\n");

        _output->appendEndif();
        _output->append("        }\n");


    }

    _output->append("        default:\n"
                    "            PHOTON_CRITICAL(\"Recieved invalid component id\");\n"
                    "            return PhotonError_InvalidComponentId;\n"
                    "        }\n"
                    "    }\n"
                    "    return PhotonError_Ok;\n}\n");
    _output->appendEol();

    _output->append("#undef _PHOTON_FNAME\n");
}

void StatusEncoderGen::appendInlineSerializer(const StatusRegexp* part, SrcBuilder* currentField, bool isSerializer)
{
    if (!part->hasAccessors()) {
        return;
    }
    assert(part->accessorsBegin()->accessorKind() == AccessorKind::Field);

    InlineSerContext ctx;
    const Type* lastType;
    for (const Accessor* acc : part->accessorsRange()) {
        switch (acc->accessorKind()) {
        case AccessorKind::Field: {
            auto facc = static_cast<const FieldAccessor*>(acc);
            currentField->append('.');
            currentField->append(facc->field()->name());
            lastType = facc->field()->type();
            break;
        }
        case AccessorKind::Subscript: {
            auto sacc = static_cast<const SubscriptAccessor*>(acc);
            const Type* type = sacc->type();
            if (type->isDynArray()) {
                _output->appendIndent(ctx);
                if (isSerializer) {
                    _output->append("PHOTON_TRY_MSG(PhotonWriter_WriteVaruint(dest, ");
                    _output->append(currentField->view());
                    _output->append(".size), \"Failed to write dynarray size\");\n");
                } else {
                    _output->append("PHOTON_TRY_MSG(PhotonReader_ReadVaruint(src, &");
                    _output->append(currentField->view());
                    _output->append(".size), \"Failed to read dynarray size\");\n");
                }
            }
            //TODO: add bounds checking for dynArrays
            if (type->isDynArray()) {
                lastType = type->asDynArray()->elementType();
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
                    } else if (type->isDynArray()) {
                        _output->append(currentField->view());
                        _output->append(".size");
                    } else {
                        assert(false);
                    }
                }
                _output->append("; ");
                _output->append(ctx.currentLoopVar());
                _output->append("++) {\n");

                if (type->isDynArray()) {
                    currentField->append(".data");
                }
                currentField->append('[');
                currentField->append(ctx.currentLoopVar());
                currentField->append(']');
                ctx = ctx.incLoopVar().indent();
            } else {
                if (type->isDynArray()) {
                    currentField->append(".data");
                }
                uintmax_t i = sacc->asIndex();
                currentField->append('[');
                currentField->appendNumericValue(i);
                currentField->append(']');
            }

            break;
        }
        default:
            assert(false);
        }
    }
    if (isSerializer) {
        _inlineInspector.inspect<true, true>(lastType, ctx, currentField->view());
    } else {
        _inlineInspector.inspect<true, false>(lastType, ctx, currentField->view());
    }
    for (std::size_t indent = ctx.indentLevel; indent > 1; indent--) {
        _output->appendIndent(indent - 1);
        _output->append("}\n");
    }
}

void StatusEncoderGen::generateEventEncoderSource(const Project* project)
{
    for (const Ast* ast : project->package()->modules()) {
        _output->appendModIfdef(ast->moduleName());
        _output->append("#include \"photon/");
        _output->append(ast->moduleName());
        _output->append("/");
        _output->appendWithFirstUpper(ast->moduleName());
        _output->append(".Component.h\"\n");
        _output->appendEndif();
    }
    _output->appendEol();

    TypeReprGen reprGen(_output);
    for (const Component* comp : project->package()->components()) {
        if (!comp->hasEvents()) {
            continue;
        }
        _output->appendModIfdef(comp->moduleName());
        for (const EventMsg* msg : comp->eventsRange()) {
            _prototypeGen.appendEventEncoderFunctionPrototype(comp, msg, &reprGen);
            _output->append("\n{\n    PhotonWriter* dest = PhotonTm_BeginEventMsg(");
            _output->appendNumericValue(comp->number());
            _output->append(", ");
            _output->appendNumericValue(msg->number());
            _output->append(");\n");

            InlineSerContext ctx;
            StringBuilder nameBuilder;
            nameBuilder.reserve(15);
            for (const Field* field : msg->partsRange()) {
                derefPassedVarNameIfRequired(field->type(), field->name(), &nameBuilder);
                _inlineInspector.inspect<true, true>(field->type(), ctx, nameBuilder.view());
                nameBuilder.clear();
            }

            _output->append("    PhotonTm_EndEventMsg();\n    return PhotonError_Ok;\n");
            _output->append("}\n");
        }
        _output->appendEndif();
        _output->appendEol();
    }
}
}
