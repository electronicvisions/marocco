#include <emscripten/bind.h>

#include "Marocco.h"

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


EMSCRIPTEN_BINDINGS(marocco_results)
{
	emscripten::class_<marocco::results::Marocco>("Marocco")
		.constructor<>()
		.function("load", &marocco::results::Marocco::load)
		.function("save", &marocco::results::Marocco::save)
		.function("properties", &marocco::results::Marocco::properties)
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
}
