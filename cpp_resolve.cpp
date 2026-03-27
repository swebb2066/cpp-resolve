#include <boost/program_options.hpp>
#include "util/Logger.h"
#include "util/CppFile.h"
#include "util/DirectoryEntryIterator.h"
#include <iostream>

namespace po = boost::program_options;
using StringType = std::string;
using StringStore = std::vector<StringType>;

// Declare the supported options.
    po::options_description
GetOptionDescription()
{
    po::options_description data
        ( "cpp_resolve {options} {file_or_directory_list}\n\nwhere valid options are"
        );
    data.add_options()
        ("help,h", "produce help message")
        ("quiet,q", "do not print file names")
        ("count,c", "do not apply changes, list the number of lines to be removed from each file")
        ("define,d", po::value<StringStore>(), "add to the list of macro definitions")
        ("ext,e", po::value<StringStore>(), "add to the list of checked file extensions: default [.cpp, .cxx, .hpp, .h]")
        ;
    return data;
}

// Parse the program line arguments for options
void processArgs(int argc, char** argv, po::variables_map& vm)
{
    po::options_description hidden;
    hidden.add_options()
        ("file-or-dir", po::value<StringStore>(), "file or directory to process")
        ;
    po::positional_options_description p;
    p.add("file-or-dir", -1);
    po::store(po::command_line_parser(argc, argv)
        .options(GetOptionDescription().add(hidden))
        .positional(p)
        .run()
        , vm);
    po::notify(vm);
}

    static Util::LoggerPtr
log_s(Util::getLogger("main"));

int main( int argc, char* argv[] )
{
    bool ok = false;
    try
    {
        using ArgFormatter = Util::SeparatedArray<char*, char>;
        LOG4CXX_INFO(log_s, "Command: " << ArgFormatter( argv, argc, ' ', 0 ) );
        po::variables_map vm;
        processArgs(argc, argv, vm);
        bool changeFiles = 0 == vm.count("count");
        bool quiet = vm.count("quiet");

        if (!vm.count("file-or-dir") || vm.count("help"))
            std::cout << "Requires the directory or file in which to resolve macros.\n\n"
                << GetOptionDescription() << "\n";
        else
        {
            StringStore itemStore = vm["file-or-dir"].as<StringStore>();
            StringStore defineStore;
            if (vm.count("define"))
                defineStore = vm["define"].as<StringStore>();
            StringStore extStore = {".cpp", ".cxx", ".hpp", ".h"};
            if (vm.count("ext"))
            {
                StringStore extra = vm["ext"].as<StringStore>();
                extStore.insert(extStore.end(), extra.begin(), extra.end());
            }
            DirectoryEntrySelectorPtr selector(new ExtensionSelector(extStore.begin(), extStore.end()));
            DirectoryEntryIterator fileIter(itemStore.begin(), itemStore.end(), selector);
            for (fileIter.Start(); !fileIter.Off(); fileIter.Forth())
            {
                CppFile file(fileIter.Item(), defineStore);
				CppFile::CountType deletedLineCount{ 0 };
                if (!file.IsValid())
                    std::cerr << "Skipping invalid " << fileIter.Item() << "\n";
                 else if (0 < file.GetUpdateCount(&deletedLineCount))
                 {
                    LOG4CXX_INFO(log_s, fileIter.Item().string()
                        << ": " << deletedLineCount << " lines"
                        << (changeFiles ? " removed" : " removable")
                        );
                    if (changeFiles)
                        file.StoreFile(fileIter.Item());
                    if (!quiet)
                    {
                        std::cout << fileIter.Item().string()
                            << ": " << deletedLineCount << " lines"
                            << (changeFiles ? " removed" : " removable")
                            << "\n";
                    }
                }
            }
        }
        ok = true;
    }
    catch (std::exception& ex)
    {
        LOG4CXX_ERROR(log_s, ex.what());
        std::cerr << ex.what();
    }
    return ok ? 0 : 1;
}
