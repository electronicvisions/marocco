#include <emscripten/bind.h>

#include "Marocco.h"

EMSCRIPTEN_BINDINGS(marocco_results)
{
	emscripten::class_<marocco::results::Marocco>("Marocco")
		.constructor<>()
		.function("load", &marocco::results::Marocco::load)
		.function("properties", &marocco::results::Marocco::properties)
	;

	emscripten::class_<marocco::results::HICANNOnWaferProperties>("HICANNOnWaferProperties")
		//.function("is_available", &PropertiesHICANNOnWafer::is_available)
		//.function("is_transit_only", &PropertiesHICANNOnWafer::is_transit)
		//.function("has_neurons", &PropertiesHICANNOnWafer::has_neurons)
		//.function("has_input", &PropertiesHICANNOnWafer::has_input)
		// number of neurons, number of inputs, ...
	;

	class_<HICANNOnWafer>("HICANNOnWafer")
		//.constructor(&create_HICANNOnWafer, allow_raw_pointers())
		//.function("east", HICANNOnWafer::east)
		//.function("west", HICANNOnWafer::west)
		//.function("north", HICANNOnWafer::north)
		//.function("south", HICANNOnWafer::south)
		//.function("min", HICANNOnWafer::min)
		//.function("max", HICANNOnWafer::max)
		//.function("toEnum", HICANNOnWafer::toEnum)
	;

	// TODO: iterator-semantics via javascript code!
}
