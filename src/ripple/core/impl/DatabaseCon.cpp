

#include <ripple/core/DatabaseCon.h>
#include <ripple/core/SociDB.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/Log.h>
#include <memory>

namespace ripple {

DatabaseCon::DatabaseCon (
    Setup const& setup,
    std::string const& strName,
    const char* initStrings[],
    int initCount)
{
    auto const useTempFiles  
        = setup.standAlone &&
          setup.startUp != Config::LOAD &&
          setup.startUp != Config::LOAD_FILE &&
          setup.startUp != Config::REPLAY;
    boost::filesystem::path pPath = useTempFiles
        ? "" : (setup.dataDir / strName);

    open (session_, "sqlite", pPath.string());

    for (int i = 0; i < initCount; ++i)
    {
        try
        {
            soci::statement st = session_.prepare <<
                initStrings[i];
            st.execute(true);
        }
        catch (soci::soci_error&)
        {
        }
    }
}

DatabaseCon::Setup setup_DatabaseCon (Config const& c)
{
    DatabaseCon::Setup setup;

    setup.startUp = c.START_UP;
    setup.standAlone = c.standalone();
    setup.dataDir = c.legacy ("database_path");
    if (!setup.standAlone && setup.dataDir.empty())
    {
        Throw<std::runtime_error>(
            "database_path must be set.");
    }

    return setup;
}

void DatabaseCon::setupCheckpointing (JobQueue* q, Logs& l)
{
    if (! q)
        Throw<std::logic_error> ("No JobQueue");
    checkpointer_ = makeCheckpointer (session_, *q, l);
}

} 
























