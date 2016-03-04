#pragma once

#include <functional>
#include <boost/graph/visitors.hpp>

namespace marocco {

template <typename Tag, typename... Args>
class event_visitor : public boost::base_visitor<event_visitor<Tag, Args...> >
{
public:
	typedef std::function<void(Args...)> function_type;
	typedef Tag event_filter;
	event_visitor(function_type handler) : m_handler(handler)
	{
	}

	void operator()(Args&&... args) const
	{
		m_handler(std::forward<Args>(args)...);
	}

private:
	function_type m_handler;
};

template <typename Tag, typename... Args>
inline event_visitor<Tag, Args...> make_event_visitor(std::function<void(Args...)> handler, Tag)
{
	return event_visitor<Tag, Args...>(handler);
}

} // namespace marocco
