#include <gtest/gtest.h>
#include <tbox/event/loop.h>
#include <tbox/base/scope_exit.hpp>
#include "action.h"

namespace tbox {
namespace flow {

TEST(Action, Timeout) {
  auto loop = event::Loop::New();
  SetScopeExitAction([loop] { delete loop; });

  class TestAction : public Action {
    public:
      using Action::Action;
      virtual std::string type() const override { return "TestAction"; }
  };

  TestAction action(*loop);
  action.setTimeout(std::chrono::milliseconds(50));

  bool is_callback = false;
  std::chrono::steady_clock::time_point ts_start;
  std::chrono::steady_clock::time_point ts_timeout;

  action.setFinishCallback(
    [&, loop] (bool is_succ) {
      EXPECT_FALSE(is_succ);
      is_callback = true;
      ts_timeout = std::chrono::steady_clock::now();
      loop->exitLoop();
    }
  );
  action.start();
  ts_start = std::chrono::steady_clock::now();

  loop->runLoop();
  EXPECT_TRUE(is_callback);

  auto cout_50 = std::chrono::duration_cast<std::chrono::milliseconds>(ts_timeout - ts_start).count();
  EXPECT_LE(cout_50, 51);
  EXPECT_GE(cout_50, 49);
}

}
}
