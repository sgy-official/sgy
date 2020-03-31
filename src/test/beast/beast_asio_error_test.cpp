

#include <ripple/beast/asio/ssl_error.h>
#include <ripple/beast/unit_test.h>
#include <string>

namespace beast {

class error_test : public unit_test::suite
{
public:
    void run() override
    {
        {
            boost::system::error_code ec =
                boost::system::error_code (335544539,
                    boost::asio::error::get_ssl_category ());
            std::string s = beast::error_message_with_ssl(ec);
            auto const lastColon = s.find_last_of(':');
            if (lastColon != s.npos)
                s = s.substr(0, lastColon);
            BEAST_EXPECT(s == " (20,0,219) error:140000DB:SSL routines:SSL routines");
        }
    }
};

BEAST_DEFINE_TESTSUITE(error,asio,beast);

} 
























