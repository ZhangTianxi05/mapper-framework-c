#include "log/log.h"

int main() {
    log_init();

    // 默认级别是 INFO，DEBUG 不会输出
    log_debug("This is a debug log (should NOT appear)");
    log_info("This is an info log");
    log_warn("This is a warning log");
    log_error("This is an error log");

    // 设置日志级别为 DEBUG，所有日志都会输出
    log_set_level(LOG_LEVEL_DEBUG);
    log_debug("This is a debug log (should appear now)");
    log_info("Another info log");
    log_warn("Another warning log");
    log_error("Another error log");

    // 设置日志级别为 ERROR，只输出 error 和 fatal
    log_set_level(LOG_LEVEL_ERROR);
    log_info("This info log should NOT appear");
    log_error("This error log should appear");

    // 测试 fatal（会退出程序，放最后）
    // log_fatal("This is a fatal log (program will exit)");

    log_flush();
    return 0;
}