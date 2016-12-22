#include "decode/generator/TypeReprGenerator.h"
#include "decode/generator/SliceNameGenerator.h"

namespace decode {

TypeReprGenerator::TypeReprGenerator(SrcBuilder* dest)
    : NameVisitor<TypeReprGenerator>(dest)
{
}

TypeReprGenerator::~TypeReprGenerator()
{
}

static bmcl::StringView builtinToC(const BuiltinType* type)
{
    switch (type->builtinTypeKind()) {
    case BuiltinTypeKind::USize:
        return "size_t";
    case BuiltinTypeKind::ISize:
        return "ptrdiff_t";
    case BuiltinTypeKind::Varuint:
        return "uint64_t";
    case BuiltinTypeKind::Varint:
        return "int64_t";
    case BuiltinTypeKind::U8:
        return "uint8_t";
    case BuiltinTypeKind::I8:
        return "int8_t";
    case BuiltinTypeKind::U16:
        return "uint16_t";
    case BuiltinTypeKind::I16:
        return "int16_t";
    case BuiltinTypeKind::U32:
        return "uint32_t";
    case BuiltinTypeKind::I32:
        return "int32_t";
    case BuiltinTypeKind::U64:
        return "uint64_t";
    case BuiltinTypeKind::I64:
        return "int64_t";
    case BuiltinTypeKind::Bool:
        return "bool";
    case BuiltinTypeKind::Void:
        return "void";
    case BuiltinTypeKind::Unknown:
        //FIXME: report error
        assert(false);
        return nullptr;
    }
    return nullptr;
}

inline bool TypeReprGenerator::visitBuiltinType(const BuiltinType* type)
{
    typeName.append(builtinToC(type));
    return false;
}

bool TypeReprGenerator::visitArrayType(const ArrayType* type)
{
    arrayIndices.push_back('[');
    arrayIndices.append(std::to_string(type->elementCount()));
    arrayIndices.push_back(']');
    return true;
}

bool TypeReprGenerator::visitReferenceType(const ReferenceType* type)
{
    if (type->isMutable()) {
        pointers.push_front(false);
    } else {
        pointers.push_front(true);
    }
    return true;
}

bool TypeReprGenerator::visitSliceType(const SliceType* type)
{
    hasPrefix = false;
    typeName.setModName(type->moduleName());
    SliceNameGenerator sng(&typeName);
    sng.genSliceName(type);
    return false;
}

inline bool TypeReprGenerator::visitFunctionType(const FunctionType* type)
{
    genFnPointerTypeRepr(type);
    return false;
}

inline bool TypeReprGenerator::appendTypeName(const Type* type)
{
    hasPrefix = true;
    typeName.setModName(type->moduleName());
    typeName.append(type->name());
    return false;
}

void TypeReprGenerator::genTypeRepr(const Type* type, bmcl::StringView fieldName)
{
    this->fieldName = fieldName;
    hasPrefix = false;
    typeName.clear();
    pointers.clear();
    arrayIndices.clear();
    traverseType(type);

    auto writeTypeName = [&]() {
        if (hasPrefix) {
            _output->appendModPrefix();
        }
        _output->append(typeName.result());
    };

    if (pointers.size() == 1) {
        if (pointers[0] == true) {
            _output->append("const ");
        }
        writeTypeName();
        _output->append('*');
    } else {
        writeTypeName();

        for (bool isConst : pointers) {
            if (isConst) {
                _output->append(" *const");
            } else {
                _output->append(" *");
            }
        }
    }
    if (!fieldName.isEmpty()) {
        _output->appendSpace();
        _output->append(fieldName.begin(), fieldName.end());
    }
    if (!arrayIndices.empty()) {
        _output->append(arrayIndices);
    }
}

void TypeReprGenerator::genFnPointerTypeRepr(const FunctionType* type)
{
    std::vector<const FunctionType*> fnStack;
    const FunctionType* current = type;
    fnStack.push_back(current);
    while (true) {
        if (current->returnValue().isSome()) {
            if (current->returnValue().unwrap()->typeKind() == TypeKind::Function) {
                current = static_cast<const FunctionType*>(current->returnValue()->get());
                fnStack.push_back(current);
            } else {
                break;
            }
        } else {
            break;
        }
    }
    if (fnStack.back()->returnValue().isSome()) {
        genTypeRepr(fnStack.back()->returnValue().unwrap().get());
    } else {
        _output->append("void");
    }
    _output->append(" ");
    for (std::size_t i = 0; i < fnStack.size(); i++) {
        _output->append("(*");
    }
    _output->append(fieldName);

    _output->append(")(");

    auto appendParameters = [this](const FunctionType* t) {
        if (t->arguments().size() > 0) {
            for (auto jt = t->arguments().cbegin(); jt < (t->arguments().cend() - 1); jt++) {
                genTypeRepr((*jt)->type().get());
                _output->append(", ");
            }
            genTypeRepr(t->arguments().back()->type().get());
        }
    };

    for (auto it = fnStack.begin(); it < (fnStack.end() - 1); it++) {
        appendParameters(*it);
        _output->append("))(");
    }
    appendParameters(fnStack.back());
    _output->append(")");
}
}
