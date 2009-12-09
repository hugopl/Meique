#include "meique.h"
#include "logger.h"
#include "meiquescript.h"

Meique::Meique(int argc, char** argv) : m_config(argc, argv)
{
}

void Meique::exec()
{
    MeiqueScript script(m_config);
    script.exec();
    if (m_config.isInConfigureMode()) {
        Notice() << "Configuring project...";
        checkOptionsAgainstArguments(script.options());
    }
    m_config.saveCache();
}

void Meique::checkOptionsAgainstArguments(const OptionsMap& options)
{
    const StringMap args = m_config.arguments(); // Is std::map implicitly shared?
    // copy options to an strmap.
    StringMap userOptions;
    for (OptionsMap::const_iterator it = options.begin(); it != options.end(); ++it)
        userOptions[it->first] = it->second.defaultValue;

    for (StringMap::const_iterator it = args.begin(); it != args.end(); ++it) {
        if (options.find(it->first) == options.end())
            Error() << "The option \"" << it->first << "\" doesn't exists, use meique --help to see the available options.";
        userOptions[it->first] = it->second;
    }
    m_config.setUserOptions(userOptions);
}


