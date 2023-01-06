#include "function_action.h"
#include <tbox/base/assert.h>
#include <tbox/event/loop.h>

namespace tbox {
namespace flow {

FunctionAction::FunctionAction(event::Loop &loop, const Func &func) :
  Action(loop), func_(func)
{
  TBOX_ASSERT(func != nullptr);
}

bool FunctionAction::onStart() {
  loop_.run([this] { finish(func_()); });
  return true;
}

}
}
