#include "marocco/Logger.h"

#include <memory>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/consoleappender.h>
#include <colorlayout.h>

#define DEFAULT_LOGGER "marocco"

using namespace log4cxx;
using namespace std;

namespace marocco {

std::string demangle(std::string const& n)
{
	int status=0;
	std::unique_ptr<char[], decltype(&free)> name(
		abi::__cxa_demangle(n.c_str(), nullptr, 0, &status), &free);
	return status==0 ? name.get() : "__cxa_demangle error";
}

void Logger::init(LevelPtr const& level)
{
	typedef vector<pair<string, log4cxx::LevelPtr>> type;
	init(type { make_pair( DEFAULT_LOGGER, level) });
}

void Logger::init(vector<pair<string, log4cxx::LevelPtr>> const& levels)
{
	static bool is_init = false;
	if (!is_init) {
		is_init = true;

		// FIXME: reset configuration first, because there's currently no
		// clever way to ensure propper configuration.
		BasicConfigurator::resetConfiguration();

		ConsoleAppender* console = new ConsoleAppender(
			LayoutPtr(new ColorLayout()));
		BasicConfigurator::configure(AppenderPtr(console));

		for (auto const& lvl : levels)
		{
			LoggerPtr logger = log4cxx::Logger::getLogger(lvl.first);
			logger->setLevel(lvl.second);
		}
	}
}

Logger::Logger(Logger const& l) :
	mFPtr(l.mFPtr), mStream(), mLogger(l.mLogger)
{}

Logger::Logger(type f) :
	mFPtr(f), mStream(), mLogger(DEFAULT_LOGGER)
{}

Logger::~Logger()
{
	log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger(mLogger);
	((*logger).*mFPtr)(mStream.str());
}

ostream& Logger::operator<< (ostream& (*manip)(ostream&))
{
	return mStream << manip;
}

#define MAROCCO_LOGGER_SINK_IMPL(level) \
	Logger level () \
	{ \
		typedef marocco::Logger::type type; \
		return { static_cast<type>(&log4cxx::Logger:: level) }; \
	}

MAROCCO_LOGGER_SINK_IMPL(fatal)
MAROCCO_LOGGER_SINK_IMPL(error)
MAROCCO_LOGGER_SINK_IMPL(warn)
MAROCCO_LOGGER_SINK_IMPL(info)
MAROCCO_LOGGER_SINK_IMPL(debug)
MAROCCO_LOGGER_SINK_IMPL(trace)

#undef MAROCCO_LOGGER_SINK_IMPL

} // namespace marocco
