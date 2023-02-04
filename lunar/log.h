#ifndef __SYLAR_LOG_H__
#define __SYLAR_LOG_H__

#include <stdint.h>
#include <string>
#include <memory>
#include <list>
#include <sstream>
#include <fstream>
#include <vector>

namespace sylar {

//日志事件
class LogEvent {
public:
	typedef std::shared_ptr<LogEvent> ptr;
	LogEvent();

    const char* getFile() const { return m_file;}
    int32_t getLine() const { return m_line;}
    uint32_t getElapse() const { return m_elapse;}
    uint32_t getThreadId() const { return m_threadId;}
    uint32_t getFiberId() const { return m_fiberId;}
    uint64_t getTime() const { return m_time;}
    const std::string& getContent() const { return m_content;}
private:
	const char* m_file = nullptr;	//文件名
	int32_t m_line = 0;				//行号
	uint32_t m_elapse = 0;			//程序自启动开始到目前的毫秒数
	uint32_t m_threadId = 0;		//线程id
	uint32_t m_fiberId = 0;			//协程id
	uint64_t m_time = 0;			//时间戳
	std::string m_content;
};

//日志等级
class LogLevel {
public:
	typedef std::shared_ptr<LogLevel> ptr;
	enum Level{
        UNKNOW = 0,
		DEBUG = 1,
		INFO = 2,
		WARN = 3,
		ERROR = 4,
		FATAL = 5
	};

    static const char* ToString(LogEvent::Level level);
};

//日志格式器
class LogFormatter {
public:
	typedef std::shared_ptr<LogFormatter> ptr;
    LogFormatter(const std::string& pattern);

    // %t   %thread_id  %m%n
	std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
public:
    class FormatItem {
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        FormatItem(const std::string& fmt) {};
        virtual ~FormatItem() {};
        virtual void format(std::ostream& os,std::shared_ptr<Logger> logger,  LogLevel::Level level, LogEvent::ptr event) = 0;
    };

    void init();
private:
    std::string m_pattern;
    std::vector<FormatItem::ptr> m_item;
};

//日志输出地
class LogAppender {
public：
	typedef std::shared_ptr<LogAppender> ptr;
	virtual ~LogAppender() {};

	virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;

    void setFormatter(LogFormatter::ptr val) { m_formatter = val;}
    LogFormatter::ptr getFormatter() const { return m_formatter;}
protected:
	LogLevel::Level m_level;
    LogFormatter::ptr m_formatter;
};

//日志器
class Logger : public std::enable_shared_from_this<Logger>{
public:
	typedef std::shared_ptr<Logger> ptr;

	Logger(const std::string name = "root");
	void log(LogLevel::Level level, LogEvent::ptr event);

	void debug(LogEvent::ptr event);
	void info(LogEvent::ptr event);
	void warn(LogEvent::ptr event);
	void error(LogEvent::ptr event);
	void fatal(LogEvent::ptr event);

    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr appender);
    LogLevel::Level getLevel() const { return m_level;}
    void setLevel(LogLevel::Level val) { m_level = val;}

    const std::string& getName() const {return m_name;}
private:
	std::string m_name;
	LogLevel::Level m_level;
	std::list<LogAppender::ptr> m_appenders;
    LogFormatter::ptr m_formatter;
};

//输出到控制台的Appender
class StdoutLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    virtual void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
};

//输出到文件的Appender
class FileLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<FileLogAppender> ptr;
    FileLogAppender(const std::string &filename);
    virtual void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;

    bool reopen(); 
private:
    std::string m_filename;
    std::ofstream m_filestream;
}

#endif
