#ifndef UTIL_OPTIONAL_HDR_
#define UTIL_OPTIONAL_HDR_

#ifdef __has_include                           // Check if __has_include is present
#  if __has_include(<boost/optional.hpp>)    // Try with an external library
#    include <boost/optional.hpp>
namespace Util { template< class T > using Optional = boost::optional<T>; }
#define UTIL_HAS_STD_OPTIONAL 1
#  elif __has_include(<optional>)                // Check for a standard version
#    include <optional>
#    if defined(__cpp_lib_optional)            // C++ >= 17
namespace Util { template< class T > using Optional = std::optional<T>; }
#define UTIL_HAS_STD_OPTIONAL 1
#    endif
#  elif __has_include(<experimental/optional>) // Check for an experimental version
#    include <experimental/optional>
namespace Util { template< class T > using Optional = std::experimental::optional<T>; }
#define UTIL_HAS_STD_OPTIONAL 1
#  else                                        // Not found at all
#define UTIL_HAS_STD_OPTIONAL 0
#  endif
#endif

#if !UTIL_HAS_STD_OPTIONAL // Implement a minimal Optional?
namespace Util
{
	template< class T >
class Optional : private std::pair<bool, T>
{
	using BaseType = std::pair<bool, T>;
public:
	Optional() : BaseType(false, T()) {}
	Optional(const T& value) : BaseType(true, value) {}
	Optional& operator=(const T&& value)
	{
		this->first = true;
		this->second = std::move(value);
		return *this;
	}
	bool has_value() const { return this->first; }
	const T& value() const { return this->second; }
};
} // namespace Util
#endif

#endif // UTIL_OPTIONAL_HDR_
