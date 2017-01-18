#pragma once

#include "decode/Config.h"
#include "decode/parser/Type.h"
#include "decode/parser/Component.h"

namespace decode {

template <typename B>
class FuncPrototypeGen {
public:
    B& base();
    void appendDeserializerFuncDecl(const Type* type);
    void appendSerializerFuncDecl(const Type* type);
    void appendStatusMessageGenFuncDecl(const Component* comp, std::uintmax_t msgNum);
    void appendStatusMessageGenFuncName(const Component* comp, std::uintmax_t msgNum);
};

template <typename B>
inline B& FuncPrototypeGen<B>::base()
{
    return *static_cast<B*>(this);
}

template <typename B>
void FuncPrototypeGen<B>::appendSerializerFuncDecl(const Type* type)
{
    base().output().append("PhotonError ");
    base().genTypeRepr(type);
    base().output().append("_Serialize(const ");
    base().genTypeRepr(type);
    if (type->typeKind() != TypeKind::Enum) {
        base().output().append('*');
    }
    base().output().append(" self, PhotonWriter* dest)");
}

template <typename B>
void FuncPrototypeGen<B>::appendDeserializerFuncDecl(const Type* type)
{
    base().output().append("PhotonError ");
    base().genTypeRepr(type);
    base().output().append("_Deserialize(");
    base().genTypeRepr(type);
    base().output().append("* self, PhotonReader* src)");
}

template <typename B>
void FuncPrototypeGen<B>::appendStatusMessageGenFuncName(const Component* comp, std::uintmax_t msgNum)
{
    base().output().append("_Photon");
    base().output().appendWithFirstUpper(comp->moduleName());
    base().output().append("_GenerateMsg");
    base().output().appendNumericValue(msgNum);
}

template <typename B>
void FuncPrototypeGen<B>::appendStatusMessageGenFuncDecl(const Component* comp, std::uintmax_t msgNum)
{
    base().output().append("PhotonError ");
    appendStatusMessageGenFuncName(comp, msgNum);
    base().output().append("(PhotonWriter* dest)");
}
}
