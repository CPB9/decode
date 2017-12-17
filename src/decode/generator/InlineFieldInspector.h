#pragma once

#include "decode/Config.h"
#include "decode/ast/Type.h"
#include "decode/ast/Field.h"
#include "decode/generator/SrcBuilder.h"
#include "decode/generator/Utils.h"

namespace decode {

template <typename B>
class InlineFieldInspector {
public:
    InlineFieldInspector(SrcBuilder* dest)
        : _dest(dest)
    {
    }

    B& base()
    {
        return *static_cast<B*>(this);
    }

    template <bool isOnboard, bool isSerializer, typename F, typename I>
    void inspect(F&& fields, I* typeInspector)
    {
        InlineSerContext ctx;
        auto begin = fields.begin();
        auto it = begin;
        auto end = fields.end();

        while (it != end) {
            bmcl::Option<std::size_t> totalSize;
            while (it != end) {
                bmcl::Option<std::size_t> size = it->type()->fixedSize();
                if (size.isNone()) {
                    break;
                }
                totalSize.emplace(totalSize.unwrapOr(0) + size.unwrap());
                it++;
            }
            if (totalSize.isSome()) {
                typeInspector->template appendSizeCheck<isOnboard, isSerializer>(ctx, std::to_string(totalSize.unwrap()), _dest);
                for (auto jt = begin; jt < it; jt++) {
                    base().beginField(*jt);
                    typeInspector->template inspect<isOnboard, isSerializer>(jt->type(), ctx, base().currentFieldName(), false);
                    base().endField(*jt);
                }
                totalSize.clear();
            } else {
                base().beginField(*it);
                typeInspector->template inspect<isOnboard, isSerializer>(it->type(), ctx, base().currentFieldName());
                base().endField(*it);
                it++;
                begin = it;
            }
        }
    }

private:
    SrcBuilder* _dest;
};

class InlineStructInspector : public InlineFieldInspector<InlineStructInspector> {
public:
    InlineStructInspector(SrcBuilder* dest, bmcl::StringView name = bmcl::StringView::empty())
        : InlineFieldInspector<InlineStructInspector>(dest)
        , _argName(name.begin(), name.size())
        , _argSize(name.size())
    {
    }

    void beginField(const Field* field)
    {
        _argName.append(field->name().begin(), field->name().end());
    }

    void endField(const Field*)
    {
        _argName.resize(_argSize);
    }

    void setArgName(bmcl::StringView name)
    {
        _argName.clear();
        _argName.append(name);
        _argSize = name.size();
    }

    bmcl::StringView currentFieldName() const
    {
        return _argName.view();
    }

private:
    StringBuilder _argName;
    std::size_t _argSize;
};

class WrappingInlineStructInspector : public InlineFieldInspector<WrappingInlineStructInspector> {
public:
    WrappingInlineStructInspector(SrcBuilder* dest, bmcl::StringView name = bmcl::StringView::empty())
        : InlineFieldInspector<WrappingInlineStructInspector>(dest)
        , _argName(name.begin(), name.size())
        , _argSize(name.size())
    {
    }

    void beginField(const Field* field)
    {
        derefPassedVarNameIfRequired(field->type(), field->name(), &_argName);
    }

    void endField(const Field*)
    {
        _argName.resize(_argSize);
    }

    void setArgName(bmcl::StringView name)
    {
        _argName.clear();
        _argName.append(name);
        _argSize = name.size();
    }

    bmcl::StringView currentFieldName() const
    {
        return _argName.view();
    }

private:
    StringBuilder _argName;
    std::size_t _argSize;
};
}
