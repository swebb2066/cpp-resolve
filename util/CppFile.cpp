#include "CppFile.h"
#include <fstream>
#include <string>
#include <ctype.h>

#ifdef _MSC_VER
#pragma warning (disable : 4996) // sprintf is used in boost::wave
#endif

///////////////////////////////////////////////////////////////////////////////
//  Include Wave itself
#include <boost/wave.hpp>

///////////////////////////////////////////////////////////////////////////////
// Include the lexer stuff
#include <boost/wave/util/flex_string.hpp>
#include <boost/wave/cpplexer/cpp_lex_token.hpp>
#include <boost/wave/cpplexer/cpp_lex_iterator.hpp>

// The following file needs to be included only once throughout the whole program.
#include <boost/wave/cpplexer/re2clex/cpp_re2c_lexer.hpp>
#include "Optional.h"

/// Boost Wave types
using position_type = boost::wave::util::file_position<BOOST_WAVE_STRINGTYPE>;
using TokenType = boost::wave::cpplexer::lex_token<position_type>;
using lex_iterator_type = boost::wave::cpplexer::lex_iterator<TokenType>;
using StringType = std::string;
using PositionType = CppFile::PositionType;
using PositionPair = std::pair<PositionType, PositionType>;

// Directive data
struct ConditionPosition
{
    TokenType             token;
    PositionPair          pos;
    Util::Optional<bool>  resolvedValue;
    ConditionPosition() {}
    ConditionPosition(const TokenType& id, const PositionType& start)
        : token{ id }
        , pos{ start, start }
        {}
};
struct DirectivePosition
{
    ConditionPosition              start;
    std::vector<ConditionPosition> end;
    DirectivePosition() {}
    DirectivePosition(const TokenType& id, const PositionType& start)
        : start{ id, start }
        {}
    DirectivePosition(const ConditionPosition& initial, const std::vector<ConditionPosition>& alternates)
        : start{ initial }
        , end{ alternates }
        {}
};

    static Util::LoggerPtr
log_s(Util::getLogger("CppFile"));

// Put \c item onto \c os
    std::ostream&
operator<<(std::ostream& stream, PositionType const& item)
{
    stream << item.line << ',' << item.column;
    return stream;
}

/// Enables output in C-style escaped characters of a string
template <class StringType>
struct CStringRef
{
    StringType const& value;
    CStringRef(StringType const& value_)
        : value(value_)
        {}
};

/// Put C-style escaped characters of \c item onto \c stream
template <class StringType>
    std::ostream&
operator<<(std::ostream& stream, CStringRef<StringType> const& item)
{
    for (auto ch : item.value)
    {
        switch (ch)
        {
        case '\r':  stream << "\\r"; break;
        case '\n':  stream << "\\n"; break;
        case '\t':  stream << "\\t"; break;
        default:
            if (isprint(ch))
                stream << ch;
            else
                stream << "0x" << std::hex << int(ch);
            break;
        }
    }
    return stream;
}

struct BoostWaveToken
{
    boost::wave::token_id      id;
    BOOST_WAVE_STRINGTYPE      name;
    position_type              pos;
};

/// Put attributes of \c item onto \c stream
    std::ostream&
operator<<(std::ostream& stream, const BoostWaveToken& item)
{
    stream << boost::wave::get_token_name(item.id)
        << "(#" << BASEID_FROM_TOKEN(item.id) << ')';

    stream << ": >|" << CStringRef<TokenType::string_type>(item.name) << "|<";
    stream << " (" << item.pos.get_line()
        << ',' << item.pos.get_column()
        << ')';
    return stream;
}

/// Processing hooks that enable single file processing
class CustomDirectivesHooks
    : public boost::wave::context_policies::default_preprocessing_hooks
{
public: // Hooked methods
    // Record a new macro
    template <typename ContextT, typename TokenT, typename ParametersT, typename DefinitionT>
    void defined_macro
        ( ContextT const&    ctx
        , TokenT const&      macro_name
        , bool               is_functionlike
        , ParametersT const& parameters
        , DefinitionT const& definition
        , bool               is_predefined
        );

    /// Prevent include processing
    template <typename ContextT>
    bool found_include_directive
        ( ContextT const&    ctx
        , StringType const& filename
        , bool               include_next
        )
    {
        LOG4CXX_TRACE(log_s, "include " << filename);
        return true;    // skip all #includes
    }

    /// Record directive positions
    template <typename ContextT, typename TokenT>
    bool found_directive(ContextT const &ctx, TokenT const &directive);

    template <typename ContextT, typename TokenT, typename ContainerT>
    bool evaluated_conditional_expression
        ( ContextT const& ctx
        , TokenT const& directive
        , ContainerT const& expression
        , bool expression_value
        );

    // Record a skipped token
    template <typename ContextT, typename TokenT>
    void skipped_token(ContextT const& ctx, TokenT const& token);
};

/// A directive processing context
class CppFile::Context : public boost::wave::context
    < StringType::const_iterator
    , lex_iterator_type
    , boost::wave::iteration_context_policies::load_file_to_string
    , CustomDirectivesHooks
    , CppFile::Context
    >
{
    using BaseType = boost::wave::context
        < StringType::const_iterator
        , lex_iterator_type
        , boost::wave::iteration_context_policies::load_file_to_string
        , CustomDirectivesHooks
        , CppFile::Context
        >;
    friend class CustomDirectivesHooks;
public: // Types
    using TokenId = boost::wave::token_id;
    using IndexedToken = std::map<PositionType, TokenId>;
    using IndexMap = std::map<PositionType, PositionType>;

public: // Attributes
    CppFile* parent{ nullptr };
    std::vector<DirectivePosition> directiveStack;
    TokenType lastDirective;
    StringStore definedSymbols;

public: // ...structors
    Context(const StringType& content, const StringType& path)
        : BaseType(content.begin(), content.end(), path.c_str(), CustomDirectivesHooks{})
    {
    }
    Context
        ( CppFile* parent
        , const StringStore& definitions
        , const PathType& path
        )
        : CppFile::Context(parent->GetContent(), path.string())
    {
        this->parent = parent;
        for (auto& item : definitions)
        {
            auto pos = item.find('=');
            if (item.npos == pos)
                definedSymbols.push_back(item);
            else
                definedSymbols.push_back(item.substr(0, pos));
        }
    }

    template <typename ContainerT>
    bool IsResolved(ContainerT const& expression, PositionType* endPos) const
    {
        bool result = false;
        for (auto& item : expression)
        {
            StringType symbol(item.get_value().c_str());
            if (this->definedSymbols.end() != std::find(this->definedSymbols.begin(), this->definedSymbols.end(), symbol))
                result = true;
            if (endPos)
            {
                *endPos = PositionType
                    { CountType(item.get_position().get_line())
                    , CountType(item.get_position().get_column() + item.get_value().size())
                    };
            }
        }
        return result;
    }

    // Delete a #if directive line that:
    // - has a 'resolvedValue' set to 'true'
    // Delete each directive block that:
    // - has a 'resolvedValue' set to 'false'
    // - is an else condition of a #if block that has its 'resolvedValue' set to 'true'
    // Change a #elif to a #else if:
    // - it has its 'resolvedValue' set to 'true' and
    // - the previous #if block is not resolved
    void ProcessDirective(const DirectivePosition& directive)
    {
        LOG4CXX_DEBUG(log_s, "ProcessDirective: " << directive.start.token.get_value()
            << " @ " << directive.start.pos.first
            << " to " << directive.start.pos.second
            << " startResolvedValue? " << directive.start.resolvedValue
            << " alternateCount " << directive.end.size()
            );
        if (directive.start.resolvedValue && *directive.start.resolvedValue)
        {
            // delete the directive line
            this->parent->RemoveLines(directive.start.pos.first.line, directive.start.pos.second.line);
            // delete the else to the endif
            this->parent->RemoveLines(directive.end.front().pos.first.line, directive.end.back().pos.second.line);
        }
        else if (directive.start.resolvedValue && !*directive.start.resolvedValue)
        {
            bool deleteEndIf{ false };
            // delete until a resolved to 'true' #elif, an unresolved #elif, upto and including the #else or #endif
            auto lastDeletedLine = directive.start.pos.first.line;
            for (auto& item : directive.end)
            {
                LOG4CXX_DEBUG(log_s, item.token.get_value()
                    << " @ " << item.pos.first
                    << " to " << item.pos.second
                    << " alternateResolvedValue? " << item.resolvedValue
                    );
                lastDeletedLine = item.pos.first.line - 1;
                if ((item.resolvedValue && *item.resolvedValue) ||
                    (!item.resolvedValue && item.token == boost::wave::T_PP_ELIF))
                {
                    StringType oldText{ item.token.get_value().begin(), item.token.get_value().end() };
                    this->parent->ModifyText(item.pos.first, oldText, "#if");
                    break;
                }
                else if (item.token == boost::wave::T_PP_ELSE)
                {
                    lastDeletedLine = item.pos.first.line;
                    deleteEndIf = true;
                    break;
                }
                else if (item.token == boost::wave::T_PP_ENDIF)
                {
                    lastDeletedLine = item.pos.first.line;
                    break;
                }
            }
            this->parent->RemoveLines(directive.start.pos.first.line, lastDeletedLine);
            if (deleteEndIf)
                this->parent->RemoveLines(directive.end.back().pos.first.line);
        }
        else if (!directive.end.empty()) // !directive.start.resolvedValue
        {
            auto pItem = directive.end.begin();
            auto& start = *pItem++;
            LOG4CXX_DEBUG(log_s, start.token.get_value()
                << " @ " << start.pos.first
                << " to " << start.pos.second
                << " resolvedValue? " << start.resolvedValue
                );
            if (start.resolvedValue && *start.resolvedValue)
            {
                // Change #elif to #else and delete subsequent alternates
                this->parent->ReplaceLines(start.pos.first.line, start.pos.second.line, "#else");
                if (2 < directive.end.size())
                {
                    auto firstDeletedLine = directive.end[1].pos.first.line;
                    auto lastDeletedLine = directive.end.back().pos.first.line - 1;
                    this->parent->RemoveLines(firstDeletedLine, lastDeletedLine);
                }
            }
            else if (start.resolvedValue && !*start.resolvedValue)
            {
                // delete until a resolved to 'true' #elif, an unresolved #elif, upto the #else or #endif
                auto lastDeletedLine = start.pos.first.line;
                while (pItem != directive.end.end())
                {
                    auto& item = *pItem++;
                    LOG4CXX_DEBUG(log_s, item.token.get_value()
                        << " @ " << item.pos.first
                        << " to " << item.pos.second
                        << " alternateResolvedValue? " << item.resolvedValue
                        );
                    lastDeletedLine = item.pos.first.line - 1;
                    if ((item.resolvedValue && *item.resolvedValue) ||
                        (!item.resolvedValue && item.token == boost::wave::T_PP_ELIF))
                    {
                        StringType oldText{ item.token.get_value().begin(), item.token.get_value().end() };
                        this->parent->ModifyText(item.pos.first, oldText, "#if");
                        break;
                    }
                    else if (item.token == boost::wave::T_PP_ELSE ||
                             item.token == boost::wave::T_PP_ENDIF)
                    {
                        lastDeletedLine = item.pos.first.line - 1;
                        break;
                    }
                }
                this->parent->RemoveLines(start.pos.first.line, lastDeletedLine);
            }
        }
    }
};

// Record a new macro
template <typename ContextT, typename TokenT, typename ParametersT, typename DefinitionT>
void CustomDirectivesHooks::defined_macro
    ( ContextT const&    ctx
    , TokenT const&      macro_name
    , bool               is_functionlike
    , ParametersT const& parameters
    , DefinitionT const& definition
    , bool               is_predefined
    )
{
    LOG4CXX_TRACE(log_s, "defined_macro " << macro_name.get_value()
        << " parameters (" << boost::wave::util::impl::as_string(parameters) << ')'
        << " is_predefined? " << is_predefined
        );
}

// Record a skipped token
template <typename ContextT, typename TokenT>
void CustomDirectivesHooks::skipped_token(ContextT const& ctx, TokenT const& token)
{
    LOG4CXX_TRACE(log_s, "skipped >|" << CStringRef<BOOST_WAVE_STRINGTYPE>(token.get_value()) << "|<");
    switch (token)
    {
    case boost::wave::T_PP_IF:
    case boost::wave::T_PP_IFDEF:
    case boost::wave::T_PP_IFNDEF:
    case boost::wave::T_PP_ELSE:
    case boost::wave::T_PP_ELIF:
    case boost::wave::T_PP_ENDIF:
        CppFile::Context& derivedContext = const_cast<CppFile::Context&>(ctx.derived());
        if (!(token.get_position() == derivedContext.lastDirective.get_position()))
            found_directive(ctx, token);
        break;
    }
}

/// Record directive positions
template <typename ContextT, typename TokenT>
bool CustomDirectivesHooks::found_directive(ContextT const &ctx, TokenT const &directive)
{
    auto current_position = directive.get_position();
    LOG4CXX_TRACE(log_s, "found_directive"
        << '(' << current_position.get_line() << ')'
        << ' ' << directive.get_value()
        );
    bool skip = false;
    auto pos = PositionType
        { CppFile::CountType(current_position.get_line())
        , CppFile::CountType(current_position.get_column())
        };
    CppFile::Context& derivedContext = const_cast<CppFile::Context&>(ctx.derived());
    auto& directiveStack = derivedContext.directiveStack;
    switch (directive)
    {
    case boost::wave::T_PP_IF:
    case boost::wave::T_PP_IFDEF:
    case boost::wave::T_PP_IFNDEF:
        directiveStack.push_back(DirectivePosition{ directive, pos });
        derivedContext.lastDirective = directive;
        break;
    case boost::wave::T_PP_ELSE:
        if (directiveStack.empty())
        {
            LOG4CXX_WARN(log_s, "Missing #if"
                << " at " << ctx.get_current_filename()
                << '(' << current_position.get_line()
                << ',' << current_position.get_column() << ')'
                );
            break;
        }
        LOG4CXX_TRACE(log_s, "else " << " start@ " << directiveStack.back().start.pos.first);
        directiveStack.back().end.push_back(ConditionPosition{ directive, pos });
        derivedContext.lastDirective = directive;
        break;
    case boost::wave::T_PP_ELIF:
        if (directiveStack.empty())
        {
            LOG4CXX_WARN(log_s, "Missing #if"
                << " at " << ctx.get_current_filename()
                << '(' << current_position.get_line()
                << ',' << current_position.get_column() << ')'
                );
            break;
        }
        if (directiveStack.back().end.empty())
            LOG4CXX_TRACE(log_s, "elif[0]" << " start@ " << directiveStack.back().start.pos.first);
        else
        {
            LOG4CXX_TRACE(log_s, "elif[" << directiveStack.back().end.size() << ']'
                << " start@ " << directiveStack.back().end.back().pos.first
                );
        }
        directiveStack.back().end.push_back(ConditionPosition{ directive, pos });
        derivedContext.lastDirective = directive;
        break;
    case boost::wave::T_PP_ENDIF:
        if (directiveStack.empty())
        {
            LOG4CXX_WARN(log_s, "Missing #if"
                << " at " << ctx.get_current_filename()
                << '(' << current_position.get_line()
                << ',' << current_position.get_column() << ')'
                );
            break;
        }
        LOG4CXX_TRACE(log_s, "endif" << " start@ " << directiveStack.back().start.pos.first);
        directiveStack.back().end.push_back(ConditionPosition{ directive, pos });
        derivedContext.ProcessDirective(directiveStack.back());
        directiveStack.pop_back();
        derivedContext.lastDirective = directive;
        break;
    default:
        skip = true;
        break;
    }
    return skip;
}

template <typename ContextT, typename TokenT, typename ContainerT>
bool CustomDirectivesHooks::evaluated_conditional_expression
    ( ContextT const& ctx
    , TokenT const& directive
    , ContainerT const& expression
    , bool expression_value
    )
{
    LOG4CXX_TRACE(log_s, "directive " << directive.get_value()
        << " conditional_expression >|" << boost::wave::util::impl::as_string(expression) << "|<"
        << " value " << expression_value
        );

    auto current_position = directive.get_position();
    CppFile::Context& derivedContext = const_cast<CppFile::Context&>(ctx.derived());
    auto& directiveStack = derivedContext.directiveStack;
    if (directiveStack.empty())
    {
        LOG4CXX_WARN(log_s, "Missing #if"
            << " at " << ctx.get_current_filename()
            << '(' << current_position.get_line()
            << ',' << current_position.get_column() << ')'
            );
        return false;
    }
    auto& conditionData = directiveStack.back().end.empty()
        ? directiveStack.back().start
        : directiveStack.back().end.back();
    if (derivedContext.IsResolved(expression, &conditionData.pos.second))
    {
        conditionData.resolvedValue
            = boost::wave::T_PP_IFNDEF == directive
            ? !expression_value
            : expression_value;
    }
    return false; // ok to continue, do not re-evaluate expression
}

///////////////////////////////////////////////////////////////////////////////
//  CppFile implementation

/// The index into \c m_content corresponding to (1-based) index.line and index.col
    auto
CppFile::GetContentIndex(const PositionType& index) const -> size_t
{
    size_t result;
    if (0 < index.line && index.line <= m_lineIndex.size())
        result = m_lineIndex[index.line - 1] + index.column - 1;
    else
        result = m_content.size();
    LOG4CXX_TRACE(log_s, "GetContentIndex: " << index << " result " << result);
    return result;
}

/// The number of instances of the identifier \c name
    auto
CppFile::GetIdentifierCount(const StringType& name) const -> CountType
{
    CountType result = 0;
    auto pItem = m_identiferPositions.find(name);
    if (m_identiferPositions.end() != pItem)
        result = static_cast<CountType>(pItem->second.size());
    return result;
}

/// The number of function call style usages of the identifier \c name
    auto
CppFile::GetFunctionCount(const StringType& name) const -> CountType
{
    CountType result = 0;
    auto pItem = m_identiferPositions.find(name);
    if (m_identiferPositions.end() == pItem)
        ;
    else for (auto& item : pItem->second)
    {
        boost::wave::token_id tokenId = GetNonWhitespaceTokenAfter(item);
        if (boost::wave::T_LEFTPAREN == tokenId)
            ++result;
    }
    return result;
}

/// The number of updates
    auto
CppFile::GetUpdateCount(CountType* deletedLineCount) const -> CountType
{
    if (deletedLineCount)
        *deletedLineCount = m_deletedLineCount;
    return static_cast<CountType>(m_updates.size());
}

/// Has an been made between \c start and before \c end
    bool
CppFile::HasUpdateBetween(const PositionType& start, const PositionType& end) const
{
    static const int MaxUpdatesPerPosition = 100;
    UpdateKey keyStart(start, -MaxUpdatesPerPosition);
    UpdateKey keyEnd(end, MaxUpdatesPerPosition);
    auto pUpdate = m_updates.lower_bound(keyStart);
    bool found = m_updates.end() != pUpdate && pUpdate->first < keyEnd;
    LOG4CXX_TRACE(log_s, "HasUpdateBetween: " << start << " and " << end << " found? " << found);
    return found;
}

/// The id (and optionally position) of the first compiler token before \c index
    boost::wave::token_id
CppFile::GetNonWhitespaceTokenBefore(const PositionType& index, PositionType* resultIndex) const
{
    boost::wave::token_id result = boost::wave::T_EOI;
    auto pItem = m_tokenPositions.lower_bound(index);
    while (m_tokenPositions.end() != pItem && (pItem->first == index
        || IS_CATEGORY(pItem->second, boost::wave::WhiteSpaceTokenType)
        || IS_CATEGORY(pItem->second, boost::wave::EOLTokenType) ))
      --pItem;
    if (m_tokenPositions.end() != pItem)
    {
        result = pItem->second;
        LOG4CXX_TRACE(log_s, "GetNonWhitespaceTokenBefore: " << index
            << " token " << boost::wave::get_token_name(result)
            << " at " << pItem->first
            );
        if (resultIndex)
            *resultIndex = pItem->first;
    }
    return result;
}

/// The id (and optionally position) of the first compiler token before the parenthesis matching \c index
    boost::wave::token_id
CppFile::GetNonWhitespaceTokenBeforeOtherParen(const PositionType& index, PositionType* resultIndex) const
{
    LOG4CXX_TRACE(log_s, "GetNonWhitespaceTokenBeforeOtherParen: " << index);
    boost::wave::token_id result = boost::wave::T_EOI;
    auto pMate = m_parenMate.find(index);
    if (m_parenMate.end() != pMate)
        result = GetNonWhitespaceTokenBefore(pMate->second, resultIndex);
    return result;
}

/// The id (and optionally position) of the first compiler token after \c index
    boost::wave::token_id
CppFile::GetNonWhitespaceTokenAfter(const PositionType& index, PositionType* resultIndex) const
{
    boost::wave::token_id result = boost::wave::T_EOI;
    auto pItem = m_tokenPositions.upper_bound(index);
    while (m_tokenPositions.end() != pItem && (pItem->first == index
        || IS_CATEGORY(pItem->second, boost::wave::WhiteSpaceTokenType)
        || IS_CATEGORY(pItem->second, boost::wave::EOLTokenType) ))
      ++pItem;
    if (m_tokenPositions.end() != pItem)
    {
        LOG4CXX_TRACE(log_s, "GetNonWhitespaceTokenAfter: " << index
            << " token " << boost::wave::get_token_name(pItem->second)
            << " at " << pItem->first
            );
        if (resultIndex)
            *resultIndex = pItem->first;
        result = pItem->second;
    }
    return result;
}

/// Has this been loaded?
    bool
CppFile::IsValid() const
{
    return m_lineIndex.size() - 1 <= m_processed.line;
}

/// Load \c path into attributes
    bool
CppFile::LoadFile(const PathType& path, const StringStore& definitions)
{
    LOG4CXX_DEBUG(log_s, "LoadFile: " << path);
    m_path = path;
    std::ifstream instream(m_path.c_str());
    if (!instream.is_open())
        return false;
    return Load(instream, definitions);
}

/// Load \c is into attributes
    bool
CppFile::Load(std::istream& is, const StringStore& definitions)
{
    is.unsetf(std::ios::skipws);
    bool ok = false;
    position_type current_position;
    try
    {
        m_identiferPositions.clear();
        m_parenMate.clear();
        m_tokenPositions.clear();
        m_content = StringType
            ( std::istreambuf_iterator<char>(is.rdbuf())
            , std::istreambuf_iterator<char>()
            );
        SetLineIndex();
        CppFile::Context ctx(this, definitions, m_path);
        ctx.set_language(boost::wave::enable_preserve_comments(ctx.get_language()));
        for (auto& item : definitions)
            ctx.add_macro_definition(item, true);

        std::vector<PositionType> parenStack;
        CppFile::Context::iterator_type first = ctx.begin();
        CppFile::Context::iterator_type last = ctx.end();
        while (first != last)
        {
            boost::wave::token_id token = *first;
            BoostWaveToken item{ token, first->get_value(), first->get_position() };
            LOG4CXX_TRACE(log_s, item);
            current_position = first->get_position();
            m_processed = PositionType
                { CountType(current_position.get_line())
                , CountType(current_position.get_column())
                };
            if (boost::wave::T_LEFTPAREN == token)
                parenStack.push_back(m_processed);
            else if (boost::wave::T_RIGHTPAREN == token && !parenStack.empty())
            {
                LOG4CXX_TRACE(log_s, "LeftParen " << parenStack.back());
                m_parenMate[m_processed] = parenStack.back();
                m_parenMate[parenStack.back()] = m_processed;
                parenStack.pop_back();
            }
            else if (boost::wave::T_IDENTIFIER == token)
            {
                StringType identifier = first->get_value().c_str();
                m_identiferPositions[identifier].push_back(m_processed);
            }
            else if (boost::wave::T_UNKNOWN == token)
            {
                LOG4CXX_WARN(log_s, "Unknown token (" << CStringRef<BOOST_WAVE_STRINGTYPE>(first->get_value()) << ')'
                    << " at " << m_path
                    << '(' << current_position.get_line()
                    << ',' << current_position.get_column() << ')'
                    );
            }
            if (boost::wave::T_CCOMMENT == token)
                m_processed.line += boost::wave::context_policies::util::ccomment_count_newlines(*first);
            m_tokenPositions[m_processed] = token;
            ++first;
        }
        ok = true;
    }
    catch (boost::wave::cpplexer::lexing_exception const& e)
    {
        LOG4CXX_WARN(log_s, e.description()
            << " at " << e.file_name()
            << '(' << e.line_no() << ')'
            );
    }
    catch (boost::wave::cpp_exception const& e)
    {
        LOG4CXX_WARN(log_s, e.description()
            << " at " << e.file_name()
            << '(' << e.line_no() << ')'
            );
    }
    catch (std::exception const& e)
    {
        LOG4CXX_WARN(log_s, e.what()
            << " at " << current_position.get_file()
            << '(' << current_position.get_line() << ')'
            );
    }
    catch (...)
    {
        LOG4CXX_WARN(log_s, "unexpected exception caught"
            << " at " << current_position.get_file()
            << '(' << current_position.get_line() << ')'
            );
    }
    return ok;
}

/// Append \c text after \c lineCol
    void
CppFile::AppendText(const PositionType& lineCol, const StringType& text)
{
    LOG4CXX_DEBUG(log_s, "AppendText: " << CStringRef<StringType>(text) << " at " << lineCol);
    size_t contentIndex = GetContentIndex(lineCol) + 1;
    UpdateData newText = {contentIndex, Insert, text, contentIndex};
    UpdateKey key(lineCol, 0);
    while (0 < m_updates.count(key))
        ++key.second;
    m_updates[key] = newText;
}

/// Insert \c text before \c lineCol
    void
CppFile::InsertText(const PositionType& lineCol, const StringType& text)
{
    LOG4CXX_DEBUG(log_s, "InsertText: " << CStringRef<StringType>(text) << " at " << lineCol);
    size_t contentIndex = GetContentIndex(lineCol);
    UpdateData newText = {contentIndex, Insert, text, contentIndex};
    UpdateKey key(lineCol, 0);
    while (0 < m_updates.count(key))
        --key.second;
    m_updates[key] = newText;
}

/// Replace \c oldText at \c lineCol with \c newText
    void
CppFile::ModifyText(const PositionType& lineCol, const StringType& oldText, const StringType& newText)
{
    LOG4CXX_DEBUG(log_s, "ModifyText:" << " at " << lineCol << " oldText " << oldText << " newText " << newText);
    size_t startIndex = GetContentIndex(lineCol);
    size_t resumeIndex = startIndex;
    resumeIndex += oldText.size();
    UpdateData newData = {startIndex, Modify, newText, resumeIndex};
    UpdateKey key(lineCol, 0);
    while (0 < m_updates.count(key))
        --key.second;
    m_updates[key] = newData;
}

/// Remove the lines from (1-based) \c first to \c last
    void
CppFile::RemoveLines(CountType first, CountType last)
{
    LOG4CXX_DEBUG(log_s, "RemoveLines:" << " first " << first << " last " << last);
    PositionType lineCol{first, 1};
    size_t startIndex = GetContentIndex(lineCol);
    size_t resumeIndex = first < last
        ? GetContentIndex(PositionType{last + 1, 1})
        : GetContentIndex(PositionType{first + 1, 1});
    UpdateData skipData = {startIndex, Delete, {}, resumeIndex};
    UpdateKey key(lineCol, 0);
    while (0 < m_updates.count(key))
        --key.second;
    m_updates[key] = skipData;
    m_deletedLineCount += first < last ? (last - first + 1) : 1;
}

/// Replace the lines from (1-based) \c first to \c last with \c newText
    void
CppFile::ReplaceLines(CountType first, CountType last, const StringType& newText)
{
    LOG4CXX_DEBUG(log_s, "ReplaceLines:" << " first " << first << " last " << last << " newText " << newText);
    PositionType lineCol{first, 1};
    size_t startIndex = GetContentIndex(lineCol);
    size_t resumeIndex = first < last
        ? GetContentIndex(PositionType{last + 1, 1})
        : GetContentIndex(PositionType{first + 1, 1});
    --resumeIndex; // At the end of the previous line
    UpdateData changeData = {startIndex, Modify, newText, resumeIndex};
    UpdateKey key(lineCol, 0);
    while (0 < m_updates.count(key))
        --key.second;
    m_updates[key] = changeData;
    m_deletedLineCount += first < last ? (last - first + 1) : 1;
}

/// Initialize m_lineIndex
    void
CppFile::SetLineIndex()
{
    m_lineIndex.clear();
    m_lineIndex.push_back(0);
    for (;;)
    {
        size_t eol = m_content.find('\n', m_lineIndex.back());
        if (m_content.npos == eol)
            break;
        m_lineIndex.push_back(static_cast<CountType>(eol + 1));
    }
    LOG4CXX_DEBUG(log_s, "SetLineIndex:" << " contentSize " << m_content.size() << " lineCount " << m_lineIndex.size());
}


/// Write the (possibly) modified content to \c path
    bool
CppFile::StoreFile(const PathType& path)
{
    LOG4CXX_DEBUG(log_s, "StoreFile: " << path);
    std::ofstream stream(path.c_str());
    Store(stream);
    stream.close();
    return !stream.bad();
}

/// Write the (possibly) modified content to \c os
    void
CppFile::Store(std::ostream& os)
{
    size_t outIndex = 0;
    for (UpdateMap::const_iterator pUpdate = m_updates.begin()
        ; pUpdate != m_updates.end()
        ; ++pUpdate)
    {
        LOG4CXX_TRACE(log_s, "Store:" << " at " << pUpdate->first.first);
        const EditType& editType = pUpdate->second.type;
        size_t copyToIndex = pUpdate->second.at;
        if (outIndex < copyToIndex)
        {
            LOG4CXX_TRACE(log_s, "Store:" << " copy " << outIndex << " to " << copyToIndex);
            os << m_content.substr(outIndex, copyToIndex - outIndex);
        }
        if (Delete != editType)
        {
            LOG4CXX_TRACE(log_s, "Store:" << " insert " << CStringRef<StringType>(pUpdate->second.text));
            os << pUpdate->second.text;
        }
        outIndex = pUpdate->second.resumeAt;
    }
    if (outIndex < m_content.size())
    {
        LOG4CXX_TRACE(log_s, "Store:" << " copy " << outIndex << " to " << m_content.size());
        os << m_content.substr(outIndex);
    }
}

///////////////////////////////////////////////////////////////////////////////
// FunctionIterator implementation

    Util::LoggerPtr
CppFile::FunctionIterator::m_log(Util::getLogger("FunctionIterator"));

/// An Off() iterator for function call names starting with \c prefix
CppFile::FunctionIterator::FunctionIterator(CppFile& file, const StringType& prefix)
    : m_file(file)
    , m_prefix(prefix)
    , m_identifier(m_file.m_identiferPositions.end())
{}

/// Skip function calls matching \c identifierPrefix
    void
CppFile::FunctionIterator::AddExclusion(const StringType& identifierPrefix)
{
    LOG4CXX_DEBUG(m_log, "AddExclusion: " << identifierPrefix);
    m_exclusions.push_back(identifierPrefix);
}

/// Add a semicolan after the closing parenthesis
    void
CppFile::FunctionIterator::AddSemicolon()
{
    LOG4CXX_DEBUG(m_log, "AddSemicolon: " << m_item.paramEnd);
    PositionType insertPos = { m_item.paramEnd.line, m_item.paramEnd.column + 1};
    m_file.InsertText(insertPos, ";");
}

/// Add an opening before the function and a closing brace after the statement
    void
CppFile::FunctionIterator::InsertBraces()
{
    LOG4CXX_DEBUG(m_log, "InsertBraces: " << m_item.identifier << " to " << m_item.paramEnd);
    PositionType previousToken;
    m_file.GetNonWhitespaceTokenBefore(m_item.identifier, &previousToken);
    StringType indent;
    if (previousToken.line < m_item.identifier.line)
    {
        PositionType startOfPreviousLine = {previousToken.line, 1};
        PositionType firstTokenOfPreviousLine;
        m_file.GetNonWhitespaceTokenAfter(startOfPreviousLine, &firstTokenOfPreviousLine);
        size_t previousIndex[2] =
            { m_file.GetContentIndex(startOfPreviousLine)
            , m_file.GetContentIndex(firstTokenOfPreviousLine)
            };
        indent = m_file.m_content.substr(previousIndex[0], previousIndex[1] - previousIndex[0]);
        PositionType startOfLine = {m_item.identifier.line, 1};
        m_file.InsertText(startOfLine, indent + "{\n");
    }
    else
        m_file.InsertText(m_item.identifier, "{");
    PositionType nextToken;
    m_file.GetNonWhitespaceTokenAfter(m_item.paramEnd, &nextToken);
    if (m_item.identifier.line < nextToken.line)
    {
        PositionType startOfNextLine = {m_item.paramEnd.line + 1, 1};
        m_file.InsertText(startOfNextLine, indent + "}\n");
    }
    else
        m_file.AppendText(m_item.paramEnd, " }");
}

/// Is the next non-white-space token a semicolon or comma? - Precondition: !Off()
    bool
CppFile::FunctionIterator::HasStatementTerminator() const
{
    boost::wave::token_id tokenId = m_file.GetNonWhitespaceTokenAfter(m_item.paramEnd);
    return boost::wave::T_SEMICOLON == tokenId ||
           boost::wave::T_COLON == tokenId ||
           boost::wave::T_COMMA == tokenId;
}

/// Is the previous non-white-space token not in [else, comma, semicolon, brace]? - Precondition: !Off()
    bool
CppFile::FunctionIterator::IsCompoundStatementBody() const
{
    PositionType previousToken;
    boost::wave::token_id tokenId = m_file.GetNonWhitespaceTokenBefore(m_item.identifier, &previousToken);
    if (boost::wave::T_RIGHTPAREN == tokenId)
    {
        boost::wave::token_id statementId = m_file.GetNonWhitespaceTokenBeforeOtherParen(previousToken);
        return boost::wave::T_CATCH == statementId ||
               boost::wave::T_FOR == statementId ||
               boost::wave::T_IF == statementId ||
               boost::wave::T_SWITCH == statementId ||
               boost::wave::T_WHILE == statementId;
    }
    return !m_file.HasUpdateBetween(previousToken, m_item.identifier) &&
           boost::wave::T_ELSE != tokenId &&
           boost::wave::T_LEFTBRACE != tokenId &&
           boost::wave::T_RIGHTBRACE != tokenId &&
           boost::wave::T_COLON != tokenId &&
           boost::wave::T_SEMICOLON != tokenId &&
           boost::wave::T_COMMA != tokenId;
}

/// Move to the next item. Precondition: !Off()
    void
CppFile::FunctionIterator::Forth()
{
    ++m_instance;
    while (!OffInstance())
    {
        if (SetItem())
            return;
        ++m_instance;
    }
    ++m_identifier;
    StartInstance();
}

// Is this iterator beyond the end or before the start?
    bool
CppFile::FunctionIterator::Off() const
{
    return m_file.m_identiferPositions.end() == m_identifier ||
        0 != m_identifier->first.compare(0, m_prefix.size(), m_prefix);
}

// Set \c m_item - Precondition: !OffInstance()
    bool
CppFile::FunctionIterator::SetItem()
{
    m_item.identifier = *m_instance;
    boost::wave::token_id tokenId = m_file.GetNonWhitespaceTokenAfter(*m_instance, &m_item.paramStart);
    if (boost::wave::T_LEFTPAREN != tokenId)
        return false;
    IndexMap::const_iterator closeParen = m_file.m_parenMate.find(m_item.paramStart);
    if (m_file.m_parenMate.end() == closeParen)
        return false;
    m_item.paramEnd = closeParen->second;
    LOG4CXX_DEBUG(m_log, m_identifier->first
        << " at " << m_item.identifier
        << " to " << m_item.paramEnd
        );
    return true;
}

// Move to the first item
    void
CppFile::FunctionIterator::Start()
{
    LOG4CXX_DEBUG(m_log, "Start: " << m_prefix);
    m_identifier = m_file.m_identiferPositions.lower_bound(m_prefix);
    StartInstance();
}

// Move to the first instance of the selected identifier
    void
CppFile::FunctionIterator::StartInstance()
{
    while (!Off())
    {
        LOG4CXX_TRACE(m_log, m_identifier->first);
        if (m_exclusions.end() == std::find_if(m_exclusions.begin(), m_exclusions.end(),
            [this](const StringType& prefix) -> bool
                { return 0 == m_identifier->first.compare(0, prefix.size(), prefix); }
            ))
        {
            m_instance = m_identifier->second.begin();
            m_instanceEnd = m_identifier->second.end();
            while (!OffInstance())
            {
                if (SetItem())
                    return;
                ++m_instance;
            }
        }
        ++m_identifier;
    }
}
