#ifndef TBOX_TERMINAL_FUNC_NODE_H_20220207
#define TBOX_TERMINAL_FUNC_NODE_H_20220207

#include "node.h"
#include <map>

namespace tbox {
namespace terminal {

class FuncNode : public Node {
  public:
    FuncNode(const Func &func, const std::string &help);

    virtual NodeType type() const override { return NodeType::kFunc; }
    void execute(const Session &s, const Args &a) const;

  private:
    Func func_;
};

}
}

#endif //TBOX_TERMINAL_FUNC_NODE_H_20220207
