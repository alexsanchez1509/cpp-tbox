#ifndef TBOX_FLOW_FLOW_H_20221001
#define TBOX_FLOW_FLOW_H_20221001

#include <string>
#include <functional>
#include <chrono>

#include <tbox/base/defines.h>
#include <tbox/base/smart_ptr.h>
#include <tbox/base/json_fwd.h>
#include <tbox/event/forward.h>

namespace tbox {
namespace flow {

//! 动作类
class Action {
  public:
    explicit Action(event::Loop &loop, const std::string &type);
    virtual ~Action();

    NONCOPYABLE(Action);
    IMMOVABLE(Action);

    TBOX_SMART_PTR_DEFINITIONS(Action);

  public:
    //! 状态
    enum class State {
      kNone,      //!< 未初始化状态
      kInited,    //!< 已初始化状态
      kRunning,   //!< 正在运行状态
      kPause,     //!< 暂停状态
      kFinished,  //!< 结束状态，自主通过 finish() 结束
      kStoped     //!< 停止状态，外部通过 stop() 结束
    };

    //! 结果
    enum class Result {
      kUnsure,    //!< 未知
      kSuccess,   //!< 成功
      kFail,      //!< 失败
    };

    inline int id() const { return id_; }
    inline const std::string& type() const { return type_; }

    inline State state() const { return state_; }
    inline Result result() const { return result_; }

    /// 是否正在进行中
    /// 表示已启动，但还没有结束或终止的状态
    inline bool isUnderway() const {
        return state_ == State::kRunning || state_ == State::kPause;
    }

    inline void set_label(const std::string &label) { label_ = label; }
    inline const std::string& label() const { return label_; }

    //!< 设置结束回调
    using FinishCallback = std::function<void(bool is_succ)>;
    inline void setFinishCallback(const FinishCallback &cb) { finish_cb_ = cb; }

    void setTimeout(std::chrono::milliseconds ms);
    void resetTimeout();

    virtual void toJson(Json &js) const;

    bool init();
    bool start();   //!< 开始
    bool pause();   //!< 暂停
    bool resume();  //!< 恢复
    bool stop();    //!< 停止
    void reset();   //!< 重置，将所有的状态恢复到刚构建状态

  protected:
    bool finish(bool is_succ);

    virtual bool onInit() = 0;   //!< 检查参数是否完整
    virtual bool onStart() { return true; }   //! WARN: 启动失败是一种异常，要少用
    virtual bool onPause() { return true; }
    virtual bool onResume() { return true; }
    virtual bool onStop() { return true; }
    virtual void onReset() { }
    virtual void onFinished(bool is_succ) { }
    virtual void onTimeout() { finish(false); }

  protected:
    event::Loop &loop_;

  private:
    static int _id_alloc_counter_;

    int id_;
    std::string type_;
    std::string label_;
    FinishCallback finish_cb_;

    State state_ = State::kNone;      //!< 状态
    Result result_ = Result::kUnsure; //!< 运行结果

    event::TimerEvent *timer_ev_ = nullptr;
};

//! 枚举转字串
std::string ToString(Action::State state);
std::string ToString(Action::Result result);

}
}

#endif //TBOX_FLOW_FLOW_H_20221001
