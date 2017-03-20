#include "decode/generator/StatusEncoderGen.h"
#include "decode/generator/TypeReprGen.h"
#include "decode/generator/IncludeCollector.h"
#include "decode/parser/Component.h"

#include <unordered_set>
#include <string>
#include <cassert>

namespace decode {

StatusEncoderGen::StatusEncoderGen(const Rc<TypeReprGen>& reprGen, SrcBuilder* output)
    : _typeReprGen(reprGen)
    , _output(output)
    , _inlineSer(reprGen, output)
{
}

StatusEncoderGen::~StatusEncoderGen()
{
}

void StatusEncoderGen::generateHeader(const std::vector<ComponentAndMsg>& messages)
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
        appendStatusMessageGenFuncDecl(msg.component.get(), n);
        _output->append(";\n");
        n++;
    }

    _output->appendEol();
    _output->endCppGuard();
    _output->appendEol();
    _output->endIncludeGuard();
}

void StatusEncoderGen::generateSource(const std::vector<ComponentAndMsg>& messages)
{
    IncludeCollector coll;
    std::unordered_set<std::string> includes;
    includes.emplace("core/Writer");
    includes.emplace("core/Error");
    for (const ComponentAndMsg& msg : messages) {
        coll.collect(msg.msg.get(), &includes);
        //TODO: remove duplicates
        _output->append("#include \"photon/");
        _output->append(msg.component->moduleName());
        _output->append('/');
        _output->appendWithFirstUpper(msg.component->moduleName());
        _output->append(".Component.h\"\n");
    }
    for (const std::string& inc : includes) {
        _output->appendLocalIncludePath(inc);
    }
    _output->appendEol();

    std::size_t n = 0;
    for (const ComponentAndMsg& msg : messages) {
        appendStatusMessageGenFuncDecl(msg.component.get(), n);
        _output->append("\n{\n");

        for (const Rc<StatusRegexp>& part : msg.msg->parts()) {
            appendInlineSerializer(msg.component.get(), part.get());
        }
        _output->append("    return PhotonError_Ok;\n}\n\n");
        n++;
    }
}

void StatusEncoderGen::appendInlineSerializer(const Component* comp, const StatusRegexp* part)
{
    if (part->accessors().empty()) {
        return;
    }
    assert(part->accessors()[0]->accessorKind() == AccessorKind::Field);
    AccessorKind lastKind = AccessorKind::Field;

    InlineSerContext ctx;
    StringBuilder currentField = "_";
    currentField.append(comp->moduleName());
    const Type* lastType;
    for (const Rc<Accessor>& acc : part->accessors()) {
        switch (acc->accessorKind()) {
        case AccessorKind::Field: {
            auto facc = static_cast<const FieldAccessor*>(acc.get());
            currentField.append('.');
            currentField.append(facc->field()->name());
            lastType = facc->field()->type();
            break;
        }
        case AccessorKind::Subscript: {
            auto sacc = static_cast<const SubscriptAccessor*>(acc.get());
            const Type* type = sacc->type().get();
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
            if (sacc->subscript().isFirst()) {
                const Range& range = sacc->subscript().unwrapFirst();
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
                uintmax_t i = sacc->subscript().unwrapSecond();
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
