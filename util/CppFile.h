#if !defined(CPP_FILE_INCLUDED)
#define CPP_FILE_INCLUDED
#include "Logger.h"
#include <boost/filesystem.hpp>
#include <boost/wave/token_ids.hpp>
#include <boost/wave/wave_config.hpp>
#include <map>

class CppFile
{
public: // Types
    using PathType = boost::filesystem::path;
    using StringType = std::string;
    using StringStore = std::vector<StringType>;
    using CountType = unsigned int;

    class FunctionIterator;
    struct PositionType
    {
        CountType line, column;
        bool operator<(PositionType const& other) const
        {
            return line < other.line || (line == other.line && column < other.column);
        }
        bool operator==(PositionType const& other) const
        {
            return line == other.line && column == other.column;
        }
    };

    class Context;

protected: // Types
    using PositionStore = std::vector<PositionType>;
    using StringPositionMap = std::map<StringType, PositionStore>;
    using IndexMap = std::map<PositionType, PositionType>;
    enum EditType { Delete, Insert, Modify };
    struct UpdateData
    {
        size_t     at;
        EditType   type;
        StringType text;
        size_t     resumeAt;
    };
    using UpdateKey = std::pair<PositionType, int>;
    using UpdateMap = std::map<UpdateKey, UpdateData>;
    using IndexStore = std::vector<int>;
    using TokenId = boost::wave::token_id;
    using IndexedToken = std::map<PositionType, TokenId>;

private: // Attributes
    PathType m_path;
    StringType m_content;
    IndexStore m_lineIndex;
    PositionType m_processed;
    IndexedToken m_tokenPositions;
    IndexMap m_parenMate;
    StringPositionMap m_identiferPositions;
    UpdateMap m_updates;
    CountType m_deletedLineCount{ 0 };

public: // ...structors
    CppFile() : m_path{ "<Unknown>" } {}
    CppFile(const PathType& path, const StringStore& definitions = {})
    { LoadFile(path, definitions); }

public: // Accessors
    const StringType& GetContent() const { return m_content; }
    CountType GetIdentifierCount(const StringType& name) const;
    CountType GetFunctionCount(const StringType& name) const;
    CountType GetLineCount() const { return static_cast<CountType>(m_lineIndex.size()); }
    CountType GetUpdateCount(CountType* deletedLineCount = 0) const;
    bool IsValid() const;

public: // Modifiers
    bool Load(std::istream& is, const StringStore& definitions = {});
    bool LoadFile(const PathType& path, const StringStore& definitions = {});
    bool StoreFile(const PathType& path);
    void Store(std::ostream& os);

protected: // Support methods
    void AppendText(const PositionType& lineCol, const StringType& text);
    void InsertText(const PositionType& lineCol, const StringType& text);
    void ModifyText(const PositionType& lineCol, const StringType& oldText, const StringType& newText);
    void RemoveLines(CountType first, CountType last = 0);
    void ReplaceLines(CountType first, CountType last, const StringType& newText);
    size_t GetContentIndex(const PositionType& index) const;
    boost::wave::token_id GetNonWhitespaceTokenAfter(const PositionType& index, PositionType* resultIndex = 0) const;
    boost::wave::token_id GetNonWhitespaceTokenBefore(const PositionType& index, PositionType* resultIndex = 0) const;
    boost::wave::token_id GetNonWhitespaceTokenBeforeOtherParen(const PositionType& index, PositionType* resultIndex = 0) const;
    bool HasUpdateBetween(const PositionType& start, const PositionType& end) const;
    void SetLineIndex();
};

/// Allows operations to be selectively performed on matched function call style instances
class CppFile::FunctionIterator
{
public: // Types
    //!< The current item
    struct ItemType
    {
        PositionType identifier;
        PositionType paramStart;
        PositionType paramEnd;
    };
private: // Types
    using StringStore = std::vector<StringType>;

private: // Attributes
    CppFile& m_file; //!< The owner of this
    StringType m_prefix; //!< Of the function of interest
    StringStore m_exclusions; //!< Ignored function call identifier prefixes
    ItemType m_item; //!< The current item
    StringPositionMap::const_iterator m_identifier; //!< Position in the identifier map
    PositionStore::const_iterator m_instance; //!< Position in the instances of the current identifier
    PositionStore::const_iterator m_instanceEnd; //!< Sentinal of the instances of the current identifier

public: // ...structors
    /// An Off() iterator for function call names starting with \c prefix
    FunctionIterator(CppFile& file, const StringType& prefix);

public: // Property modifiers
    /// Skip function calls matching \c identifierPrefix
    void AddExclusion(const StringType& identifierPrefix);

public: // Accessors
    /// Is the next non-white-space token a semicolon or comma? - Precondition: !Off()
    bool HasStatementTerminator() const;

    /// Is the previous non-white-space token not in [else, comma, semicolon, brace]? - Precondition: !Off()
    bool IsCompoundStatementBody() const;

    /// The current item - Precondition: !Off()
    const ItemType& Item() const { return m_item; }

    /// Is this iterator beyond the end or before the start?
    bool Off() const;

public: // Methods
    /// Add a semicolan after the closing parenthesis
    void AddSemicolon();

    /// Add an opening before the function and a closing brace after the statement
    void InsertBraces();

    /// Move to the first item
    void Start();

    /// Move to the next item. Precondition: !Off()
    void Forth();

protected: // Support methods
    /// Is \c m_instance beyond the end or before the start of the current instance collection? - Precondition: !Off
    inline bool OffInstance() const { return m_instanceEnd == m_instance; }

    /// Set \c m_item - Precondition: !OffInstance()
    bool SetItem();

    /// Move to the first instance of the selected identifier
    void StartInstance();

private: // Class data
    static Util::LoggerPtr m_log;
};

#endif // !defined(CPP_FILE_INCLUDED)
