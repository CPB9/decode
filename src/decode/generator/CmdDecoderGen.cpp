#include "decode/generator/CmdDecoderGen.h"

namespace decode {

CmdDecoderGen::CmdDecoderGen(const Rc<TypeReprGen>& reprGen, SrcBuilder* output)
    : _typeReprGen(reprGen)
    , _output(output)
    , _inlineSer(reprGen, _output)
    , _inlineDeser(reprGen, _output)
    , _paramName("_p0")
{
}

CmdDecoderGen::~CmdDecoderGen()
{
}

void CmdDecoderGen::generateHeader(const std::map<std::size_t, Rc<Component>>& comps)
{
    _output->startIncludeGuard("PRIVATE", "CMD_DECODER");
    _output->appendEol();

    _output->appendLocalIncludePath("core/Error");
    _output->appendLocalIncludePath("core/Reader");
    _output->appendLocalIncludePath("core/Writer");
    _output->appendEol();

    _output->startCppGuard();

//     for (auto it : comps) {
//         if (it.second->commands().isNone()) {
//             continue;
//         }
//         std::size_t cmdNum = 0;
//         for (const Rc<FunctionType>& jt : it.second->commands().unwrap()->functions()) {
//             (void)jt;
//             appendFunctionPrototype(it.first, cmdNum);
//             _output->append(";\n");
//             cmdNum++;
//         }
//     }
//     _output->appendEol();

    appendMainFunctionPrototype();
    _output->append(";\n");

    _output->appendEol();
    _output->endCppGuard();
    _output->appendEol();
    _output->endIncludeGuard();
}

void CmdDecoderGen::appendFunctionPrototype(unsigned componenNum, unsigned cmdNum)
{
    _output->append("PhotonError ");
    appendFunctionName(componenNum, cmdNum);
    _output->append("(PhotonReader* src, PhotonWriter* dest)");
}

void CmdDecoderGen::appendFunctionName(unsigned componenNum, unsigned cmdNum)
{
    _output->append("decodeCmd");
    _output->appendNumericValue(componenNum);
    _output->append('_');
    _output->appendNumericValue(cmdNum);
}

void CmdDecoderGen::generateSource(const std::map<std::size_t, Rc<Component>>& comps)
{
    _output->appendLocalIncludePath("CmdDecoder.Private");
    _output->appendEol();

    for (auto it : comps) {
        _output->append("#include \"photon/");
        _output->append(it.second->moduleName());
        _output->append("/");
        _output->appendWithFirstUpper(it.second->moduleName());
        _output->appendWithFirstUpper(".Component.h\"\n");
    }
    _output->appendEol();

    for (auto it : comps) {
        if (it.second->commands().isNone()) {
            continue;
        }
        std::size_t cmdNum = 0;
        for (const Rc<Function>& jt : it.second->commands().unwrap()->functions()) {
            generateFunc(it.second, jt, it.first, cmdNum);
            cmdNum++;
            _output->appendEol();
            _output->appendEol();
        }
    }

    generateMainFunc(comps);
}

void CmdDecoderGen::appendMainFunctionPrototype()
{
    _output->append("PhotonError Photon_ExecCmd(uint8_t compNum, uint8_t cmdNum, PhotonReader* src, PhotonWriter* dest)");
}

void CmdDecoderGen::generateMainFunc(const std::map<std::size_t, Rc<Component>>& comps)
{
    appendMainFunctionPrototype();
    _output->append("\n{\n");
    _output->append("    switch (compNum) {\n");
    for (auto it : comps) {
        if (it.second->commands().isNone()) {
            continue;
        }

        _output->append("    case ");
        _output->appendNumericValue(it.first);
        _output->append(": {\n");

        _output->append("        switch (cmdNum) {\n");

        std::size_t cmdNum = 0;
        for (const Rc<Function>& func : it.second->commands().unwrap()->functions()) {
            (void)func;
            _output->append("        case ");
            _output->appendNumericValue(cmdNum);
            _output->append(":\n");
            _output->append("            return ");
            appendFunctionName(it.first, cmdNum);
            _output->append("(src, dest);\n");
            cmdNum++;
        }
        _output->append("        default:\n");
        _output->append("            return PhotonError_InvalidCmdId;\n");
        _output->append("        }\n    }\n");
    }
    _output->append("    }\n    return PhotonError_InvalidComponentId;\n}");
}

template <typename C>
void CmdDecoderGen::foreachParam(const Rc<Function>& func, C&& f)
{
    std::size_t fieldNum = 0;
    _paramName.resize(2);
    InlineSerContext ctx;
    for (const Rc<Field>& field : func->type()->arguments()) {
        _paramName.appendNumericValue(fieldNum);
        f(field, _paramName.view());
        _paramName.resize(2);
        fieldNum++;
    }
}

void CmdDecoderGen::writePointerOp(const Type* type)
{
    const Type* t = type; //HACK: fix Rc::operator->
    switch (type->typeKind()) {
    case TypeKind::Reference:
    case TypeKind::Array:
    case TypeKind::Function:
    case TypeKind::Enum:
    case TypeKind::Builtin:
        break;
    case TypeKind::Slice:
    case TypeKind::Struct:
    case TypeKind::Variant:
        _output->append("&");
        break;
    case TypeKind::Imported:
        writePointerOp(t->asImported()->link());
        break;
    case TypeKind::Alias:
        writePointerOp(t->asAlias()->alias());
        break;
    }
}

void CmdDecoderGen::generateFunc(const Rc<Component>& comp, const Rc<Function>& func, unsigned componenNum, unsigned cmdNum)
{
    const FunctionType* ftype = func->type();
    _output->append("static ");
    appendFunctionPrototype(componenNum, cmdNum);
    _output->append("\n{\n");

    foreachParam(func, [this](const Rc<Field>& field, bmcl::StringView name) {
        _output->append("    ");
        _typeReprGen->genTypeRepr(field->type(), name);
        _output->append(";\n");
    });

    if (ftype->returnValue().isSome()) {
        _output->append("    ");
        _typeReprGen->genTypeRepr(ftype->returnValue().unwrap().get(), "_rv");
        _output->append(";\n");
    }
    _output->appendEol();

    InlineSerContext ctx;
    foreachParam(func, [this, ctx](const Rc<Field>& field, bmcl::StringView name) {
        _inlineDeser.inspect(field->type(), ctx, name);
    });
    _output->appendEol();

    //TODO: gen command call
    _output->append("    PHOTON_TRY(Photon");
    _output->appendWithFirstUpper(comp->moduleName());
    _output->append("_");
    _output->appendWithFirstUpper(func->name());
    _output->append("(");
    foreachParam(func, [this](const Rc<Field>& field, bmcl::StringView name) {
        (void)field;
        writePointerOp(field->type());
        _output->append(name);
        _output->append(", ");
    });
    if (ftype->returnValue().isSome()) {
        writePointerOp(ftype->returnValue().unwrap().get());
        _output->append("_rv");
    } else if (!ftype->arguments().empty()) {
        _output->removeFromBack(2);
    }
    _output->append("));\n\n");

    if (ftype->returnValue().isSome()) {
        _inlineSer.inspect(ftype->returnValue().unwrap().get(), ctx, "_rv");
    }

    _output->append("\n    return PhotonError_Ok;\n}");
}

void CmdDecoderGen::genTypeRepr(const Type* type, bmcl::StringView fieldName)
{
    _typeReprGen->genTypeRepr(type, fieldName);
}

}
