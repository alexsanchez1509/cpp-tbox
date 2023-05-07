#ifndef TBOX_MAIN_LOG_H_20220414
#define TBOX_MAIN_LOG_H_20220414

#include <tbox/base/json_fwd.h>

#include <tbox/log/stdout_channel.h>
#include <tbox/log/syslog_channel.h>
#include <tbox/log/file_async_channel.h>

#include <tbox/terminal/terminal_nodes.h>

#include "context.h"

namespace tbox {
namespace main {

class Log {
  public:
    Log();

  public:
    void fillDefaultConfig(Json &cfg) const;
    bool initialize(const char *proc_name, Context &ctx, const Json &cfg);
    void cleanup();

  protected:
    void initChannel(const Json &js, log::Channel &ch);
    void buildTerminalNodes(terminal::TerminalNodes &term);
    void buildTerminalNodesForChannel(terminal::TerminalNodes &term,
            log::Channel &log_ch, terminal::NodeToken dir_node);

  private:
    log::StdoutChannel stdout_;
    log::SyslogChannel syslog_;
    log::FileAsyncChannel filelog_;
};

}
}

#endif //TBOX_MAIN_LOG_H_20220414
