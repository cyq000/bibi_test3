#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include <mutex>

class Logger {
public:
    enum Level {
        LEVEL_INFO,
        LEVEL_WARNING,
        LEVEL_ERROR,
        LEVEL_DEBUG
    };

    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    // private members first so member functions can use them
private:
    Logger() {}
    ~Logger() {
        if (ofs_.is_open()) ofs_.close();
    }

    std::ofstream ofs_;
    std::mutex mutex_;

private:
    std::string timestamp() {
        std::time_t now = std::time(nullptr);
        char buf[64];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        return buf;
    }

    std::string levelToString(Level level) {
        switch (level) {
            case LEVEL_INFO: return "[INFO]";
            case LEVEL_WARNING: return "[WARNING]";
            case LEVEL_ERROR: return "[ERROR]";
            case LEVEL_DEBUG: return "[DEBUG]";
        }
        return "[UNKNOWN]";
    }

public:
    void setLogFile(const std::string& filename) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (ofs_.is_open()) {
            ofs_.close();
        }
        ofs_.open(filename, std::ios::app);
    }

    void log(Level level, const std::string& msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::ostringstream oss;
        oss << "[" << timestamp() << "] "
            << levelToString(level) << " "
            << msg << "\n";

        // output to console
        std::cout << oss.str();

        // output to file
        if (ofs_.is_open()) {
            ofs_ << oss.str();
            ofs_.flush();
        }
    }
};

/* 宏定义简化调用 */
#define LOG_INFO(msg) Logger::instance().log(Logger::LEVEL_INFO, msg)
#define LOG_WARN(msg) Logger::instance().log(Logger::LEVEL_WARNING, msg)
#define LOG_ERROR(msg) Logger::instance().log(Logger::LEVEL_ERROR, msg)
#define LOG_DEBUG(msg) Logger::instance().log(Logger::LEVEL_DEBUG, msg)

#endif // LOGGER_H
