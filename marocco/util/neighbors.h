#pragma once

#include <vector>
#include <type_traits>
#include <boost/iterator/permutation_iterator.hpp>

#include <nanoflann.hpp>

#include "halco/common/geometry.h"

namespace marocco {

/**
 * @brief Finds nearest neighbors for halbe grid coordinates.
 */
template <typename Coord, typename FloatingT = float>
class Neighbors
{
	struct kd_tree_adaptor_type;

public:
	typedef FloatingT value_type;
	typedef std::vector<Coord> points_type;
	typedef std::vector<size_t> indices_type;
	typedef std::vector<value_type> distances_type;
	typedef boost::permutation_iterator<typename points_type::const_iterator,
	                                    indices_type::const_iterator> iterator_type;

	/// @brief Forwarding constructor to initialize the set of all points.
	template <typename... F>
	explicit Neighbors(F&&... args) : points_(std::forward<F>(args)...)
	{
	}

	void find_near(Coord const& point, size_t num_results = 0)
	{
		find_near(point.x(), point.y(), num_results);
	}

	void find_near(value_type x, value_type y, size_t num_results = 0)
	{
		if (num_results == 0) {
			num_results = points_.size();
		} else if (num_results > points_.size()) {
			// nanoflann returns garbage for this case so we throw an error.
			throw std::runtime_error("not enough points");
		}

		indices_ = indices_type(num_results);
		squared_distances_ = distances_type(num_results);
		value_type point[2] = {x, y};
		kd_tree().knnSearch(point, num_results, indices_.data(), squared_distances_.data());
	}

	/// @brief Indices into #points() container in increasing distance from query point.
	indices_type const& indices() const
	{
		return indices_;
	}

	/// @brief Squared distances of neighboring points.
	distances_type const& squared_distances() const
	{
		return squared_distances_;
	}

	/// @brief Set of all points to be considered as neighbors.
	points_type const& points() const
	{
		return points_;
	}

	/// @brief Return distance from query point for a neighbor.
	value_type distance(iterator_type it) const
	{
		return std::sqrt(squared_distances_.at(std::distance(indices_.begin(), it.base())));
	}

	/// @brief Iterator to the beginning of all found neighbors.
	iterator_type begin() const
	{
		return boost::make_permutation_iterator(points_.begin(), indices_.begin());
	}

	/// @brief Iterator to the end of all found neighbors.
	iterator_type end() const
	{
		return boost::make_permutation_iterator(points_.begin(), indices_.end());
	}

	/// @brief Reserve storage in points container.
	/// @param new_cap New minimal capacity of container
	void reserve(size_t new_cap)
	{
		points_.reserve(new_cap);
	}

	/// @brief Add point to set of neighbors to consider.
	/// @note This will invalidate the index and lead to its rebuilding
	///       on the next call to #find_near().
	void push_back(Coord const& point)
	{
		dirty = true;
		points_.push_back(point);
	}

private:
	struct kd_tree_adaptor_type
	{
		points_type const& points;

		/// Return the number of data points.
		inline size_t kdtree_get_point_count() const
		{
			return points.size();
		}

		/// Returns the squared distance between the two-dimensional vector *p1 and
		/// the data point with index idx_p2.
		inline value_type
		kdtree_distance(value_type const* p1, size_t const idx_p2, size_t /*size*/) const
		{
			value_type const d0 = p1[0] - value_type(points[idx_p2].x());
			value_type const d1 = p1[1] - value_type(points[idx_p2].y());
			return d0 * d0 + d1 * d1;
		}

		/// Returns the dim'th component of the idx'th point.
		inline value_type kdtree_get_pt(size_t const idx, int const dim) const
		{
			if (dim == 0) {
				return value_type(points[idx].x());
			} else {
				return value_type(points[idx].y());
			}
		}

		template <class BBOX>
		bool kdtree_get_bbox(BBOX& /*bb*/) const
		{
			// Return false to default to a standard bounding box computation.
			return false;
		}
	}; // kd_tree_adaptor_type

	typedef nanoflann::KDTreeSingleIndexAdaptor<
	    nanoflann::L2_Simple_Adaptor<value_type, kd_tree_adaptor_type>,
	    kd_tree_adaptor_type,
	    2 /* dim */
	    > kd_tree_type;

	kd_tree_type& kd_tree()
	{
		if (dirty) {
			kd_tree_.buildIndex();
			dirty = false;
		}

		return kd_tree_;
	}

	bool dirty = true;
	points_type points_;
	kd_tree_adaptor_type kd_tree_adaptor_{points_};
	kd_tree_type kd_tree_{2 /*dim*/, kd_tree_adaptor_};
	indices_type indices_;
	distances_type squared_distances_;
}; // Neighbors

} // namespace marocco
