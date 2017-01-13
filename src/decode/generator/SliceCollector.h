#pragma once

#include "decode/Config.h"
#include "decode/parser/AstVisitor.h"
#include "decode/parser/Component.h"
#include "decode/generator/SrcBuilder.h"
#include "decode/generator/TypeNameGen.h"

#include <string>
#include <unordered_map>

namespace decode {

class Type;
class TypeReprGen;

class SliceCollector : public AstVisitor<SliceCollector> {
public:
    void collectUniqueSlices(Type* type, std::unordered_map<std::string, Rc<SliceType>>* dest);
    void collectUniqueSlices(Component* type, std::unordered_map<std::string, Rc<SliceType>>* dest);

    bool visitSliceType(SliceType* slice);

private:

    SrcBuilder _sliceName;
    std::unordered_map<std::string, Rc<SliceType>>* _dest;
};

inline void SliceCollector::collectUniqueSlices(Component* comp, std::unordered_map<std::string, Rc<SliceType>>* dest)
{
    _dest = dest;
    traverseComponentParameters(comp);
}

inline void SliceCollector::collectUniqueSlices(Type* type, std::unordered_map<std::string, Rc<SliceType>>* dest)
{
    _dest = dest;
    traverseType(type);
}

inline bool SliceCollector::visitSliceType(SliceType* slice)
{
    _sliceName.clear();
    TypeNameGen gen(&_sliceName);
    gen.genTypeName(slice);
    _dest->emplace(_sliceName.result(), slice);
    return true;
}
}
