#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/ast/Type.h"

namespace decode {

#define CREATE_BUILTIN(name, enum) \
        _##name##Type = new BuiltinType(BuiltinTypeKind:: enum);

class AllBuiltinTypes : public RefCountable {
public:
    AllBuiltinTypes()
    {
        CREATE_BUILTIN(usize, USize);
        CREATE_BUILTIN(isize, ISize);
        CREATE_BUILTIN(varint, Varint);
        CREATE_BUILTIN(varuint, Varuint);
        CREATE_BUILTIN(u8, U8);
        CREATE_BUILTIN(u16, U16);
        CREATE_BUILTIN(u32, U32);
        CREATE_BUILTIN(u64, U64);
        CREATE_BUILTIN(i8, I8);
        CREATE_BUILTIN(i16, I16);
        CREATE_BUILTIN(i32, I32);
        CREATE_BUILTIN(i64, I64);
        CREATE_BUILTIN(f32, F32);
        CREATE_BUILTIN(f64, F64);
        CREATE_BUILTIN(bool, Bool);
        CREATE_BUILTIN(void, Void);
        CREATE_BUILTIN(char, Char);
    }

    ~AllBuiltinTypes()
    {
    }

    const BuiltinType* usizeType() const
    {
        return _usizeType.get();
    }

    const BuiltinType* isizeType() const
    {
        return _isizeType.get();
    }

    const BuiltinType* varintType() const
    {
        return _varintType.get();
    }

    const BuiltinType* varuintType() const
    {
        return _varuintType.get();
    }

    const BuiltinType* u8Type() const
    {
        return _u8Type.get();
    }

    const BuiltinType* u16Type() const
    {
        return _u16Type.get();
    }

    const BuiltinType* u32Type() const
    {
        return _u32Type.get();
    }

    const BuiltinType* u64Type() const
    {
        return _u64Type.get();
    }

    const BuiltinType* i8Type() const
    {
        return _i8Type.get();
    }

    const BuiltinType* i16Type() const
    {
        return _i16Type.get();
    }

    const BuiltinType* i32Type() const
    {
        return _i32Type.get();
    }

    const BuiltinType* i64Type() const
    {
        return _i64Type.get();
    }

    const BuiltinType* f32Type() const
    {
        return _f32Type.get();
    }

    const BuiltinType* f64Type() const
    {
        return _f64Type.get();
    }

    const BuiltinType* boolType() const
    {
        return _boolType.get();
    }

    const BuiltinType* voidType() const
    {
        return _voidType.get();
    }

    const BuiltinType* charType() const
    {
        return _charType.get();
    }

    BuiltinType* usizeType()
    {
        return _usizeType.get();
    }

    BuiltinType* isizeType()
    {
        return _isizeType.get();
    }

    BuiltinType* varintType()
    {
        return _varintType.get();
    }

    BuiltinType* varuintType()
    {
        return _varuintType.get();
    }

    BuiltinType* u8Type()
    {
        return _u8Type.get();
    }

    BuiltinType* u16Type()
    {
        return _u16Type.get();
    }

    BuiltinType* u32Type()
    {
        return _u32Type.get();
    }

    BuiltinType* u64Type()
    {
        return _u64Type.get();
    }

    BuiltinType* i8Type()
    {
        return _i8Type.get();
    }

    BuiltinType* i16Type()
    {
        return _i16Type.get();
    }

    BuiltinType* i32Type()
    {
        return _i32Type.get();
    }

    BuiltinType* i64Type()
    {
        return _i64Type.get();
    }

    BuiltinType* f32Type()
    {
        return _f32Type.get();
    }

    BuiltinType* f64Type()
    {
        return _f64Type.get();
    }

    BuiltinType* boolType()
    {
        return _boolType.get();
    }

    BuiltinType* voidType()
    {
        return _voidType.get();
    }

    BuiltinType* charType()
    {
        return _charType.get();
    }

private:
    Rc<BuiltinType> _usizeType;
    Rc<BuiltinType> _isizeType;
    Rc<BuiltinType> _varuintType;
    Rc<BuiltinType> _varintType;
    Rc<BuiltinType> _u8Type;
    Rc<BuiltinType> _i8Type;
    Rc<BuiltinType> _u16Type;
    Rc<BuiltinType> _i16Type;
    Rc<BuiltinType> _u32Type;
    Rc<BuiltinType> _i32Type;
    Rc<BuiltinType> _u64Type;
    Rc<BuiltinType> _i64Type;
    Rc<BuiltinType> _f32Type;
    Rc<BuiltinType> _f64Type;
    Rc<BuiltinType> _boolType;
    Rc<BuiltinType> _voidType;
    Rc<BuiltinType> _charType;
};
}
