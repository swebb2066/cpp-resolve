#include <boost/program_options.hpp>
#include "util/Logger.h"
#include "util/CppFile.h"
#include "util/DirectoryEntryIterator.h"
#include <iostream>

namespace po = boost::program_options;
using StringType = std::string;
using StringStore = std::vector<StringType>;

// Declare the supported options.
static const char* substituteParam =
"add arg (to the list of identifier substitions.\n"
"Use a string of the form identifier=newIdentifier for arg.\n"
;
static const char* defineParam =
"add arg to the list of macro definitions.\n"
"Use a string of the form macroName=intValue for arg.\n"
;
static const char* extParam =
"add arg to the list of checked file extensions.\n"
"The default extension list is [.cpp, .cxx, .hpp, .h].\n"
;
static const char* countParam =
"do not apply changes.\n"
"List the number of lines to be removed from each file.\n"
;
    po::options_description
GetOptionDescription()
{
    po::options_description data
        ( "cpp_resolve {options} {file_or_directory_list}\n\nwhere valid options are"
        );
    data.add_options()
        ("help,h", "output a usage synopsis")
        ("quiet,q", "do not print file names")
        ("count,c", countParam)
        ("substitute,s", po::value<StringStore>(), substituteParam)
        ("define,d", po::value<StringStore>(), defineParam)
        ("ext,e", po::value<StringStore>(), extParam)
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
            StringStore substitutionStore;
            if (vm.count("substitute"))
                substitutionStore = vm["substitute"].as<StringStore>();
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
                auto filePath = fileIter.Item();
                CppFile file(filePath, defineStore);
                for (auto& item : substitutionStore)
                {
                    auto assignIndex = item.find('=');
                    auto identifier = item.substr(0, assignIndex);
                    StringType identifierValue;
                    if (item.npos != assignIndex)
                        identifierValue = item.substr(assignIndex + 1);
                    file.AddSubstitution(identifier, identifierValue);
                }
                CppFile::CountType deletedLineCount{ 0 };
                CppFile::CountType updateCount{ 0 };
                if (!file.IsValid())
                    std::cerr << "Skipping invalid " << filePath << "\n";
                 else if (0 < (updateCount = file.GetUpdateCount(&deletedLineCount)))
                 {
                    std::stringstream ss;
                    ss << filePath.string() << ": ";
                    if (0 < deletedLineCount)
                        ss << deletedLineCount << " lines"
                           << (changeFiles ? " removed" : " removable");
                    else if (!substitutionStore.empty())
                        ss << updateCount << " substitutions";
                    LOG4CXX_INFO(log_s, ss.str());
                    if (changeFiles)
                        file.StoreFile(filePath);
                    if (!quiet)
                        std::cout << ss.str() << "\n";
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
