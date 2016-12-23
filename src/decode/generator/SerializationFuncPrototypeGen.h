#pragma once

#include "decode/Config.h"
#include "decode/parser/Type.h"

namespace decode {

template <typename B>
class SerializationFuncPrototypeGen {
public:
    B& base();
    void appendDeserializerFuncDecl(const Type* type);
    void appendSerializerFuncDecl(const Type* type);
};

template <typename B>
inline B& SerializationFuncPrototypeGen<B>::base()
{
    return *static_cast<B*>(this);
}

template <typename B>
void SerializationFuncPrototypeGen<B>::appendSerializerFuncDecl(const Type* type)
{
    base().output().append("PhotonError ");
    base().genTypeRepr(type);
    base().output().append("_Serialize(const ");
    base().genTypeRepr(type);
    base().output().append("* self, PhotonWriter* dest)");
}

template <typename B>
void SerializationFuncPrototypeGen<B>::appendDeserializerFuncDecl(const Type* type)
{
    base().output().append("PhotonError ");
    base().genTypeRepr(type);
    base().output().append("_Deserialize(");
    base().genTypeRepr(type);
    base().output().append("* self, PhotonReader* src)");
}
}
