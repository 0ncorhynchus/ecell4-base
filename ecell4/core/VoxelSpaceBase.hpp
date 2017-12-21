#ifndef ECELL4_VOXELSPACEBASE_HPP
#define ECELL4_VOXELSPACEBASE_HPP

#include <vector>
#include <set>
#include <map>
#include <stdexcept>

#include "Shape.hpp"
#include "Space.hpp"
#include "Integer3.hpp"
#include "get_mapper_mf.hpp"

#ifdef WITH_HDF5
#include "LatticeSpaceHDF5Writer.hpp"
#endif

#include "VoxelPool.hpp"
#include "Voxel.hpp"

namespace ecell4
{

#ifdef WIN32_MSC
double rint(const double x);
double round(const double x);
#endif

static inline std::string get_location_serial(const VoxelPool* vp)
{
    if (vp == NULL || vp->location() == NULL || vp->location()->is_vacant()) {
        return "";
    }

    return vp->location()->species().serial();
}

static inline std::string get_location_serial(const boost::shared_ptr<VoxelPool>& vp)
{
    if (vp == NULL || vp->location() == NULL || vp->location()->is_vacant()) {
        return "";
    }

    return vp->location()->species().serial();
}


class VoxelSpaceBase : public Space
{
public:

    typedef Voxel::coordinate_type coordinate_type;
    typedef VoxelPool::coordinate_id_pair_type coordinate_id_pair_type;

protected:

    typedef utils::get_mapper_mf<Species, boost::shared_ptr<VoxelPool> >::type
        voxel_pool_map_type;
    typedef utils::get_mapper_mf<Species, boost::shared_ptr<MoleculePool> >::type
        molecule_pool_map_type;

public:

    VoxelSpaceBase(const Real& voxel_radius);
    virtual ~VoxelSpaceBase();

    const Real t() const
    {
        return t_;
    }

    void set_t(const Real& t)
    {
        if (t < 0.0)
        {
            throw std::invalid_argument("the time must be positive.");
        }
        t_ = t;
    }

    virtual void save(const std::string& filename) const
    {
        throw NotSupported(
            "save(const std::string) is not supported by this space class");
    }

#ifdef WITH_HDF5
    virtual void save_hdf5(H5::Group* root) const
    {
        throw NotSupported(
            "save_hdf5(H5::Group* root) is not supported by this space class");
    }

    virtual void load_hdf5(const H5::Group& root)
    {
        throw NotSupported(
            "load_hdf5(const H5::Group& root) is not supported by this space class");
    }
#endif

    /**
     * static members
     */

    static inline Real calculate_voxel_volume(const Real r)
    {
        return 4.0 * sqrt(2.0) * r * r * r;
    }

    static inline Real3 calculate_hcp_lengths(const Real voxel_radius)
    {
        return Real3(
            voxel_radius / sqrt(3.0), // HCP_L
            voxel_radius * sqrt(8.0 / 3.0), // HCP_X
            voxel_radius * sqrt(3.0)); // HCP_Y
    }

    static inline Integer3 calculate_shape(const Real3& edge_lengths, const Real& voxel_radius, const bool is_periodic)
    {
        const Real3 hcpLXY = calculate_hcp_lengths(voxel_radius);
        const Real lengthX = edge_lengths[0];
        const Real lengthY = edge_lengths[1];
        const Real lengthZ = edge_lengths[2];

        Integer col_size = (Integer)rint(lengthX / hcpLXY[1]) + 1;
        Integer layer_size = (Integer)rint(lengthY / hcpLXY[2]) + 1;
        Integer row_size = (Integer)rint((lengthZ / 2) / voxel_radius) + 1;

        if (is_periodic)
        {
            // The number of voxels in each axis must be even for a periodic boundary.
            col_size = (col_size % 2 == 0 ? col_size : col_size + 1);
            layer_size = (layer_size % 2 == 0 ? layer_size : layer_size + 1);
            row_size = (row_size % 2 == 0 ? row_size : row_size + 1);
        }

        return Integer3(col_size, row_size, layer_size);
    }

    static inline Real calculate_volume(const Real3& edge_lengths, const Real& voxel_radius, const bool is_periodic)
    {
        const Integer3 shape = calculate_shape(edge_lengths, voxel_radius, is_periodic);
        return static_cast<Real>(shape[0] * shape[1] * shape[2]) * calculate_voxel_volume(voxel_radius);
    }

    Real voxel_radius() const
    {
        return voxel_radius_;
    }

    Real voxel_volume() const
    {
        return calculate_voxel_volume(voxel_radius_);
    }

    Real get_volume(const Species& sp) const
    {
        return voxel_volume() * num_voxels_exact(sp);
        // return inner_size() * voxel_volume();
    }

    Real actual_volume() const
    {
        return inner_size() * voxel_volume();
    }

    Real unit_area() const
    {
        const Real r(voxel_radius_);
        return 2.0 * sqrt(3.0) * r * r;
    }

    virtual Integer num_molecules(const Species& sp) const = 0;

    virtual Integer num_molecules_exact(const Species& sp) const
    {
        return num_voxels_exact(sp);
    }

    Integer num_particles() const
    {
        return num_voxels();
    }

    Integer num_particles(const Species& sp) const
    {
        return num_voxels(sp);
    }

    Integer num_particles_exact(const Species& sp) const
    {
        return num_voxels_exact(sp);
    }

    bool has_particle(const ParticleID& pid) const
    {
        return has_voxel(pid);
    }

    virtual bool remove_particle(const ParticleID& pid)
    {
        return remove_voxel(pid);
    }

    virtual std::pair<ParticleID, Particle> get_particle(const ParticleID& pid) const
    {
        const Voxel v(get_voxel(pid).second);
        return std::make_pair(pid, Particle(
            v.species(), coordinate2position(v.coordinate()), v.radius(), v.D()));
    }

    virtual std::vector<std::pair<ParticleID, Particle> > list_particles() const
    {
        const std::vector<std::pair<ParticleID, Voxel> > voxels(list_voxels());

        std::vector<std::pair<ParticleID, Particle> > retval;
        retval.reserve(voxels.size());
        for (std::vector<std::pair<ParticleID, Voxel> >::const_iterator
            i(voxels.begin()); i != voxels.end(); ++i)
        {
            const ParticleID& pid((*i).first);
            const Particle p(particle_at((*i).second.coordinate()));
            retval.push_back(std::make_pair(pid, p));
        }
        return retval;
    }

    virtual std::vector<std::pair<ParticleID, Particle> > list_particles(const Species& sp) const
    {
        const std::vector<std::pair<ParticleID, Voxel> > voxels(list_voxels(sp));

        std::vector<std::pair<ParticleID, Particle> > retval;
        retval.reserve(voxels.size());
        for (std::vector<std::pair<ParticleID, Voxel> >::const_iterator
            i(voxels.begin()); i != voxels.end(); ++i)
        {
            const ParticleID& pid((*i).first);
            const Particle p(particle_at((*i).second.coordinate()));
            retval.push_back(std::make_pair(pid, p));
        }
        return retval;
    }

    virtual std::vector<std::pair<ParticleID, Particle> > list_particles_exact(const Species& sp) const
    {
        const std::vector<std::pair<ParticleID, Voxel> >
            voxels(list_voxels_exact(sp));

        std::vector<std::pair<ParticleID, Particle> > retval;
        retval.reserve(voxels.size());
        for (std::vector<std::pair<ParticleID, Voxel> >::const_iterator
            i(voxels.begin()); i != voxels.end(); ++i)
        {
            const ParticleID& pid((*i).first);
            const Particle p(particle_at((*i).second.coordinate()));
            retval.push_back(std::make_pair(pid, p));
        }
        return retval;
    }

    virtual Integer size() const = 0;
    virtual Integer3 shape() const = 0;
    virtual Integer inner_size() const = 0;

    virtual bool on_structure(const Voxel& v) = 0;
    virtual bool make_structure_type(const Species& sp, Shape::dimension_kind dimension, const std::string loc)
    {
        throw NotImplemented("make_structure_type is not implemented.");
    }

    virtual bool make_interface_type(const Species& sp, Shape::dimension_kind dimension, const std::string loc)
    {
        throw NotImplemented("make_interface_type is not implemented.");
    }


    virtual std::vector<Species> list_species() const;

    virtual Integer num_voxels_exact(const Species& sp) const;
    virtual Integer num_voxels(const Species& sp) const;
    virtual Integer num_voxels() const;
    virtual bool has_voxel(const ParticleID& pid) const;

    virtual std::vector<std::pair<ParticleID, Voxel> > list_voxels() const;
    virtual std::vector<std::pair<ParticleID, Voxel> > list_voxels(const Species& sp) const;
    virtual std::vector<std::pair<ParticleID, Voxel> > list_voxels_exact(const Species& sp) const;

    virtual std::pair<ParticleID, Voxel> get_voxel(const ParticleID& pid) const;
    virtual VoxelPool* find_voxel_pool(const Species& sp);
    virtual const VoxelPool* find_voxel_pool(const Species& sp) const;
    virtual bool has_molecule_pool(const Species& sp) const;
    virtual MoleculePool* find_molecule_pool(const Species& sp);
    virtual const MoleculePool* find_molecule_pool(const Species& sp) const;

    virtual coordinate_type inner2coordinate(const coordinate_type inner) const = 0;

    virtual Real3 coordinate2position(const coordinate_type& coord) const = 0;
    virtual coordinate_type position2coordinate(const Real3& pos) const = 0;

    virtual Integer num_neighbors(const coordinate_type& coord) const = 0;
    virtual coordinate_type get_neighbor(
        const coordinate_type& coord, const Integer& nrand) const = 0;
    virtual coordinate_type get_neighbor_boundary(
        const coordinate_type& coord, const Integer& nrand) const = 0;

    /*
     * Virtual functions
     */

    virtual Real3 actual_lengths() const = 0;

    virtual std::pair<ParticleID, Voxel> get_voxel_at(const coordinate_type& coord) const = 0;
    virtual VoxelPool* get_voxel_pool_at(const coordinate_type& coord) const = 0;
    virtual const Particle particle_at(const coordinate_type& coord) const = 0;

    virtual bool update_voxel(const ParticleID& pid, const Voxel& v) = 0;
    virtual bool remove_voxel(const ParticleID& pid) = 0;
    virtual bool remove_voxel(const coordinate_type& coord) = 0;

    virtual bool can_move(const coordinate_type& src, const coordinate_type& dest) const = 0;
    virtual bool move(
        const coordinate_type& src, const coordinate_type& dest,
        const std::size_t candidate=0) = 0;
    virtual std::pair<coordinate_type, bool> move_to_neighbor(
        VoxelPool* const& from, VoxelPool* const& loc,
        coordinate_id_pair_type& info, const Integer nrand) = 0;

protected:

    virtual Integer count_voxels(const boost::shared_ptr<VoxelPool>& vp) const = 0;
    void push_voxels(std::vector<std::pair<ParticleID, Voxel> >& voxels,
            const boost::shared_ptr<MoleculePool>& voxel_pool,
            const Species& species) const;

protected:

    Real t_;
    Real voxel_radius_;

    voxel_pool_map_type voxel_pools_;
    molecule_pool_map_type molecule_pools_;

};

} // ecell4

#endif /* ECELL4_VOXELSPACEBASE_HPP */
