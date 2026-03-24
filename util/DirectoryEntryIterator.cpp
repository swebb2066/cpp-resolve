#include "DirectoryEntryIterator.h"
#include "Logger.h"
#include <boost/algorithm/string.hpp>

namespace fs = boost::filesystem;

    static Util::LoggerPtr
log_s(Util::getLogger("DirectoryEntryIterator"));

ExistsException::ExistsException(const boost::filesystem::path& name) noexcept
        : std::invalid_argument(name.string() + " not found")
        , path(name)
    {}

// Throw any path that does not exist
    void
DirectoryEntryIterator::ThrowUnlessPathExists() const
{
    for ( PathStore::const_iterator pItem = m_pathStore.begin()
        ; m_pathStore.end() != pItem
        ; ++pItem)
    {
        PathType path(*pItem);
        if (!exists(path))
            throw ExistsException(path);
    }
}

// Move to the first item
    void
DirectoryEntryIterator::Start()
{
    m_pathItem = m_pathStore.begin();
    while (m_pathStore.end() != m_pathItem)
    {
        fs::path path(*m_pathItem);
        if (!fs::is_directory(path))
        {
            m_item = path;
            break;
        }
        else if (StartDir(path))
            break;
        ++m_pathItem;
    }
}

// Move to the first item in \c dir
    bool
DirectoryEntryIterator::StartDir(const fs::path& dir)
{
    m_dirItem = fs::recursive_directory_iterator(dir);
    return SetItem();
}

// Move to the next item. Precondition: !Off()
    void
DirectoryEntryIterator::Forth()
{
    if (!OffDir())
        ++m_dirItem;
    if (!SetItem())
    {
        ++m_pathItem;
        while (m_pathStore.end() != m_pathItem)
        {
            fs::path path(*m_pathItem);
            if (!fs::is_directory(path))
            {
                m_item = path;
                break;
            }
            else if (StartDir(path))
                break;
            ++m_pathItem;
        }
    }
}

// Is this iterator beyond the end or before the start?
    bool
DirectoryEntryIterator::Off() const
{
    return m_pathStore.end() == m_pathItem;
}

// Is this iterator beyond the end or before the start of the current directory iterator?
    bool
DirectoryEntryIterator::OffDir() const
{
    return m_dirItem == m_dirEnd;
}

// Set m_item
    bool
DirectoryEntryIterator::SetItem()
{
    while (!OffDir())
    {
        bool skipDirectory = false;
        if (!m_test || m_test->IsIncluded(m_dirItem.level(), m_dirItem->path(), skipDirectory))
        {
            m_item = m_dirItem->path();
            return true;
        }
        if (skipDirectory && !fs::is_directory(m_dirItem->path()))
            m_dirItem.pop();
        else
        {
            m_dirItem.no_push(skipDirectory);
            ++m_dirItem;
        }
    }
    return false;
}

// Is \c entry at \c level included, and if not, can the remainder of the directory be skipped?
    bool
DirectoryEntrySelector::IsIncluded(int level, const fs::path& entry, bool& skipDirectory) const
{
    bool result = m_expectedLevel < 0 || level <= m_expectedLevel;
    if (!result)
        skipDirectory = true;
    return result;
}

// Is \c entry at \c level a selected file, and if not, can the remainder of the directory be skipped?
    bool
ExtensionSelector::IsIncluded(int level, const fs::path& entry, bool& skipDirectory) const
{
    bool result = DirectoryEntrySelector::IsIncluded(level, entry, skipDirectory);
    if (!fs::is_regular_file(entry))
        result = false;
    else if (result)
    {
        std::string ext = entry.extension().string();
        result = m_allowed.end() != std::find(m_allowed.begin(), m_allowed.end(), ext);
    }
    LOG4CXX_DEBUG(log_s, "IsIncluded: level " << level << ' ' << entry << " result " << result << " skipDirectory? " << skipDirectory);
    return result;
}
