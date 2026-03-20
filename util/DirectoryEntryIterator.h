#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include <stdexcept>

/// An base of directory entry selectors
class DirectoryEntrySelector
{
public: // Types
    typedef boost::filesystem::path PathType;

protected: // Attributes
    int m_expectedLevel;

public: // ...structors
    /// A selector (optionally) limiting the search to an \c expectedLevel tree
    DirectoryEntrySelector(int expectedLevel = -1)
        : m_expectedLevel(expectedLevel)
        {}
    virtual ~DirectoryEntrySelector() {}

public: // Accessors
    /// Is \c entry at \c level included, and if not, can the remainder of the directory be skipped?
    virtual bool IsIncluded(int level, const PathType& entry, bool& skipDirectory) const;
};
typedef boost::shared_ptr<DirectoryEntrySelector> DirectoryEntrySelectorPtr;

/// An iterator over directory entries
class DirectoryEntryIterator
{
public: // Types
    typedef boost::filesystem::path PathType;

protected: // Types
    typedef std::vector<PathType> PathStore;
    typedef boost::filesystem::recursive_directory_iterator IterType;

private: // Attributes
    PathStore m_pathStore; //!< DirectoryEntrys or directory names
    DirectoryEntrySelectorPtr m_test; //!< Selects the directory entries
    PathStore::const_iterator m_pathItem; //!< Stepped through m_pathStore
    IterType m_dirItem; //!< Steps through all the entries
    IterType m_dirEnd;
    PathType m_item; //!< The current directory entry

public: // ...structors
    DirectoryEntryIterator(const PathType& dir, const DirectoryEntrySelectorPtr& test = DirectoryEntrySelectorPtr())
        : m_pathStore(1, dir)
        , m_test(test)
        , m_pathItem(m_pathStore.end())
    { ThrowUnlessPathExists(); }

    template <class FwdIter>
    DirectoryEntryIterator(FwdIter first, FwdIter last, const DirectoryEntrySelectorPtr& test = DirectoryEntrySelectorPtr())
        : m_pathStore(first, last)
        , m_test(test)
        , m_pathItem(m_pathStore.end())
    { ThrowUnlessPathExists(); }

public: // Accessors
    /// Is this iterator beyond the end or before the start?
    bool Off() const;

    /// The full path of the current item. Precondition: !Off()
    const PathType& Item() const { return m_item; }

public: // Methods
    /// Move to the first item
    void Start();

    /// Move to the next item. Precondition: !Off()
    void Forth();

protected: // Support methods
    /// Is this iterator beyond the end or before the start of the current directory iterator?
    bool OffDir() const;

    /// Set m_item
    bool SetItem();

    bool StartDir(const PathType& dir);

    // Throw any path that does not exist
    void ThrowUnlessPathExists() const;
};

/// Select regular files based on their extension
class ExtensionSelector : public DirectoryEntrySelector
{
protected: // Types
    typedef std::vector<std::string> StringStore;

protected: // Attributes
    StringStore m_allowed;

public: // ...structors
    /// A selector of regular files with extensions names provided by the range (first,last]
    template <class FwdIter>
    ExtensionSelector(FwdIter first, FwdIter last)
        : m_allowed(first, last)
    {}

public: // Methods
    // Is \c entry at \c level a selected file, and if not, can the remainder of the directory be skipped?
    bool IsIncluded(int level, const PathType& entry, bool& skipDirectory) const;
};

class ExistsException : public std::invalid_argument
{
public: // Attributes
    boost::filesystem::path path;
public: // ...stuctors
    ExistsException(const boost::filesystem::path& name) noexcept;
};
