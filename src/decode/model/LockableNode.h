#pragma once

#include "decode/Config.h"
#include "decode/model/Node.h"
#include "decode/core/Lock.h"

namespace decode {

class LockableNode : public Node {
public:
    LockableNode(bmcl::OptionPtr<Node> parent)
        : Node(parent)
    {
    }

    void lock() const
    {
        _lock.lock();
    }

    void unlock() const
    {
        _lock.unlock();
    }

private:
    mutable Lock _lock;
};

}
