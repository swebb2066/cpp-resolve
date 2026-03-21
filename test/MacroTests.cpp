#define BOOST_TEST_MODULE cpp_resolve test
#include <boost/test/unit_test.hpp>
#include "util/CppFile.h"

BOOST_AUTO_TEST_CASE( aprinitializer_cpp_test )
{
    CppFile file;
	CppFile::StringStore definitions
	{ "LOG4CXX_ABI_VERSION=16"
	};
    BOOST_REQUIRE(file.LoadFile("log4cxx/aprinitializer.cpp", definitions));
    BOOST_CHECK_EQUAL(file.GetUpdateCount(), 4);
    BOOST_CHECK(file.StoreFile("log4cxx/aprinitializer_new.cpp"));
}
