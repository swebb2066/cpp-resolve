#ifndef UTIL_LOGGER_H_
#define UTIL_LOGGER_H_
#include <log4cxx/Logger.h>

namespace Util
{

/// The logger pointer we use
using LoggerPtr = log4cxx::LoggerPtr;

/// Retrieve the \c name logger pointer.
/// Configure Log4cxx on the first call.
extern auto getLogger(const std::string& name = std::string()) -> LoggerPtr;

} // namespace Util
#endif // UTIL_LOGGER_H_
