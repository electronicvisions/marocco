#pragma once

// apache logger
#include <log4cxx/logger.h>

#include <pywrap/compat/debug.hpp>
#include <sstream>
#include <vector>
#include <cxxabi.h>
#include <typeinfo>
#include <string>

namespace marocco {

std::string demangle(std::string const& n);

template <typename T>
inline std::string typestring(T t)
{
	return demangle(typeid(t).name());
}

template <typename T>
inline std::string typestring()
{
	return demangle(typeid(T).name());
}



class Logger
{
public:
	typedef void (log4cxx::Logger:: *type)(std::string const&) const;

	static void init(log4cxx::LevelPtr const& level = log4cxx::Level::getAll());
	static void init(std::vector<std::pair<std::string, log4cxx::LevelPtr> > const& levels);

	template<typename T>
	Logger(type f, T const* _this);
	Logger(type f);
	Logger(Logger const& l);
	~Logger();

	template<typename T>
	std::ostream& operator<< (T const& /*t*/);
	std::ostream& operator<< (std::ostream& (* /*manip*/)(std::ostream&));

	template<typename T>
	static char const* getLogger(T const* t);

private:
	template<typename T>
	static std::string getLogger_(T const* t);

	type const mFPtr;
	std::ostringstream mStream;
	std::string mLogger;
};


#define MAROCCO_LOGGER_SINK(level) \
	template<typename T> \
	marocco::Logger level (T const* t) \
	{ \
		typedef marocco::Logger::type type; \
		return marocco::Logger(static_cast<type>(&log4cxx::Logger:: level), t); \
	} \
	marocco::Logger level ();

MAROCCO_LOGGER_SINK(fatal)
MAROCCO_LOGGER_SINK(error)
MAROCCO_LOGGER_SINK(warn)
MAROCCO_LOGGER_SINK(info)
MAROCCO_LOGGER_SINK(debug)
MAROCCO_LOGGER_SINK(trace)

#undef MAROCCO_LOGGER_SINK


#define __MAROCCO_LOGGER_MACRO(level, ...) \
	LOG4CXX_ ## level ( \
		log4cxx::Logger::getLogger(marocco::Logger::getLogger(this)), \
		__VA_ARGS__ )

#if !defined(MAROCCO_NDEBUG)
#define MAROCCO_FATAL(...) __MAROCCO_LOGGER_MACRO(FATAL, __VA_ARGS__ )
#define MAROCCO_ERROR(...) __MAROCCO_LOGGER_MACRO(ERROR, __VA_ARGS__ )
#define MAROCCO_WARN(...) __MAROCCO_LOGGER_MACRO(WARN, __VA_ARGS__ )
#define MAROCCO_INFO(...) __MAROCCO_LOGGER_MACRO(INFO, __VA_ARGS__ )
#define MAROCCO_DEBUG(...) __MAROCCO_LOGGER_MACRO(DEBUG, __VA_ARGS__ )
#define MAROCCO_TRACE(...) __MAROCCO_LOGGER_MACRO(TRACE, __VA_ARGS__ )
#else
#define MAROCCO_FATAL(...) __MAROCCO_LOGGER_MACRO(FATAL, __VA_ARGS__ )
#define MAROCCO_ERROR(...) __MAROCCO_LOGGER_MACRO(ERROR, __VA_ARGS__ )
#define MAROCCO_WARN(...) __MAROCCO_LOGGER_MACRO(WARN, __VA_ARGS__ )
#define MAROCCO_INFO(...)
#define MAROCCO_DEBUG(...)
#define MAROCCO_TRACE(...)
#endif


template<typename T>
std::ostream& Logger::operator<< (T const& t)
{
	return mStream<< t;
}

template<typename T>
Logger::Logger(type f, T const* t) :
	mFPtr(f), mStream(), mLogger()
{
	mLogger = getLogger(t);
}

template<typename T>
char const* Logger::getLogger(T const* t)
{
	static std::string const logger = getLogger_(t);
	return logger.c_str();
}

template<typename T>
std::string Logger::getLogger_(T const* /*instance*/)
{
	std::string logger = typestring<T>();
	size_t pos = logger.find("::");
	while(pos != std::string::npos) {
		logger.replace(pos, 2, ".");
		pos = logger.find("::");
	}
	return logger;
}

} // namespace marocco
