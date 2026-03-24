Introduction
============

This command line program can modify a source code tree of C/C++ files
deleting all but one branch of conditionally compiled preprocessor directives.

Only those conditional compilation expressions in which
all identifiers are resolved are modified.

For example, ```cpp_resovle -d LOG4CXX_ABI_VERSION=16 src``` may result in the following diff report
leaving a single line of code in place of the preprocessor code block:
```
@@ -255,11 +234,7 @@ class LOG4CXX_EXPORT Hierarchy : public spi::LoggerRepository
                @param ifNotUsed If true and use_count() indicates there are other references, do not remove the Logger and return false.
                @returns true if \c name Logger was removed from the hierarchy.
                */
-#if LOG4CXX_ABI_VERSION <= 15
-               bool removeLogger(const LogString& name, bool ifNotUsed = true);
-#else
                bool removeLogger(const LogString& name, bool ifNotUsed = true) override;
-#endif

        private:
```

Synopsis
========

cpp_resolve {options} {file_or_directory_list}

Option             | Description
-------------------|------------------------------------------------
-h [ --help ]      |   produce help message
-q [ --quiet ]     |   do not print file names
-d [ --define ] arg|   add to the list of macro definitions
-c [ --count ]     |   do not apply changes, list the number of lines to be removed from each file
-e [ --ext ] arg   |   add to the list of checked file extensions: default [.cpp, .cxx, .hpp, .h]
