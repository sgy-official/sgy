

#ifndef TEST_UNIT_TEST_DIRGUARD_H
#define TEST_UNIT_TEST_DIRGUARD_H

#include <ripple/basics/contract.h>
#include <test/jtx/TestSuite.h>
#include <boost/filesystem.hpp>

namespace ripple {
namespace test {
namespace detail {


class DirGuard
{
protected:
    using path = boost::filesystem::path;

private:
    path subDir_;
    bool rmSubDir_{false};

protected:
    beast::unit_test::suite& test_;

    auto rmDir (path const& toRm)
    {
        if (is_directory (toRm) && is_empty (toRm))
            remove (toRm);
        else
            test_.log << "Expected " << toRm.string ()
            << " to be an empty existing directory." << std::endl;
    }

public:
    DirGuard (beast::unit_test::suite& test, path subDir,
        bool useCounter = true)
        : subDir_ (std::move (subDir))
        , test_ (test)
    {
        using namespace boost::filesystem;

        static auto subDirCounter = 0;
        if (useCounter)
            subDir_ += std::to_string(++subDirCounter);
        if (!exists (subDir_))
        {
            create_directory (subDir_);
            rmSubDir_ = true;
        }
        else if (is_directory (subDir_))
            rmSubDir_ = false;
        else
        {
            Throw<std::runtime_error> (
                "Cannot create directory: " + subDir_.string ());
        }
    }

    ~DirGuard ()
    {
        try
        {
            using namespace boost::filesystem;

            if (rmSubDir_)
                rmDir (subDir_);
            else
                test_.log << "Skipping rm dir: "
                << subDir_.string () << std::endl;
        }
        catch (std::exception& e)
        {
            test_.log << "Error in ~DirGuard: " << e.what () << std::endl;
        };
    }

    path const& subdir() const
    {
        return subDir_;
    }
};


class FileDirGuard : public DirGuard
{
protected:
    path const file_;

public:
    FileDirGuard(beast::unit_test::suite& test,
        path subDir, path file, std::string const& contents,
        bool useCounter = true)
        : DirGuard(test, subDir, useCounter)
        , file_(file.is_absolute()  ? file : subdir() / file)
    {
        if (!exists (file_))
        {
            std::ofstream o (file_.string ());
            o << contents;
        }
        else
        {
            Throw<std::runtime_error> (
                "Refusing to overwrite existing file: " +
                file_.string ());
        }
    }

    ~FileDirGuard ()
    {
        try
        {
            using namespace boost::filesystem;
            if (!exists (file_))
                test_.log << "Expected " << file_.string ()
                << " to be an existing file." << std::endl;
            else
                remove (file_);
        }
        catch (std::exception& e)
        {
            test_.log << "Error in ~FileGuard: "
                << e.what () << std::endl;
        };
    }

    path const& file() const
    {
        return file_;
    }

    bool fileExists () const
    {
        return boost::filesystem::exists (file_);
    }
};

}
}
}

#endif 








