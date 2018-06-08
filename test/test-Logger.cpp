#include "test/common.h"
#include "marocco/Logger.h"

#define STRINGIFY(msg) #msg
#define A_LOGGER_MSG(level) \
	level () << "a " STRINGIFY(level) " msg"

namespace marocco {

TEST(Logger, NormalLogging)
{
	A_LOGGER_MSG(fatal);
	A_LOGGER_MSG(error);
	A_LOGGER_MSG(warn);
	A_LOGGER_MSG(info);
	A_LOGGER_MSG(debug);
	A_LOGGER_MSG(trace);

	warn(this) << "a warn msg in submodule";
}

TEST(Logger, MacroLogging)
{
	MAROCCO_FATAL("a fatal msg " << 42);
	MAROCCO_ERROR("a error msg " << 42);
	MAROCCO_WARN ("a  warn msg " << 42);
	MAROCCO_INFO ("a  info msg " << 42);
	MAROCCO_DEBUG("a debug msg " << 42);
	MAROCCO_TRACE("a trace msg " << 42);
}

} // marocco
