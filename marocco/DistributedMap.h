#pragma once

#include <unordered_map>
#include <stdexcept>
#include <boost/graph/use_mpi.hpp>
#include <boost/graph/distributed/adjacency_list.hpp>
#include <boost/graph/distributed/mpi_process_group.hpp>

#include "marocco/test.h"

namespace marocco {

template<typename DistributedGraph, typename T>
class DistributedMap
{
public:
	typedef T                value_type;
	typedef DistributedGraph graph_type;

private:
	//typedef boost::graph::distributed::mpi_process_group process_t;

	typedef typename DistributedGraph::vertex_descriptor local_descriptor;

	// could also be parametrized to be hash_map or so
	typedef std::unordered_map<
			local_descriptor,
			value_type
		> local_map;
	//typedef boost::associative_property_map<local_map> local_pmap;

	//typedef boost::detail::parallel::global_descriptor_property_map<
		//local_descriptor> global_key;

	//typedef boost::property_map<
			//local_descriptor, local_pmap
		//> distributed_pmap;

public:
	//typedef typename global_key::key_type key_type;
	typedef local_descriptor key_type;

	//DistributedMap(process_t const& comm = process_t());
	DistributedMap();
	DistributedMap(DistributedMap&&) = delete;

	/// Sets the consistency model of the distributed property map, which will
	/// take effect on the next synchronization step.
	void set_consistency_model(int model);

	/// Emits a message sending the contents of all local ghost cells to the
	/// owners of those cells.
	void flush();

	/// Replaces the values stored in each of the ghost cells with the default
	/// value generated by the reduction operation.
	void reset();

	/// Removes all ghost cells from the property map.
	void clear();

	/// Retrieves the element in pm associated with the given key. If the key
	/// refers to data stored locally, returns the actual value associated with
	/// the key. If the key refers to nonlocal data, returns the value of the
	/// ghost cell. If no ghost cell exists, the behavior depends on the current
	/// reduction operation: if a reduction operation has been set and has
	/// non_default_resolver set true, then a ghost cell will be created according
	/// to the default value provided by the reduction operation. Otherwise, the
	/// call to get will abort because no value exists for this remote cell. To
	/// avoid this problem, either set a reduction operation that generates
	/// default values, request() the value and then perform a synchronization
	/// step, or put a value into the cell before reading it.
	value_type get(key_type const& k) const;

	/// Places the given value associated with key into property map pm. If the
	/// key refers to data stored locally, the value is immediately updates. If
	/// the key refers to data stored in a remote process, updates (or creates) a
	/// local ghost cell containing this value for the key and sends the new value
	/// to the owning process. Note that the owning process may reject this value
	/// based on the reduction operation, but this will not be detected until the
	/// next synchronization step.
	void put(key_type const& k, value_type const& v);

	/// Equivalent to put(pm, key, value), except that no message is sent to the
	/// owning process when the value is changed for a nonlocal key.
	void local_put(key_type const& k, value_type const& v);

	/// Synchronize the values stored in the distributed property maps. Each
	/// process much execute synchronize at the same time, after which the ghost
	/// cells in every process will reflect the actual value stored in the owning
	/// process.
	void synchronize();

	/// Request that the element "key" be available after the next
	/// synchronization step. For a non-local key, this means establishing a ghost
	/// cell and requesting.
	void request(key_type const& k);

	/// returns the number of elements in local storage
	size_t size() const;

	bool contains(key_type const& k) const;

	/// access the mpi_process_group - which controls communication between the
	/// participants in the distributed property map.
	//process_t const& process() const;

	template<typename A, typename B>
	friend bool operator== (DistributedMap<A,B> const& a, DistributedMap<A,B> const& b);

	template<typename A, typename B>
	friend bool operator!= (DistributedMap<A,B> const& a, DistributedMap<A,B> const& b);

private:
	//process_t         mProcess;
	local_map         mMap;
	//local_pmap        mLocal;
	//distributed_pmap  mDistributed;

	FRIEND_TEST(DistributedMap, Basic);
};



//template<typename Graph, typename T>
//DistributedMap<Graph, T>::DistributedMap(process_t const& comm) :
	//mProcess(comm),
	//mMap(),
	//mLocal(mMap),
	//mDistributed(mProcess, global_key(), mLocal)
//{}

template<typename Graph, typename T>
DistributedMap<Graph, T>::DistributedMap()
{}

template<typename Graph, typename T>
void DistributedMap<Graph, T>::set_consistency_model(int const model)
{
	//mDistributed.set_consistency_model(model);
}

template<typename Graph, typename T>
void DistributedMap<Graph, T>::flush()
{
	//mDistributed.flush();
}

template<typename Graph, typename T>
void DistributedMap<Graph, T>::reset()
{
	//mDistributed.reset();
}

template<typename Graph, typename T>
void DistributedMap<Graph, T>::clear()
{
	//mDistributed.clear();
	mMap.clear();
}

template<typename Graph, typename T>
typename DistributedMap<Graph, T>::value_type
DistributedMap<Graph, T>::get(key_type const& k) const
{
	if (!contains(k))
		throw std::out_of_range("element not in distributed property map");
	//return boost::parallel::get(mDistributed, k);
	return mMap.at(k);
}

template<typename Graph, typename T>
void DistributedMap<Graph, T>::put(key_type const& k, value_type const& v)
{
	//boost::parallel::put(mDistributed, k, v);
	mMap[k] = v;
}

template<typename Graph, typename T>
void DistributedMap<Graph, T>::local_put(key_type const& k, value_type const& v)
{
	//boost::parallel::local_put(mDistributed, k, v);
	mMap[k] = v;
}

template<typename Graph, typename T>
void DistributedMap<Graph, T>::synchronize()
{
	//boost::parallel::synchronize(mDistributed);
}

template<typename Graph, typename T>
void DistributedMap<Graph, T>::request(key_type const& k)
{
	//return boost::parallel::request(mDistributed, k);
}

template<typename Graph, typename T>
size_t DistributedMap<Graph, T>::size() const
{
	return mMap.size();
}

template<typename Graph, typename T>
bool DistributedMap<Graph, T>::contains(key_type const& k) const
{
	return mMap.find(k) != mMap.end();
}

//template<typename Graph, typename T>
//typename DistributedMap<Graph, T>::process_t const&
//DistributedMap<Graph, T>::process() const
//{
	//return mProcess;
//}

template<typename Graph, typename T>
bool operator== (DistributedMap<Graph, T> const& a,
				 DistributedMap<Graph, T> const& b)
{
	return a.mMap == b.mMap;
}

template<typename Graph, typename T>
bool operator!= (DistributedMap<Graph, T> const& a,
				 DistributedMap<Graph, T> const& b)
{
	return a.mMap != b.mMap;
}

} // namespace marocco