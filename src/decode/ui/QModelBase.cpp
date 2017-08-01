#include "decode/ui/QModelBase.h"
#include "decode/model/Node.h"
#include "decode/model/NodeView.h"

namespace decode {

template class QModelBase<Node>;
template class QModelBase<NodeView>;
}
