#pragma once

#include "decode/Config.h"
#include "decode/model/Node.h"
#include "decode/model/NodeView.h"
#include "decode/model/NodeViewUpdate.h"

#include <bmcl/Logging.h>

#include <unordered_map>

namespace decode {

class NodeViewStore {
public:
    NodeViewStore(NodeView* view)
        : _root(view)
    {
        registerNodes(_root.get());
    }

    void setRoot(NodeView* view)
    {
        _map.clear();
        _root.reset(view);
        registerNodes(view);
    }

    bool apply(NodeViewUpdate* update)
    {
        auto it = _map.find(update->id());
        if (it == _map.end()) {
            BMCL_CRITICAL() << "invalid update";
            return false;
        }
        NodeView* dest = it->second.get();

        switch (update->kind()) {
            case NodeViewUpdateKind::None:
                break;
            case NodeViewUpdateKind::Value:
                dest->setValue(std::move(update->as<Value>()));
                break;
            case NodeViewUpdateKind::Extend: {
                NodeViewVec& vec = update->as<NodeViewVec>();
                for (std::size_t i = 0; i < vec.size(); i++) {
                    const Rc<NodeView>& view = vec[i];
                    view->_parent = dest;
                    view->_indexInParent = i;
                    registerNodes(view.get());
                }
                dest->_children.insert(dest->_children.end(), vec.begin(), vec.end());
                break;
            }
            case NodeViewUpdateKind::Shrink: {
                std::size_t newSize = update->as<std::size_t>();
                if (newSize > dest->size()) {
                    return false;
                }
                for (std::size_t i = newSize; i < dest->size(); i++) {
                    unregisterNodes(dest->_children[i].get());
                }
                dest->_children.resize(newSize);
                break;
            }
        }
        return true;
    }

private:
    void registerNodes(NodeView* view)
    {
        view->visitNode([this](NodeView* visited) {
            _map.emplace(visited->id(), visited);
        });
    }

    void unregisterNodes(NodeView* view)
    {
        view->visitNode([this](NodeView* visited) {
            _map.erase(visited->id());
        });
    }

    Rc<NodeView> _root;
    std::unordered_map<uintptr_t, Rc<NodeView>> _map;
};

}
