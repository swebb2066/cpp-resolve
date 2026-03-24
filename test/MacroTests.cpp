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
    auto oldLineCount = file.GetLineCount();
    CppFile::CountType deletedLineCount;
    BOOST_CHECK_EQUAL(file.GetUpdateCount(&deletedLineCount), 6);
    BOOST_CHECK_EQUAL(deletedLineCount, 32);

    std::stringstream ss;
    file.Store(ss);
    CppFile processedFile;
    BOOST_REQUIRE(processedFile.Load(ss, definitions));
    BOOST_CHECK(processedFile.IsValid());
    BOOST_CHECK_EQUAL(processedFile.GetLineCount(), oldLineCount - deletedLineCount);
    BOOST_CHECK_EQUAL(processedFile.GetUpdateCount(), 0);
}

BOOST_AUTO_TEST_CASE( ifelse_directive_test )
{
    CppFile file;
	CppFile::StringStore definitions
	{ "LOG4CXX_ABI_VERSION=16"
	};
    BOOST_REQUIRE(file.LoadFile("log4cxx/hierarchy.h", definitions));
    auto oldLineCount = file.GetLineCount();
    CppFile::CountType deletedLineCount;
    BOOST_CHECK_EQUAL(file.GetUpdateCount(&deletedLineCount), 7);
    BOOST_CHECK_EQUAL(deletedLineCount, 25);

    std::stringstream ss;
    file.Store(ss);
    CppFile processedFile;
    BOOST_REQUIRE(processedFile.Load(ss, definitions));
    BOOST_CHECK_EQUAL(processedFile.GetLineCount(), oldLineCount - deletedLineCount);
    BOOST_CHECK(processedFile.IsValid());
    BOOST_CHECK_EQUAL(processedFile.GetUpdateCount(), 0);
}

BOOST_AUTO_TEST_CASE( compound_elif_directive_test )
{
    CppFile file;
	CppFile::StringStore definitions
	{ "LOG4CXX_ABI_VERSION=16"
    , "__GNUC__=11"
	};
    BOOST_REQUIRE(file.LoadFile("log4cxx/log4cxx.h", definitions));
    auto oldLineCount = file.GetLineCount();
    CppFile::CountType deletedLineCount;
    BOOST_CHECK_EQUAL(file.GetUpdateCount(&deletedLineCount), 3);
    BOOST_CHECK_EQUAL(deletedLineCount, 5);

    std::stringstream ss;
    file.Store(ss);
    CppFile processedFile;
    BOOST_REQUIRE(processedFile.Load(ss, definitions));
    BOOST_CHECK_EQUAL(processedFile.GetLineCount(), oldLineCount - deletedLineCount);
    BOOST_CHECK(processedFile.IsValid());
    BOOST_CHECK_EQUAL(processedFile.GetUpdateCount(), 0);
}
