#pragma once

#include <boost/python.hpp>

template <typename V>
struct variant_to_object : boost::static_visitor<PyObject*>
{
	static result_type convert(V const& v)
	{
		return boost::apply_visitor(variant_to_object<V>(), v);
	}

	template <typename T>
	result_type operator()(T const& t) const
	{
		return boost::python::incref(boost::python::object(t).ptr());
	}
};
