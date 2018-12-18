#include "util.hpp"

#include <stdio.h>

#include <stdexcept>

#include <boost/algorithm/string/join.hpp>
#include <boost/filesystem.hpp>

#ifdef __linux__
    #include <unistd.h>
#elif _WIN32
    #include <windows.h>
#endif

namespace fs = boost::filesystem;

std::string searchImageAbsoluteFilename(const std::string & filename)
{
    // Convenient variables
    fs::path programPath(programAbsoluteFilename());
    fs::path programDirPath(programPath.parent_path());
    fs::path currentDirPath = fs::current_path();

    // Predefined search paths
    std::vector<fs::path> searchedPaths = {
        fs::path(programDirPath.string() + "/../share/hexabomb-visu/"), // Assets/program in installed hexabomb-visu.
        fs::path(programDirPath.string() + "/../assets/img/"), // Assets in git repo. Program in meson build directory.
        fs::path(currentDirPath.string() + "/assets/img/"), // Run from the git repo root.
        fs::path(currentDirPath.string() + "/") // Assets are in current directory.
    };

    std::vector<std::string> searchedPathsStrings;
    for (const auto & path : searchedPaths)
    {
        searchedPathsStrings.push_back("- " + path.string());

        fs::path searchedFilename = path.string() + filename;
        if (fs::exists(searchedFilename))
            return fs::absolute(searchedFilename).string();
    }

    throw std::runtime_error("Could not find image '" + filename + "'. Searched paths:\n" +
        boost::algorithm::join(searchedPathsStrings, "\n"));
}

std::string programAbsoluteFilename()
{
// https://stackoverflow.com/a/198099
#ifdef __linux__
    char szTmp[128];
    char pBuf[1024];
    size_t len = 1024;
    sprintf(szTmp, "/proc/%d/exe", getpid());
    int bytes = std::min((size_t)readlink(szTmp, pBuf, len), len - 1);
    if(bytes >= 0)
        pBuf[bytes] = '\0';
    return std::string(pBuf);
#elif _WIN32
    int bytes = GetModuleFileName(NULL, pBuf, len);
    if(bytes == 0)
        return "";
    else
        return std::string(bytes);
#else
    #error Unsupported system: Does not know how to get program absolute filename
#endif
}
