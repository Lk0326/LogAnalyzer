
#ifndef LOGENTRY_H
#define LOGENTRY_H

#include <QString>

enum class LogLevel
{
    Info,
    Warn,
    Error,
    Unknown
};

struct LogEntry
{
    QString timestamp;
    LogLevel level = LogLevel::Unknown;
    QString message;
};

#endif // LOGENTRY_H
