#include <emscripten/bind.h>

#include <boost/lexical_cast.hpp>
#include <boost/mpl/for_each.hpp>

#include "Marocco.h"

namespace { // helpers

/* some overload-in-js-not-supported helpers */
size_t num_buses_left(marocco::results::HICANNOnWaferProperties& self)
{
	return self.num_buses(halco::common::left);
}

size_t num_buses_right(marocco::results::HICANNOnWaferProperties& self)
{
	return self.num_buses(halco::common::right);
}

size_t num_buses_horizontal(marocco::results::HICANNOnWaferProperties& self)
{
	return self.num_buses(halco::common::horizontal);
}

size_t num_buses_vertical(marocco::results::HICANNOnWaferProperties& self)
{
	return self.num_buses(halco::common::vertical);
}

struct store_and_convert_to_string
{
	template<typename T> void operator()(T const& item)
	{
		std::string name = boost::lexical_cast<std::string>(item);
		m_names.push_back(name);
	}

	std::vector<std::string> m_names;
};

std::string L1Route_segment_type_to_name(marocco::L1Route::segment_type& v)
{
	store_and_convert_to_string visitor;
	boost::apply_visitor(visitor, v);
	if (visitor.m_names.size() != 1) {
		throw std::runtime_error("store_and_convert_to_string.m_names.size() should be 1");
	}
	return visitor.m_names[0];
}

std::vector<std::string> L1Route_Segment_type_to_name_list() {
	store_and_convert_to_string visitor;
	boost::mpl::for_each<marocco::L1Route::segment_type::types>(boost::ref(visitor));
	return visitor.m_names;
}

}


EMSCRIPTEN_BINDINGS(marocco_results)
{
	emscripten::class_<marocco::results::Marocco>("Marocco")
		.constructor<>()
		.class_function("from_file", &marocco::results::Marocco::from_file)
		.function("load", &marocco::results::Marocco::load)
		.function("save", &marocco::results::Marocco::save)
		.function("properties", &marocco::results::Marocco::properties)
		.function("l1_properties", &marocco::results::Marocco::l1_properties)
	;
	emscripten::class_<marocco::results::HICANNOnWaferProperties>("HICANNOnWaferProperties")
		.constructor<>()
		.function("is_available", &marocco::results::HICANNOnWaferProperties::is_available)
		.function("is_transit_only", &marocco::results::HICANNOnWaferProperties::is_transit_only)
		.function("has_neurons", &marocco::results::HICANNOnWaferProperties::has_neurons)
		.function("has_inputs", &marocco::results::HICANNOnWaferProperties::has_inputs)
		.function("num_neurons", &marocco::results::HICANNOnWaferProperties::num_neurons)
		.function("num_inputs", &marocco::results::HICANNOnWaferProperties::num_inputs)
		.function("num_buses_left", &num_buses_left)
		.function("num_buses_right", &num_buses_right)
		.function("num_buses_horizontal", &num_buses_horizontal)
		.function("num_buses_vertical", &num_buses_vertical)
	;

	{
		typedef halco::common::detail::BaseType<halco::common::Enum, size_t> base_t;
		emscripten::class_<base_t>("_details_BaseType_Enum")
			.function("value", &base_t::value)
			.function("toEnum", &base_t::toEnum)
		;
		emscripten::class_<halco::common::Enum, emscripten::base<base_t> >("Enum")
			.constructor<>()
			.constructor<size_t>()
		;
	}

	{
		typedef halco::common::detail::BaseType<halco::common::X, size_t> basex_t;
		emscripten::class_<basex_t>("_details_BaseType_X")
			.function("value", &basex_t::value)
			.function("toEnum", &basex_t::toEnum)
		;
		emscripten::class_<halco::common::X, emscripten::base<basex_t> >("X")
			.constructor<>()
			.constructor<size_t>()
		;
	}

	{
		typedef halco::common::detail::BaseType<halco::common::Y, size_t> basey_t;
		emscripten::class_<basey_t>("_details_BaseType_Y")
			.function("value", &basey_t::value)
			.function("toEnum", &basey_t::toEnum)
		;
		emscripten::class_<halco::common::Y, emscripten::base<basey_t> >("Y")
			.constructor<>()
			.constructor<size_t>()
		;
	}

	{
		typedef halco::common::detail::RantWrapper<
			halco::common::XRanged<35, 0>, halco::common::X::value_type, 35, 0>
			rant_t;
		emscripten::class_<rant_t>("_details_RantWrapper_x_type")
			.function("value", &rant_t::value)
			.function("toEnum", &rant_t::toEnum)
		;
		emscripten::class_<halco::hicann::v2::HICANNOnWafer::x_type, emscripten::base<rant_t> >(
			"HICANNOnWafer_x_type")
			.constructor<>()
			.constructor<size_t>()
			;
	}

	{
		typedef halco::common::detail::RantWrapper<
			halco::common::YRanged<15, 0>, halco::common::Y::value_type, 15, 0>
			rant_t;
		emscripten::class_<rant_t>("_details_RantWrapper_y_type")
			.function("value", &rant_t::value)
			.function("toEnum", &rant_t::toEnum)
		;
		emscripten::class_<halco::hicann::v2::HICANNOnWafer::y_type, emscripten::base<rant_t> >(
			"HICANNOnWafer_y_type") // halco::common::YRanged<15, 0>
			.constructor<>()
			.constructor<size_t>()
		;
	}

	{
		typedef halco::common::detail::RantWrapper<
			halco::common::EnumRanged<384>, halco::common::Enum::value_type, 383, 0>
			rant_t;
		emscripten::class_<rant_t>("_details_RantWrapper_EnumRanged_type")
			.function("value", &rant_t::value)
			.function("toEnum", &rant_t::toEnum)
		;
		emscripten::class_<halco::hicann::v2::HICANNOnWafer::enum_type, emscripten::base<rant_t> >(
			"HICANNOnWafer_EnumRanged_type")
			.constructor<>()
			.constructor<size_t>()
		;
	}

	{
		typedef halco::common::detail::GridCoordinate<
			halco::hicann::v2::HICANNOnWafer, halco::common::XRanged<35, 0>,
			halco::common::YRanged<15, 0>, 384>
				grid_t;
		emscripten::class_<grid_t>("_details_GridCoordinate_HICANNOnWafer_type")
			.function("toEnum", &grid_t::toEnum)
			.function("x", &grid_t::x)
			.function("y", &grid_t::y)
		;
		emscripten::class_<halco::hicann::v2::HICANNOnWafer, emscripten::base<grid_t> >(
			"HICANNOnWafer")
			.constructor<halco::hicann::v2::HICANNOnWafer::enum_type const&>()
			.constructor<halco::hicann::v2::HICANNOnWafer::x_type const&,
			             halco::hicann::v2::HICANNOnWafer::y_type const&>()
			.function("east", &halco::hicann::v2::HICANNOnWafer::east)
			.function("west", &halco::hicann::v2::HICANNOnWafer::west)
			.function("north", &halco::hicann::v2::HICANNOnWafer::north)
			.function("south", &halco::hicann::v2::HICANNOnWafer::south)
			.function("move", &halco::hicann::v2::HICANNOnWafer::move)
			.function("toDNCOnWafer", &halco::hicann::v2::HICANNOnWafer::toDNCOnWafer)
			.function("toHICANNOnDNC", &halco::hicann::v2::HICANNOnWafer::toHICANNOnDNC)
		;
	}

	{
		typedef halco::common::detail::RantWrapper<
			halco::common::XRanged<1, 0>, halco::common::XRanged<1, 0>::value_type, 1, 0>
			rant_t;
		emscripten::class_<rant_t>("_details_RantWrapper_SideHorizontal_type")
			.function("value", &rant_t::value)
			.function("toEnum", &rant_t::toEnum)
		;
		emscripten::class_<halco::common::SideHorizontal, emscripten::base<rant_t> >("SideHorizontal")
			.constructor<>()
			.constructor<size_t>()
		;
		emscripten::constant("left", halco::common::left);
		emscripten::constant("right", halco::common::right);
	}

	{
		typedef halco::common::detail::RantWrapper<halco::common::Orientation, uint_fast16_t, 1, 0>
			rant_t;
		emscripten::class_<rant_t>("_details_RantWrapper_Orientation_type")
			.function("value", &rant_t::value)
			.function("toEnum", &rant_t::toEnum)
		;
		emscripten::class_<halco::common::Orientation, emscripten::base<rant_t> >("Orientation")
			.constructor<>()
			.constructor<uintmax_t>()
		;
		emscripten::constant("horizontal", halco::common::horizontal);
		emscripten::constant("vertical", halco::common::vertical);
	}

	{
		emscripten::class_<marocco::results::L1RouteProperties>("L1RouteProperties")
			/* not constructible */
			.function("projection_ids", &marocco::results::L1RouteProperties::projection_ids)
			.function("route", &marocco::results::L1RouteProperties::route)
			.function("l1_addresses", &marocco::results::L1RouteProperties::l1_addresses)
		;

		emscripten::register_vector<size_t>("VectorSize_t");
		emscripten::register_vector<marocco::results::L1RouteProperties>("VectorL1RouteProperties");

		emscripten::class_<marocco::L1Route>("L1Route")
			.function("size", &marocco::L1Route::size)
			.function("get", &emscripten::internal::VectorAccess<marocco::L1Route>::get)
			.function("source_hicann", &marocco::L1Route::source_hicann)
			.function("target_hicann", &marocco::L1Route::target_hicann)
		;

		emscripten::class_<marocco::L1Route::segment_type>("L1RouteSegment_type")
			.function("to_string", &L1Route_segment_type_to_name)
			.function("which", &marocco::L1Route::segment_type::which)
			.function("empty", &marocco::L1Route::segment_type::empty)
			.class_function("list", &L1Route_Segment_type_to_name_list)
		;

		emscripten::register_vector<std::string>("VectorString");

		// some opaque types (variants of marocco::L1Route::segment_type)
		emscripten::class_<HMF::Coordinate::Merger0OnHICANN>("Merger0OnHICANN");
		emscripten::class_<HMF::Coordinate::Merger1OnHICANN>("Merger1OnHICANN");
		emscripten::class_<HMF::Coordinate::Merger2OnHICANN>("Merger2OnHICANN");
		emscripten::class_<HMF::Coordinate::Merger3OnHICANN>("Merger3OnHICANN");
		emscripten::class_<HMF::Coordinate::GbitLinkOnHICANN>("GbitLinkOnHICANN");
		emscripten::class_<HMF::Coordinate::DNCMergerOnHICANN>("DNCMergerOnHICANN");
		emscripten::class_<HMF::Coordinate::RepeaterBlockOnHICANN>("RepeaterBlockOnHICANN");
		emscripten::class_<HMF::Coordinate::HLineOnHICANN>("HLineOnHICANN");
		emscripten::class_<HMF::Coordinate::VLineOnHICANN>("VLineOnHICANN");
		emscripten::class_<HMF::Coordinate::SynapseDriverOnHICANN>("SynapseDriverOnHICANN");
		emscripten::class_<HMF::Coordinate::SynapseOnHICANN>("SynapseOnHICANN");
	}
}
