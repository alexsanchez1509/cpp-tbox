#include "if_else_action.h"

#include <tbox/base/assert.h>
#include <tbox/base/defines.h>
#include <tbox/base/json.hpp>

namespace tbox {
namespace flow {

using namespace std::placeholders;

IfElseAction::IfElseAction(event::Loop &loop, Action *cond_action,
                           Action *if_action, Action *else_action) :
  Action(loop),
  cond_action_(cond_action),
  if_action_(if_action),
  else_action_(else_action)
{
  TBOX_ASSERT(cond_action != nullptr);

  cond_action_->setFinishCallback(std::bind(&IfElseAction::onCondActionFinished, this, _1));
  if (if_action_ != nullptr)
    if_action_->setFinishCallback(std::bind(&IfElseAction::finish, this, _1));
  if (else_action_ != nullptr)
    else_action_->setFinishCallback(std::bind(&IfElseAction::finish, this, _1));
}

IfElseAction::~IfElseAction() {
  CHECK_DELETE_RESET_OBJ(cond_action_);
  CHECK_DELETE_RESET_OBJ(if_action_);
  CHECK_DELETE_RESET_OBJ(else_action_);
}

void IfElseAction::toJson(Json &js) const {
  Action::toJson(js);
  cond_action_->toJson(js["cond"]);
  if (if_action_ != nullptr)
    if_action_->toJson(js["if"]);
  if (else_action_ != nullptr)
    else_action_->toJson(js["else"]);

  js["is_cond_done"] = is_cond_done_;
  if (is_cond_done_)
    js["is_cond_succ"] = is_cond_succ_;
}

bool IfElseAction::onStart() {
  return cond_action_->start();
}

bool IfElseAction::onStop() {
  if (is_cond_done_) {
    if (is_cond_succ_)
      return if_action_->stop();
    else
      return else_action_->stop();
  } else {
    return cond_action_->stop();
  }
}

bool IfElseAction::onPause() {
  if (is_cond_done_) {
    if (is_cond_succ_)
      return if_action_->pause();
    else
      return else_action_->pause();
  } else {
    return cond_action_->pause();
  }
}

bool IfElseAction::onResume() {
  if (is_cond_done_) {
    if (is_cond_succ_)
      return if_action_->resume();
    else
      return else_action_->resume();
  } else {
    return cond_action_->resume();
  }
}

void IfElseAction::onReset() {
  cond_action_->reset();

  if (if_action_ != nullptr)
    if_action_->reset();

  if (else_action_ != nullptr)
    else_action_->reset();

  is_cond_done_ = false;
  is_cond_succ_ = false;
}

void IfElseAction::onCondActionFinished(bool is_succ) {
  is_cond_done_ = true;
  is_cond_succ_ = is_succ;

  if (is_succ) {
    if (if_action_ != nullptr) {
      if_action_->start();
      return;
    }
  } else {
    if (else_action_ != nullptr) {
      else_action_->start();
      return;
    }
  }

  finish(true);
}

}
}
