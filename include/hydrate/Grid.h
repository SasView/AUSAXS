#pragma once

#include <list>
#include <vector>

#include <data/Atom.h>
#include <data/Water.h>
#include <hydrate/placement/PlacementStrategy.h>
#include <hydrate/culling/CullingStrategy.h>
#include <hydrate/GridMember.h>
#include <utility/Settings.h>
#include <utility/Axis3D.h>
#include <hydrate/GridObj.h>

// forwards declaration
class Body;

class Grid {
	public:
		/**
		 * @brief Constructor.
		 * 
		 * @param base the base point for the grid.
		 */
		Grid(const Axis3D& axes) : Grid(axes, setting::grid::ra, setting::grid::rh, setting::grid::placement_strategy, setting::grid::culling_strategy) {}

		/**
		 * @brief Constructor.
		 * 
		 * @param base the base point for the grid.
		 * @param bins the number of bins in all dimensions. 
		 * @param radius the radius of each atom.
		 */
		Grid(const Axis3D& axes, int radius) : Grid(axes, radius, radius, setting::grid::placement_strategy, setting::grid::culling_strategy) {}

		/**
		 * @brief Constructor.
		 * 
		 * @param base the base point for the grid.
		 * @param bins the number of bins in each dimension. 
		 * @param ra the radius of each atom.
		 * @param rh the radius of each water molecule.
		 */
		Grid(const Axis3D& axes, double ra, double rh, setting::grid::PlacementStrategy ps = setting::grid::placement_strategy, setting::grid::CullingStrategy cs = setting::grid::culling_strategy);

		/**
		 * @brief Space-saving constructor. 
		 * 
		 * Construct a new Grid with a volume only slightly larger than that spanned by the input vector. 
		 * 
		 * @param atoms The atoms to be stored in the Grid. 
		 */
		Grid(const std::vector<Atom>& atoms) : Grid(atoms, setting::grid::width, setting::grid::ra, setting::grid::rh, setting::grid::placement_strategy, setting::grid::culling_strategy) {}

		/**
		 * @brief Space-saving constructor. 
		 * 
		 * Construct a new Grid with a volume only slightly larger than that spanned by the input vector. 
		 * 
		 * @param atoms The atoms to be stored in the Grid. 
		 * @param width the distance between each point.
		 * @param bins the number of bins in each dimension. 
		 * @param ra the radius of each atom.
		 * @param rh the radius of each water molecule.
		 */
		Grid(const std::vector<Atom>& atoms, double width, double ra, double rh, setting::grid::PlacementStrategy ps = setting::grid::placement_strategy, setting::grid::CullingStrategy cs = setting::grid::culling_strategy);

		/**
		 * @brief Space-saving constructor. 
		 * 
		 * Construct a new Grid with a volume only slightly larger than that spanned by the input bodies. 
		 * 
		 * @param bodies The bodies to be stored in the Grid. 
		 */
		Grid(const std::vector<Body>& bodies) : Grid(bodies, setting::grid::width, setting::grid::ra, setting::grid::rh, setting::grid::placement_strategy, setting::grid::culling_strategy) {}

		/**
		 * @brief Space-saving constructor. 
		 * 
		 * Construct a new Grid with a volume only slightly larger than that spanned by the input bodies. 
		 * 
		 * @param bodies The bodies to be stored in the Grid. 
		 * @param width the distance between each point.
		 * @param bins the number of bins in each dimension. 
		 * @param ra the radius of each atom.
		 * @param rh the radius of each water molecule.
		 */
		Grid(const std::vector<Body>& bodies, double width, double ra, double rh, setting::grid::PlacementStrategy ps = setting::grid::placement_strategy, setting::grid::CullingStrategy cs = setting::grid::culling_strategy);

		/**
		 * @brief Copy constructor. 
		 */
		Grid(const Grid& grid);

		/**
		 * @brief Move constructor. 
		 */
		Grid(Grid&& grid) noexcept;

		/** 
		 * @brief Add a vector of atoms to the grid. 
		 * 		  All added atoms are automatically expanded.
		 * 		  Complexity: O(n) in the number of added atoms.
		 */
		template <typename T, typename = std::enable_if_t<std::is_base_of<Atom, T>::value>>
		std::vector<grid::GridMember<T>> add(const std::vector<T>& atoms) {
			std::vector<grid::GridMember<T>> v(atoms.size());
			size_t index = 0;
			for (const auto& a : atoms) {
				v[index++] = add(a);
			}
			return v;
		}

		/**
		 * @brief Add the contents of a body to the grid.
		 * 		  All added atoms are automatically expanded.
		 * 		  Complexity: O(n) in the number of added atoms.
		 */
		std::vector<grid::GridMember<Atom>> add(const Body* body);

		/** 
		 * @brief Add a single atom to the grid. 
		 *        Complexity: O(1). 
		 */
		grid::GridMember<Atom> add(const Atom& atom, bool expand = false);

		/** 
		 * @brief Add a single water to the grid. 
		 *        Complexity: O(1). 
		 */
		grid::GridMember<Water> add(const Water& atom, bool expand = false);

		/**
		 * @brief Remove the contents of a body from the grid.
		 * 		  All removed atoms are automatically deflated. 
		 * 		  Complexity: O(n) in the number of member atoms.
		 */
		void remove(const Body* body);

		/**
		 * @brief Remove atoms as specified by the @a to_remove vector. 
		 * 		  All removed atoms are automatically deflated.
		 * 		  Complexity: O(n) in the number of member atoms.
		 */
		void remove(std::vector<bool>& to_remove);

		/**
		 * @brief Remove a single atom from the grid.
		 *        Complexity: O(n) in the number of member atoms. 
		 */
		void remove(const Atom& atom);

		/**
		 * @brief Remove a single waters from the grid.
		 *        Complexity: O(n) in the number of member waters. 
		 */
		void remove(const Water& atom);

		/**
		 * @brief Remove multiple waters from the grid.
		 *        Complexity: O(n) in the number of removed waters. 
		 */
		void remove(const std::vector<Water>& atom);

		/**
		 * @brief Remove multiple protein atoms from the grid.
		 *        Complexity: O(n) in the number of removed atoms. 
		 */
		void remove(const std::vector<Atom>& atom);

		/**
		 * @brief Remove all waters from the grid.
		 *        Complexity: O(n) in the number of waters. 
		 */
		void clear_waters();

		/** 
		 * @brief Expand all member atoms and waters into actual spheres based on the radii ra and rh. 
		 * 		  Only expands atoms if they have not already been expanded. 
		 * 		  Complexity: O(n) in the number of unexpanded atoms.
		 */
		void expand_volume();

		/**
		 * @brief Expand all member atoms and waters into actual spheres based on the radii ra and rh.
		 * 		  This method will expand all atoms, regardless of whether they have already been expanded.
		 * 		  Complexity: O(n) in the number of atoms.
		 */
		void force_expand_volume(); 

		/** 
		 * @brief Deflate all member atoms and water molecules into actual spheres based on the radii ra and rh. 
		 * 		  Only deflates atoms if they have been expanded. 
		 * 		  Complexity: O(n) in the number of expanded atoms.
		 */
		void deflate_volume();

		/**
		 * @brief Count the number of atoms in each cluster, and get those with less than \a min atoms.
		 *        This is useful for removing "floating" atoms from e.g. EM map data.
		 * 
		 * @return A vector of booleans indicating whether the atom at the corresponding index is part of a cluster with less than \a min atoms.
		 */
	    std::vector<bool> remove_disconnected_atoms(unsigned int min);

		/**
		 * @brief Generate a new hydration layer for the grid.
		 * 		  The layer is generated by the selected strategy.
		 * 		  Complexity: Depends on the strategy. Worst case is O(n) in the number of bins. 
		 * 
		 * @return Pointers to the new water molecules. 
		 */
		std::vector<Water> hydrate();

		/**
		 * @brief Get the number of bins in each dimension.
		 */
		Vector3<int> get_bins() const;

		/**
		 * @brief Get a copy of all waters in the grid. 
		 * 		  Complexity: O(n) in the number of member waters.
		 */
		std::vector<Water> get_waters() const;

		/**
		 * @brief Get a copy of all atoms in the grid.
		 * 		  Complexity: O(n) in the number of member atoms.
		 */
		std::vector<Atom> get_atoms() const;

		/**
		 * @brief Get the total volume spanned by the atoms in this grid in Å^3.
		 * 		  This will trigger the expansion of all unexpanded atoms. 
		 *        Waters do not count towards this volume.
		 * 		  Complexity: O(n) in the number of unexpanded atoms.
		 */
		double get_volume();

		/**
		 * @brief Get the width of each bin.
		 * 		  Complexity: O(1).
		 */
		double get_width() const {return width;}

		/**
		 * @brief Get a copy of the axes of the grid.
		 * 		  Complexity: O(1).
		 */
		Axis3D get_axes() const {return axes;}

		/**
		 * @brief Set the atomic radius (does not affect waters). 
		 * 		  Note that this will not affect already expanded atoms.
		 * 		  Complexity: O(1).
		 * 
		 * @param radius The new radius in Ångström.
		 */
		void set_radius_atoms(double radius);

		/**
		 * @brief Set the water radius (does not affect atoms).
		 * 		  Note that this will not affect already expanded waters.
		 * 
		 * @param radius The new radius in Ångström.
		 */
		void set_radius_water(double radius);

		/**
		 * @brief Get the atomic radius.
		 * 		  Complexity: O(1).
		 */
		unsigned int get_radius_atoms() const {return ra;}

		/**
		 * @brief Get the water radius.
		 * 		  Complexity: O(1).
		 */
		unsigned int get_radius_water() const {return rh;}

		/**
		 * @brief Create the smallest possible box containing the center points of all member atoms.
		 * 		  Complexity: O(n) in the number of atoms.
		 * 
		 * @return Two vectors containing the minimum and maximum coordinates of the box. 
		 */
		std::pair<Vector3<int>, Vector3<int>> bounding_box_index() const;

		/**
		 * @brief Convert a vector of absolute coordinates (x, y, z) to a vector of bin locations.
		 * 		  Complexity: O(1).
		 */
		Vector3<int> to_bins(const Vector3<double>& v) const;

		/**
		 * @brief Convert a location in the grid (binx, biny, binz) to a vector of absolute coordinates (x, y, z).
		 * 		  Complexity: O(1).
		 */
		template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
		Vector3<double> to_xyz(const Vector3<T>& v) const {
			return to_xyz(v.x(), v.y(), v.z());
		}

		Vector3<double> to_xyz(int i, int j, int k) const {
			double x = axes.x.min + width*i;
			double y = axes.y.min + width*j;
			double z = axes.z.min + width*k;
			return {x, y, z};
		}

		/**
		 * @brief Set this Grid equal to another.
		 * 		  Complexity: O(n) in the number of bins.
		 */
		Grid& operator=(const Grid& rhs);

		/**
		 * @brief Set this Grid equal to another.
		 * 		  Complexity: O(n) in the number of bins.
		 */
		Grid& operator=(Grid&& rhs) noexcept;

		/**
		 * @brief Check if this Grid is identical to another.
		 * 		  Neither the grid contents or members are comparead. 
		 *        Primarily intended to be used with tests.
		 * 		  Complexity: O(1).
		 */
		bool operator==(const Grid& rhs) const;

		/**
		 * @brief Save this Grid as a PDB file.
		 * 		  Complexity: O(n) in the number of bins.
		 */
		void save(std::string path) const;

		/**
		 * @brief Convert all bins occupied by atoms to dummy atoms for use in excluded volume calculations.
		 */
		Body generate_excluded_volume() const;

		std::vector<Atom> get_surface_atoms() const;

		/**
		 * @brief Get the contents of a single bin.
		 */
		GridObj::DATATYPE index(unsigned int i, unsigned int j, unsigned int k) const;

		GridObj grid; // The actual grid.
		std::list<grid::GridMember<Atom>> a_members; // A list of all member atoms and where they are located.
		std::list<grid::GridMember<Water>> w_members; // A list of all member water molecules and where they are located. 

	protected: // only protected since they are important for testing
		unsigned int volume = 0; // The number of bins covered by the members, i.e. the actual volume in the unit (width)^3
		unsigned int ra = 0; // Radius of each atom represented as a number of bins
		unsigned int rh = 0; // Radius of each water molecule represented as a number of bins

		/** 
		 * @brief Expand a single member atom into an actual sphere.
		 * 		  Only expands atoms if they have not already been expanded. 
		 * 		  Complexity: O(1).
		 */
		void expand_volume(grid::GridMember<Atom>& atom);

		/** 
		 * @brief Expand a single member water molecule into an actual sphere.
		 * 		  Only expands molecules if they have not already been expanded.
		 * 		  Complexity: O(1).
		 */
		void expand_volume(grid::GridMember<Water>& atom);

		/** 
		 * @brief Deflate a single member atom into an actual sphere.
		 * 		  Only deflates atoms if they have been expanded.
		 * 		  Complexity: O(1).
		 */
		void deflate_volume(grid::GridMember<Atom>& atom);

		/** 
		 * @brief Deflate a single member atom into an actual sphere.
		 * 		  Only deflates atoms if they have been expanded.
		 * 		  Complexity: O(1).
		 */
		void deflate_volume(grid::GridMember<Water>& atom);

		/**
		 * @brief Create the smallest possible box containing all atoms.
		 * 		  Complexity: O(n) in the number of atoms.
		 * 
		 * @return Two vectors containing the minimum and maximum coordinates of the box. 
		 */
		static std::pair<Vector3<double>, Vector3<double>> bounding_box(const std::vector<Atom>& atoms);

		/**
		 * @brief Identify possible hydration binding locations for the structure. 
		 * 
		 * @return A list of possible (binx, biny, binz) locations.
		 */
		std::vector<grid::GridMember<Water>> find_free_locs();

	private:
		Axis3D axes;
		double width; // distance between each grid point
		std::unique_ptr<grid::PlacementStrategy> water_placer; // the strategy for placing water molecules
		std::unique_ptr<grid::CullingStrategy> water_culler; // the strategy for culling the placed water molecules

		/** 
		 * @brief Expand a single member atom into an actual sphere.
		 * 		  All empty bins within the radius of the atom will be set to either GridObj::A_AREA or GridObj::H_AREA, 
		 * 		  and the center will be set to either GridObj::A_CENTER or GridObj::H_CENTER.
		 * 		  The grid volume is updated accordingly. 
		 * 
		 * @param loc The bin location of the atom. 
		 * @param is_water If the atom is a water molecule. Used to determine which marker to use in the grid. 
		 */
		void expand_volume(const Vector3<int>& loc, bool is_water);

		/** 
		 * @brief Deflate a single member atom from a sphere to a point.
		 * 		  All bins within the radius of the atom will be set to GridObj::EMPTY, and the center
		 * 		  will be set to either GridObj::A_CENTER or GridObj::H_CENTER.
		 * 		  The grid volume is updated accordingly.
		 * 		  
		 * 		  Note that this may cause the grid to be in an inconsistent state without an addtional call to expand_volume, 
		 * 		  since this method does not consider that more than one atom may fill the same bin.
		 * 
		 * @param loc The bin location of the atom. 
		 * @param is_water If the atom is a water molecule. Used to determine which marker to use in the grid. 
		 */
		void deflate_volume(const Vector3<int>& loc, bool is_water);

		void setup(double width, double ra, double rh, setting::grid::PlacementStrategy ps, setting::grid::CullingStrategy cs);
};