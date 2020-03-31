

#ifndef RIPPLE_JSON_JSON_READER_H_INCLUDED
#define RIPPLE_JSON_JSON_READER_H_INCLUDED

# define CPPTL_JSON_READER_H_INCLUDED

#include <ripple/json/json_forwards.h>
#include <ripple/json/json_value.h>
#include <boost/asio/buffer.hpp>
#include <stack>

namespace Json
{


class Reader
{
public:
    using Char = char;
    using Location = const Char*;

    
    Reader () = default;

    
    bool parse ( std::string const& document, Value& root);

    
    bool parse ( const char* beginDoc, const char* endDoc, Value& root);

    bool parse ( std::istream& is, Value& root);

    
    template<class BufferSequence>
    bool
    parse(Value& root, BufferSequence const& bs);

    
    std::string getFormatedErrorMessages () const;

    static constexpr unsigned nest_limit {25};

private:
    enum TokenType
    {
        tokenEndOfStream = 0,
        tokenObjectBegin,
        tokenObjectEnd,
        tokenArrayBegin,
        tokenArrayEnd,
        tokenString,
        tokenInteger,
        tokenDouble,
        tokenTrue,
        tokenFalse,
        tokenNull,
        tokenArraySeparator,
        tokenMemberSeparator,
        tokenComment,
        tokenError
    };

    class Token
    {
    public:
        explicit Token() = default;

        TokenType type_;
        Location start_;
        Location end_;
    };

    class ErrorInfo
    {
    public:
        explicit ErrorInfo() = default;

        Token token_;
        std::string message_;
        Location extra_;
    };

    using Errors = std::deque<ErrorInfo>;

    bool expectToken ( TokenType type, Token& token, const char* message );
    bool readToken ( Token& token );
    void skipSpaces ();
    bool match ( Location pattern,
                 int patternLength );
    bool readComment ();
    bool readCStyleComment ();
    bool readCppStyleComment ();
    bool readString ();
    Reader::TokenType readNumber ();
    bool readValue(unsigned depth);
    bool readObject(Token& token, unsigned depth);
    bool readArray (Token& token, unsigned depth);
    bool decodeNumber ( Token& token );
    bool decodeString ( Token& token );
    bool decodeString ( Token& token, std::string& decoded );
    bool decodeDouble ( Token& token );
    bool decodeUnicodeCodePoint ( Token& token,
                                  Location& current,
                                  Location end,
                                  unsigned int& unicode );
    bool decodeUnicodeEscapeSequence ( Token& token,
                                       Location& current,
                                       Location end,
                                       unsigned int& unicode );
    bool addError ( std::string const& message,
                    Token& token,
                    Location extra = 0 );
    bool recoverFromError ( TokenType skipUntilToken );
    bool addErrorAndRecover ( std::string const& message,
                              Token& token,
                              TokenType skipUntilToken );
    void skipUntilSpace ();
    Value& currentValue ();
    Char getNextChar ();
    void getLocationLineAndColumn ( Location location,
                                    int& line,
                                    int& column ) const;
    std::string getLocationLineAndColumn ( Location location ) const;
    void skipCommentTokens ( Token& token );

    using Nodes = std::stack<Value*>;
    Nodes nodes_;
    Errors errors_;
    std::string document_;
    Location begin_;
    Location end_;
    Location current_;
    Location lastValueEnd_;
    Value* lastValue_;
};

template<class BufferSequence>
bool
Reader::parse(Value& root, BufferSequence const& bs)
{
    using namespace boost::asio;
    std::string s;
    s.reserve (buffer_size(bs));
    for (auto const& b : bs)
        s.append(buffer_cast<char const*>(b), buffer_size(b));
    return parse(s, root);
}


std::istream& operator>> ( std::istream&, Value& );

} 

#endif 








