#ifndef OS_H
#define OS_H

#include "basictypes.h"

namespace OS
{
    int exec(const std::string& cmd, const StringList& args, std::string* output = 0);
    /// Like cd command.
    void cd(const std::string& dir);
    /// Like pwd command.
    std::string pwd();
    /// Like mkdir -p.
    void mkdir(const std::string& dir);
    /// Returns true if \p fileName exists.
    bool fileExists(const std::string& fileName);

    class ChangeWorkingDirectory
    {
    public:
        ChangeWorkingDirectory(const std::string& dir)
        {
            m_oldDir = pwd();
            cd(dir);
        }
        ~ChangeWorkingDirectory()
        {
            cd(m_oldDir);
        }
    private:
        std::string m_oldDir;
    };
}

#endif
