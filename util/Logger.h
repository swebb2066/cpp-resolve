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

/// Streamable array elements
template <typename T, typename S, typename D = T>
class SeparatedArray
{
    const T *m_vec;
    size_t m_len;
    S m_separator;
    size_t m_perLine;
public:
    SeparatedArray(const T *vec, size_t len, S separ, size_t perLine = 10)
        : m_vec(vec)
        , m_len(len)
        , m_separator(separ)
        , m_perLine(perLine)
    {}
    void Write(std::ostream& os) const
    {
        if (0 < m_perLine && m_perLine <= m_len)
            os << std::endl;
        for (size_t i = 0; i < m_len; ++i)
        {
            if (0 < i)
            {
                if (0 < m_perLine && 0 == (i % m_perLine))
                    os << std::endl;
                else
                    os << m_separator;
            }
            os << (D)m_vec[i];
        }
    }
};

/// Put \c S separated type \c D elements of \c v onto \c os
    template <typename T, typename S, typename D>
    std::ostream&
operator<<(std::ostream& os, const SeparatedArray<T, S, D>& v)
{ v.Write(os); return os;  }

} // namespace Util
#endif // UTIL_LOGGER_H_
