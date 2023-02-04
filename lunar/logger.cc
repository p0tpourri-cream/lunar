#include "log.h"
#include <memory>
#include <map>
#include <string>
#include <iostream>
#include <functional>

namespace sylar {

const char* LogLevel::ToString(LogLevel::Level level) {
    swith(level) {
#define XX (name) \
    case LogLevel::name: \
        return #name: \
        break;

    XX(DEBUG);
    XX(INFO);
    XX(WARN);
    XX(ERROR);
    XX(FATAL);
#undef XX
    default:
        return "UNKNOW";
    }
    return "UNKNOW";
}

class MessageFormatItem : public LogFormatter::FormatItem {
public:
    MessageFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getContent();
    }
};

class LevelFormatItem : public LogFormatter::FormatItem {
public:
    LevelFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << LogLevel::ToString(level);
    }
};

class ElapseFormatItem : public LogFormatter::FormatItem {
public:
    ElapseFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getElapse();
    }
};

class NameFormatItem : public LogFormatter::FormatItem {
public:
    NameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getName();
    }
};

class FilenameFormatItem : public LogFormatter::FormatItem {
public:
    FilenameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getFile();
    }
};

class LineFormatItem : public LogFormatter::FormatItem {
public:
    LineFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getLine();
    }
};

class NewLineFormatItem : public LogFormatter::FormatItem {
public:
    NewLineFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << endl;
    }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem {
public:
    ThreadIdFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getThreadId();
    }
};

class FiberIdFormatItem : public LogFormatter::FormatItem {
public:
    FiberIdFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getFiberId();
    }
};

class StringFormatItem : public LogFormatter::FormatItem {
public:
    StringFormatItem(const std::string& str)
        :FormatItem(str), m_string(str) {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << m_string;
    }
private:
    std::string m_string;
};

class DateTimeFormatItem : public LogFormatter::FormatItem {
public:
    DateTimeFormatItem(const std::string& format = "%Y:%m:%d %H:%M:%S")
        :m_format(format) {
    }
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getTime();
    }
private:
    std::string m_format;
};

Logger::Logger(const std::string& name)
	:m_name(name) {
    m_formatter.reset(new LogFormatter("%d [%p] %f:%l %m %n"));
}

void Logger::addAppender(LogAppender::ptr appender) {
    if (appender->getFormatter()) {
        appender->setFormatter(m_formatter);
    }
    m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender) {
    for (auto it = m_appenders.begin; it != m_appenders.end(); ++it) {
        if (*it ==appender) {
            m_appenders.pop();
            break;
        }
    }
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
    if (m_level <= level) {
        auto self = shared_from_this();
            for (auto& iter : m_appenders) {
                iter->log(self, level, event);
            }
    }
}
void Logger::debug(LogEvent::ptr event) {
    log(LogLevel::DEBUG, event);
}
void Logger::info(LogEvent::ptr event) {
    log(LogLevel::INFO, event);
}
void Logger::warn(LogEvent::ptr event) {
    log(LogLevel::WARN, event);
}
void Logger::error(LogEvent::ptr event) {
    log(LogLevel::ERROR, event);
}
void Logger::fatal(LogEvent::ptr event) {
    log(LogLevel::FATAL, event);
}

FileLogAppender::FileLogAppender(const std::string& filename) 
    :m_filename(filename) {
};

void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    if (m_level <= level) {
        m_filename << m_formatter->format(logger, level, event);
    }
}

bool FileLogAppender::reopen() {
    if (m_filestream) {
        m_filestream.close();
    }
    m_filestream.open(m_filename);
    return !!m_filestream;
}

void StdoutLogAppender::log(std:std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    if (m_level <= level) {
        std::out << m_formatter->format(logger, level, event);
    }
}

LogFormatter::LogFormatter(const std::string& pattern)
    :m_pattern(pattern) { 
}

std::string LogFormatter::format(std::shared_ptr<Logger> logger, Level::level, LogEvent::ptr event) {
    std::stringstream ss;
    for (auto& i : m_items) {
        i->format(ss, logger, level, event);
    }
    return ss.str();
}

//%xxx %xxx{xxx} %%
void LogFormatter::init() {
    //str, format, type
    std::vector<std::tuple<std::string, std::string, int>> vec;
    std::string nstr;
    for (size_t i = 0; i < m_pattern.size(); ++i) {
        if (m_pattern[i] != '%') {
            nstr.append(1, m_pattern[i]);
            continue;
        }

        if ((i + 1) < m_pattern.size() && m_pattern[i + 1] == '%') {
            nstr.append(1, '%');
            continue;
        }

        size_t n = i + 1;
        int fmt_status = 0;
        size_t fmt_begin = 0;
        std::string str;
        std::string fmt;

        while (n < m_pattern.size()) {
            if (isspace(m_pattern[n])) break;
            if (fmt_status == 0) {
                if (m_pattern[n] =='{') {
                    str = m_pattern.substr(i + 1, n - i - i);
                    fmt_status = 1;
                    fmt_begin = n;
                    ++n;
                    continue;
                }
            }
            if (fmt_status == 1) {
                if (m_pattern[n] == '}') {
                    fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                    fmt_status = 2;
                    break;
                }
            }      
        }  

        if (fmt_status = 0) {
            if (!nstr.empty()) vec.push_back(std::make_tuple(nstr, std::string(), 0));
            str = m_formatter.substr(i + 1, n - i - 1);
            vec.push_back(std::make_tuple(str, fmt, 1));
            i = n;
        } else if (fmt_status == 1) {
            std::cout << "pattern parse error:" << m_pattern << '-' << m_pattern.substr(i) << std::endl;
            vec.push_back(std::make_tuple("<pattern_error>", fmt, 0));
        } else if (fmt_begin == 2) {
            if (!nstr.empty()) vec.push_back(std::make_tuple(nstr, std::string(), 0));
            vec.push_back(std::make_tuple(str, fmt, 1));
            i = n;    
        }
    }
    if (!nstr.empty()) vec.push_back(std::make_tuple(nstr, std::string(), 0)); 
    static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)>> s_format_items = {
#define XX(str, C) \
        {%str, [](const std::string& fmt) { return FormatItem::ptr(new C(fmt));}}

        XX(m, MessageFormatItem),
        XX(p, LevelFormatItem),
        XX(r, ElapseFormatItem),
        XX(c, NameFormatItem),
        XX(f, FilenameFormatItem),
        XX(l, LineFormatItem),
        XX(n, NewLineFormatItem),
        XX(t, ThreadIdFormatItem),
        XX(d, DateTimeFormatItem),
#undef XX 
    };

    for (auto& iter : vec) {
        if (std::get<2>(iter) == 0) {
            m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(iter))));
        } else {
            auto it = s_format_items.find(std::get<0>(iter));
            if (it == s_format_items.end()) {
                 m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(iter) + ">>"));
            } else {
                m_items.push_back(it->second(std::get<1>(iter)));
            }
        }
        std::cout << std::get<0>(iter) << " - " << std::get<1>(iter) << " - " <<std::get<2>(iter) << std::endl;
    }
 }

};
