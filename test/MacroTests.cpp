#define BOOST_TEST_MODULE cpp_resolve test
#include <boost/test/unit_test.hpp>
#include "util/CppFile.h"

BOOST_AUTO_TEST_CASE( simplest_directive_test )
{
    CppFile file;
	CppFile::StringStore definitions
	{ "LOG4CXX_ABI_VERSION=16"
	};
    BOOST_REQUIRE(file.LoadFile("log4cxx/aprinitializer.cpp", definitions));
    BOOST_CHECK_EQUAL(file.GetUpdateCount(), 6);

    std::stringstream ss;
    file.Store(ss);
    CppFile processedFile;
    processedFile.Load(ss, definitions);
    BOOST_CHECK(processedFile.IsValid());
    BOOST_CHECK_EQUAL(processedFile.GetUpdateCount(), 0);
}

BOOST_AUTO_TEST_CASE( ifelse_directive_test )
{
    CppFile file;
	CppFile::StringStore definitions
	{ "LOG4CXX_ABI_VERSION=16"
	};
    BOOST_REQUIRE(file.LoadFile("log4cxx/hierarchy.h", definitions));
    BOOST_CHECK_EQUAL(file.GetUpdateCount(), 3);

    std::stringstream ss;
    file.Store(ss);
    CppFile processedFile;
    processedFile.Load(ss, definitions);
    BOOST_CHECK(processedFile.IsValid());
    BOOST_CHECK_EQUAL(processedFile.GetUpdateCount(), 0);
}
