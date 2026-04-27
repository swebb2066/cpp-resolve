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

BOOST_AUTO_TEST_CASE( substitution_test )
{
    CppFile file;
	file.AddSubstitution("LOG4CXX_NS", "nlog4cxx");
    BOOST_REQUIRE(file.LoadFile("log4cxx/log4cxx.h"));
    auto oldLineCount = file.GetLineCount();
    CppFile::CountType deletedLineCount;
    BOOST_CHECK_EQUAL(file.GetUpdateCount(&deletedLineCount), 2);

    std::stringstream ss;
    file.Store(ss);
    CppFile processedFile;
    BOOST_REQUIRE(processedFile.Load(ss));
    BOOST_CHECK_EQUAL(processedFile.GetLineCount(), oldLineCount - deletedLineCount);
    BOOST_CHECK(processedFile.IsValid());
    BOOST_CHECK_EQUAL(processedFile.GetUpdateCount(), 0);
}

BOOST_AUTO_TEST_CASE( deeply_embedded_resolved_true_directive_test )
{
    std::stringstream input;
    input << "#if  UNRESOLVED_MACRO_1\n"
        "#define CASE_1 1\n"
        "#elif  UNRESOLVED_MACRO_2\n"
        "#define CASE_2 1\n"
        "#elif  UNRESOLVED_MACRO_3\n"
        "#define CASE_3 1\n"
        "#elif  RESOLVED_MACRO_4\n"
        "#define CASE_4 1\n"
        "#elif  UNRESOLVED_MACRO_5\n"
        "#define CASE_5 1\n"
        "#endif\n"
        ;
    CppFile file;
	CppFile::StringStore definitions
	{ "RESOLVED_MACRO_4=1"
	};
    BOOST_REQUIRE(file.Load(input, definitions));
    auto oldLineCount = file.GetLineCount();
    CppFile::CountType deletedLineCount;
    BOOST_CHECK_EQUAL(file.GetUpdateCount(&deletedLineCount), 2);
    BOOST_CHECK_EQUAL(deletedLineCount, 2);

    std::stringstream ss;
    file.Store(ss);
    CppFile processedFile;
    BOOST_REQUIRE(processedFile.Load(ss, definitions));
    BOOST_CHECK_EQUAL(processedFile.GetLineCount(), oldLineCount - deletedLineCount);
    BOOST_CHECK(processedFile.IsValid());
    BOOST_CHECK_EQUAL(processedFile.GetUpdateCount(), 0);
}

BOOST_AUTO_TEST_CASE( deeply_embedded_resolved_false_directive_test )
{
    std::stringstream input;
    input << "#if  UNRESOLVED_MACRO_1\n"
        "#define CASE_1 1\n"
        "#elif  UNRESOLVED_MACRO_2\n"
        "#define CASE_2 1\n"
        "#elif  UNRESOLVED_MACRO_3\n"
        "#define CASE_3 1\n"
        "#elif  RESOLVED_MACRO_4\n"
        "#define CASE_4 1\n"
        "#elif  UNRESOLVED_MACRO_5\n"
        "#define CASE_5 1\n"
        "#endif\n"
        ;
    CppFile file;
	CppFile::StringStore definitions
	{ "RESOLVED_MACRO_4=0"
	};
    BOOST_REQUIRE(file.Load(input, definitions));
    auto oldLineCount = file.GetLineCount();
    CppFile::CountType deletedLineCount;
    BOOST_CHECK_EQUAL(file.GetUpdateCount(&deletedLineCount), 1);
    BOOST_CHECK_EQUAL(deletedLineCount, 2);

    std::stringstream ss;
    file.Store(ss);
    CppFile processedFile;
    BOOST_REQUIRE(processedFile.Load(ss, definitions));
    BOOST_CHECK_EQUAL(processedFile.GetLineCount(), oldLineCount - deletedLineCount);
    BOOST_CHECK(processedFile.IsValid());
    BOOST_CHECK_EQUAL(processedFile.GetUpdateCount(), 0);
}
