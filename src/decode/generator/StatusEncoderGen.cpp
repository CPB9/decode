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
#include "decode/generator/IncludeCollector.h"
#include "decode/ast/Component.h"
#include "decode/ast/ModuleInfo.h"
#include "decode/parser/Containers.h"

#include <string>
#include <cassert>

namespace decode {

StatusEncoderGen::StatusEncoderGen(TypeReprGen* reprGen, SrcBuilder* output)
    : _typeReprGen(reprGen)
    , _output(output)
    , _inlineSer(reprGen, output)
    , _inlineDeser(reprGen, output)
    , _prototypeGen(reprGen, output)
{
}

StatusEncoderGen::~StatusEncoderGen()
{
}

void StatusEncoderGen::generateEncoderHeader(const Project* project)
{
    _output->startIncludeGuard("PRIVATE", "STATUS_ENCODER");
    _output->appendEol();

    _output->appendOnboardIncludePath("core/Error");
    _output->appendOnboardIncludePath("core/Writer");
    _output->appendEol();
    _output->startCppGuard();
    _output->appendEol();

    std::size_t n = 0;
    for (const ComponentAndMsg& msg : project->package()->statusMsgs()) {
        _output->appendModIfdef(msg.component->moduleName());
        _prototypeGen.appendStatusMessageGenFuncDecl(msg.component.get(), n);
        _output->append(";\n");
        _output->appendEndif();
        n++;
    }

    _output->appendEol();
    _output->endCppGuard();
    _output->appendEol();
    _output->endIncludeGuard();
}

void StatusEncoderGen::generateDecoderHeader(const Project* project)
{
    _output->startIncludeGuard("PRIVATE", "STATUS_DECODER");

    _output->appendOnboardIncludePath("core/Error");
    _output->appendOnboardIncludePath("core/Reader");
    _output->appendEol();

    for (const Component* comp : project->package()->components()) {
        _output->appendSourceModIfdef(comp->moduleName());
        _output->appendComponentInclude(comp->moduleName());
        _output->appendEndif();
    }
    _output->appendEol();

    for (const Device* dev : project->devices()) {
        _output->appendSourceDeviceIfdef(dev->name);
        _output->append("typedef struct {\n");
        for (const Rc<Ast>& ast : dev->modules) {
            if (ast->component().isNone()) {
                continue;
            }
            if (!ast->component()->hasParams()) {
                continue;
            }
            _output->appendSourceModIfdef(ast->component()->moduleName());
            _output->append("    Photon");
            _output->appendWithFirstUpper(ast->component()->moduleName());
            _output->appendSpace();
            _output->appendWithFirstLower(ast->component()->moduleName());
            _output->append(";\n");
            _output->appendEndif();
        }
        _output->append("} Photon_");
        _output->appendWithFirstUpper(dev->name);
        _output->append("TmState;\n\n");
        _output->append("extern Photon_");
        _output->appendWithFirstUpper(dev->name);
        _output->append("TmState _photonTmState_");
        _output->appendWithFirstUpper(dev->name);
        _output->append(";\n");
        _output->appendEndif();
        _output->appendEol();
    }

    _output->startCppGuard();

    for (const Device* dev : project->devices()) {
        _output->appendSourceDeviceIfdef(dev->name);
        appendTmDecoderPrototype(dev->name);
        _output->append(";\n");
        _output->appendEndif();
    }
    _output->appendEol();
    _output->endCppGuard();

    for (const Device* dev : project->devices()) {
        _output->append("#ifdef PHOTON_DEVICE_");
        _output->appendUpper(dev->name);
        _output->appendEol();
        _output->appendSourceDeviceIfdef(dev->name);

        _output->append("typedef Photon_Tm");
        _output->appendWithFirstUpper(dev->name);
        _output->append("State Photon_TmState;\n");
        _output->append("#define Photon_DecodeTelemetry Photon_Decode");
        _output->appendWithFirstUpper(dev->name);
        _output->append("Telemetry\n");

        _output->appendEndif();
        _output->appendEndif();
    }
    _output->appendEol();


    _output->endIncludeGuard();
}

void StatusEncoderGen::appendTmDecoderPrototype(bmcl::StringView name)
{
    _output->append("PhotonError Photon_Decode");
    _output->appendWithFirstUpper(name);
    _output->append("Telemetry(");
    _output->append("PhotonReader* src, Photon_");
    _output->appendWithFirstUpper(name);
    _output->append("TmState* dest)");
}

void StatusEncoderGen::generateEncoderSource(const Project* project)
{
    IncludeCollector coll;
    HashSet<std::string> includes;

    for (const char* inc : {"core/Writer", "core/Error", "core/Try", "core/Logging"}) {
        _output->appendOnboardIncludePath(inc);
    }

    for (const Component* comp : project->package()->components()) {
        coll.collectStatuses(comp->statusesRange(), &includes);

        _output->appendModIfdef(comp->moduleName());
        for (const std::string& inc : includes) {
            _output->appendOnboardIncludePath(inc);
        }
        _output->appendComponentInclude(comp->moduleName());
        _output->appendEndif();

        includes.clear();
    }
    _output->appendEol();
    _output->append("#define _PHOTON_FNAME \"StatusEncoder.Private.c\"\n\n");

    std::size_t n = 0;
    for (const ComponentAndMsg& msg : project->package()->statusMsgs()) {
        _output->appendModIfdef(msg.component->moduleName());
        _prototypeGen.appendStatusMessageGenFuncDecl(msg.component.get(), n);
        _output->append("\n{\n");

        for (const StatusRegexp* part : msg.msg->partsRange()) {
            SrcBuilder currentField("_photon");
            currentField.appendWithFirstUpper(msg.component->moduleName());
            appendInlineSerializer(part, &currentField, true);
        }
        _output->append("    return PhotonError_Ok;\n}\n");
        _output->appendEndif();
        _output->appendEol();
        n++;
    }
    _output->append("#undef _PHOTON_FNAME\n");
}

void StatusEncoderGen::generateDecoderSource(const Project* project)
{
    _output->append("#include \"photon/StatusDecoder.Private.h\"\n\n");
    _output->appendOnboardIncludePath("core/Try");
    _output->appendOnboardIncludePath("core/Logging");
    _output->appendEol();

    _output->append("#define _PHOTON_FNAME \"StatusDecoder.Private.c\"\n\n");

    for (const Component* comp : project->package()->components()) {
        if (!comp->hasStatuses()) {
            continue;
        }
        _output->appendSourceModIfdef(comp->moduleName());

        for (const StatusMsg* msg : comp->statusesRange()) {
            _output->append("static PhotonError ");
            _output->append("decode");
            _output->appendWithFirstUpper(comp->moduleName());
            _output->append("Msg");
            _output->appendNumericValue(msg->number());
            _output->append("(PhotonReader* src, Photon");
            _output->appendWithFirstUpper(comp->moduleName());
            _output->append("* dest)\n{\n");

            for (const StatusRegexp* part : msg->partsRange()) {
                SrcBuilder currentField("(*dest)");
                appendInlineSerializer(part, &currentField, false);
            }
            _output->append("    return PhotonError_Ok;\n}\n\n");
        }

        _output->append("static PhotonError decode");
        _output->appendWithFirstUpper(comp->moduleName());
        _output->append("Telemetry(PhotonReader* src, Photon");
        _output->appendWithFirstUpper(comp->moduleName());
        _output->append("* dest)\n{\n");

        _output->append("    uint64_t id;\n");
        _output->append("    PHOTON_TRY(PhotonReader_ReadVaruint(src, &id));\n");
        _output->append("    switch (id) {\n");

        for (const StatusMsg* msg : comp->statusesRange()) {
            _output->append("    case ");
            _output->appendNumericValue(msg->number());
            _output->append(":\n");
            _output->append("        return decode");
            _output->appendWithFirstUpper(comp->moduleName());
            _output->append("Msg");
            _output->appendNumericValue(msg->number());
            _output->append("(src, dest);\n");
        }
        _output->append("    }\n");

        _output->append("    return PhotonError_InvalidMessageId;\n}\n");
        _output->appendEndif();
        _output->appendEol();
    }

    for (const Device* dev : project->devices()) {
        _output->appendSourceDeviceIfdef(dev->name);
        appendTmDecoderPrototype(dev->name);

        _output->append("\n{\n");
        _output->append("    uint64_t id;\n");
        _output->append("    PHOTON_TRY(PhotonReader_ReadVaruint(src, &id));\n");
        _output->append("    switch (id) {\n");

        for (const Rc<Ast>& ast : dev->modules) {
            if (ast->component().isNone()) {
                continue;
            }
            const Component* comp = ast->component().unwrap();
            if (!comp->hasStatuses()) {
                continue;
            }
            _output->appendSourceModIfdef(comp->moduleName());
            _output->append("    case PHOTON_");
            _output->appendUpper(comp->moduleName());
            _output->append("_COMPONENT_ID:\n");
            _output->append("        return decode");
            _output->appendWithFirstUpper(comp->moduleName());
            _output->append("Telemetry(src, &dest->");
            _output->appendWithFirstLower(comp->moduleName());
            _output->append(");\n");
            _output->appendEndif();
        }

        _output->append("    }\n");
        _output->append("    return PhotonError_InvalidComponentId;\n}\n");
        _output->appendEndif();
        _output->appendEol();
    }

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
        _inlineSer.inspect(lastType, ctx, currentField->view());
    } else {
        _inlineDeser.inspect(lastType, ctx, currentField->view());
    }
    for (std::size_t indent = ctx.indentLevel; indent > 1; indent--) {
        _output->appendIndent(indent - 1);
        _output->append("}\n");
    }
}
}
