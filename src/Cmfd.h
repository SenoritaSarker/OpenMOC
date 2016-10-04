/**
 * @file Cmfd.h
 * @brief The Cmfd class.
 * @date October 14, 2013
 * @author Sam Shaner, MIT, Course 22 (shaner@mit.edu)
 */

#ifndef CMFD_H_
#define CMFD_H_

#ifdef __cplusplus
#define _USE_MATH_DEFINES
#ifdef SWIG
#include "Python.h"
#endif
#include "log.h"
#include "Universe.h"
#include "Track.h"
#include "Track3D.h"
#include "Quadrature.h"
#include "linalg.h"
#include "Geometry.h"
#include "Timer.h"
#endif

/** Forward declaration of Geometry class */
class Geometry;

/** Comparitor for sorting k-nearest stencil std::pair objects */
inline bool stencilCompare(const std::pair<int, FP_PRECISION>& firstElem,
                           const std::pair<int, FP_PRECISION>& secondElem) {
  return firstElem.second < secondElem.second;
}

#undef track_flux

/** Indexing macro for the angular fluxes for each polar angle and energy
 *  group for either the forward or reverse direction for a given Track */
#define track_flux(p,e) (track_flux[(p)*_num_moc_groups + (e)]

/**
 * @class Cmfd Cmfd.h "src/Cmfd.h"
 * @brief A class for Coarse Mesh Finite Difference (CMFD) acceleration.
 */
class Cmfd {

private:

  /** Pointer to polar quadrature object */
  Quadrature* _quadrature;

  /** Pointer to geometry object */
  Geometry* _geometry;

  /** The keff eigenvalue */
  FP_PRECISION _k_eff;

  /** The A (destruction) matrix */
  Matrix* _A;

  /** The M (production) matrix */
  Matrix* _M;

  /** The old source vector */
  Vector* _old_source;

  /** The new source vector */
  Vector* _new_source;

  /** Vector representing the flux for each cmfd cell and cmfd enegy group at
   * the end of a CMFD solve */
  Vector* _new_flux;

  /** Vector representing the flux for each cmfd cell and cmfd enegy group at
   * the beginning of a CMFD solve */
  Vector* _old_flux;

  /** Vector representing the ratio of the new to old CMFD flux */
  Vector* _flux_ratio;

  /** Gauss-Seidel SOR relaxation factor */
  FP_PRECISION _SOR_factor;

  /** cmfd source convergence threshold */
  FP_PRECISION _source_convergence_threshold;

  /** Number of cells in x direction */
  int _num_x;

  /** Number of cells in y direction */
  int _num_y;

  /** Number of cells in z direction */
  int _num_z;

  /** Number of energy groups */
  int _num_moc_groups;

  /** Number of polar angles */
  int _num_polar;

  /** Number of energy groups used in cmfd solver. Note that cmfd supports
   * energy condensation from the MOC */
  int _num_cmfd_groups;

  /** Coarse energy indices for fine energy groups */
  int* _group_indices;

  /** Map of MOC groups to CMFD groups */
  int* _group_indices_map;

  /** If the user specified fine-to-coarse group indices */
  bool _user_group_indices;

  /** Number of FSRs */
  int _num_FSRs;

  /** The volumes (areas) for each FSR */
  FP_PRECISION* _FSR_volumes;

  /** Pointers to Materials for each FSR */
  Material** _FSR_materials;

  /** The FSR scalar flux in each energy group */
  FP_PRECISION* _FSR_fluxes;

  /** The source region flux moments (x, y, and z) for each energy group */
  FP_PRECISION* _flux_moments;

  /** Array of CMFD cell volumes */
  Vector* _volumes;

  /** Array of material pointers for CMFD cell materials */
  Material** _materials;

  /** Physical dimensions of the geometry and each CMFD cell */
  FP_PRECISION _width_x;
  FP_PRECISION _width_y;
  FP_PRECISION _width_z;
  FP_PRECISION _cell_width_x;
  FP_PRECISION _cell_width_y;
  FP_PRECISION _cell_width_z;

  /** Array of geometry boundaries */
  boundaryType* _boundaries;

  /** Array of surface currents for each CMFD cell */
  Vector* _surface_currents;

  /** Vector of vectors of FSRs containing in each cell */
  std::vector< std::vector<int> > _cell_fsrs;

  /** Pointer to Lattice object representing the CMFD mesh */
  Lattice* _lattice;

  /** Flag indicating whether to update the MOC flux */
  bool _flux_update_on;

  /** Flag indicating whether to us centroid updating */
  bool _centroid_update_on;

  /** Number of cells to used in updating MOC flux */
  int _k_nearest;

  /** Map storing the k-nearest stencil for each fsr */
  std::map<int, std::vector< std::pair<int, FP_PRECISION> > >
    _k_nearest_stencils;

  /** OpenMP mutual exclusion locks for atomic CMFD cell operations */
  omp_lock_t* _cell_locks;

  /** Flag indicating whether the problem is 2D or 3D */
  bool _solve_3D;

  /** Array of azimuthal track spacings */
  FP_PRECISION* _azim_spacings;

  /** 2D array of polar track spacings */
  FP_PRECISION** _polar_spacings;

  //FIXME
  int _total_tally_size;
  FP_PRECISION* _tally_memory;
  FP_PRECISION** _nu_fission_tally;
  FP_PRECISION** _reaction_tally;
  FP_PRECISION** _volume_tally;
  FP_PRECISION** _total_tally;
  FP_PRECISION** _neutron_production_tally;
  FP_PRECISION** _diffusion_tally;
  FP_PRECISION*** _scattering_tally;
  FP_PRECISION*** _chi_tally;
  bool _tallies_allocated;
#ifdef MPIx
  FP_PRECISION* _tally_buffer;
  Vector* _currents_buffer;
#endif

  /** A timer to record timing data for a simulation */
  Timer* _timer;

  /* Private worker functions */
  FP_PRECISION computeLarsensEDCFactor(FP_PRECISION dif_coef,
                                       FP_PRECISION delta);
  void constructMatrices(int moc_iteration);
  void collapseXS();
  void updateMOCFlux();
  void rescaleFlux();
  void splitVertexCurrents();
  void splitEdgeCurrents();
  void getVertexSplitSurfaces(int cell, int vertex, std::vector<int>* surfaces);
  void getEdgeSplitSurfaces(int cell, int edge, std::vector<int>* surfaces);
  void initializeMaterials();
  void initializeCurrents();
  void generateKNearestStencils();

  /* Private getter functions */
  int getCellNext(int cell_id, int surface_id);
  int getCellByStencil(int cell_id, int stencil_id);
  FP_PRECISION getUpdateRatio(int cell_id, int moc_group, int fsr);
  FP_PRECISION getDistanceToCentroid(Point* centroid, int cell_id,
                                     int stencil_index);
  FP_PRECISION getSurfaceDiffusionCoefficient(int cmfd_cell, int surface,
                                              int group, int moc_iteration,
                                              bool correction);
  FP_PRECISION getDiffusionCoefficient(int cmfd_cell, int group);
  FP_PRECISION getSurfaceWidth(int surface);
  FP_PRECISION getPerpendicularSurfaceWidth(int surface);
  int getSense(int surface);


public:

  Cmfd();
  virtual ~Cmfd();

  /* Worker functions */
  FP_PRECISION computeKeff(int moc_iteration);
  void initialize();
  void initializeCellMap();
  void initializeGroupMap();
  void allocateTallies();
  void initializeLattice(Point* offset);
  int findCmfdCell(LocalCoords* coords);
  int findCmfdSurface(int cell_id, LocalCoords* coords);
  int findCmfdSurfaceOTF(int cell_id, double z, int surface_2D);
  void addFSRToCell(int cell_id, int fsr_id);
  void zeroCurrents();
  void tallyCurrent(segment* curr_segment, FP_PRECISION* track_flux,
                    int azim_index, int polar_index, bool fwd);
  void printTimerReport();
  void checkNeutronBalance();

  /* Get parameters */
  int getNumCmfdGroups();
  int getNumMOCGroups();
  int getNumCells();
  int getCmfdGroup(int group);
  int getBoundary(int side);
  Lattice* getLattice();
  int getNumX();
  int getNumY();
  int getNumZ();
  int convertFSRIdToCmfdCell(int fsr_id);
  int convertGlobalFSRIdToCmfdCell(long global_fsr_id);
  std::vector< std::vector<int> >* getCellFSRs();
  bool isFluxUpdateOn();
  bool isCentroidUpdateOn();

  /* Set parameters */
  void setSORRelaxationFactor(FP_PRECISION SOR_factor);
  void setGeometry(Geometry* geometry);
  void setWidthX(double width);
  void setWidthY(double width);
  void setWidthZ(double width);
  void setNumX(int num_x);
  void setNumY(int num_y);
  void setNumZ(int num_z);
  void setNumFSRs(int num_fsrs);
  void setNumMOCGroups(int num_moc_groups);
  void setBoundary(int side, boundaryType boundary);
  void setLatticeStructure(int num_x, int num_y, int num_z=1);
  void setFluxUpdateOn(bool flux_update_on);
  void setCentroidUpdateOn(bool centroid_update_on);
  void setGroupStructure(std::vector< std::vector<int> > group_indices);
  void setSourceConvergenceThreshold(FP_PRECISION source_thresh);
  void setQuadrature(Quadrature* quadrature);
  void setKNearest(int k_nearest);
  void setSolve3D(bool solve_3d);
  void setAzimSpacings(FP_PRECISION* azim_spacings, int num_azim);
  void setPolarSpacings(FP_PRECISION** polar_spacings, int num_azim,
                        int num_polar);

  /* Set FSR parameters */
  void setFSRMaterials(Material** FSR_materials);
  void setFSRVolumes(FP_PRECISION* FSR_volumes);
  void setFSRFluxes(FP_PRECISION* scalar_flux);
  void setCellFSRs(std::vector< std::vector<int> >* cell_fsrs);
  void setFluxMoments(FP_PRECISION* flux_moments);
};


/**
 * @brief Get the CMFD group given an MOC group.
 * @param group the MOC energy group
 * @return the CMFD energy group
 */
inline int Cmfd::getCmfdGroup(int group) {
  return _group_indices_map[group];
}


/*
 * @brief Quickly finds a 3D CMFD surface given a cell, global coordinate, and
 *        2D CMFD surface. Intended for use in axial on-the-fly ray tracing.
 * @details If the coords is not on a surface, -1 is returned. If there is
 *          no 2D CMFD surface intersection, -1 should be input for the 2D CMFD
 *          surface.
 * @param cell_id The CMFD cell ID that the local coords is in.
 * @param z the axial height in the root universe of the point being evaluated.
 * @param surface_2D The ID of the 2D CMFD surface that the LocalCoords object
 *        intersects. If there is no 2D intersection, -1 should be input.
 */
inline int Cmfd::findCmfdSurfaceOTF(int cell_id, double z, int surface_2D) {
  return _lattice->getLatticeSurfaceOTF(cell_id, z, surface_2D);
}


/**
 * @brief Tallies the current contribution from this segment across the
 *        the appropriate CMFD mesh cell surface.
 * @param curr_segment the current Track segment
 * @param track_flux the outgoing angular flux for this segment
 * @param polar_weights array of polar weights for some azimuthal angle
 * @param fwd boolean indicating direction of integration along segment
 */
inline void Cmfd::tallyCurrent(segment* curr_segment, FP_PRECISION* track_flux,
                               int azim_index, int polar_index, bool fwd) {

  int surf_id, cell_id, cmfd_group;
  int ncg = _num_cmfd_groups;
  FP_PRECISION currents[_num_cmfd_groups];
  memset(currents, 0.0, sizeof(FP_PRECISION) * _num_cmfd_groups);

  /* Check if the current needs to be tallied */
  bool tally_current = false;
  if (curr_segment->_cmfd_surface_fwd != -1 && fwd) {
    surf_id = curr_segment->_cmfd_surface_fwd % NUM_SURFACES;
    cell_id = curr_segment->_cmfd_surface_fwd / NUM_SURFACES;
    tally_current = true;
  }
  else if (curr_segment->_cmfd_surface_bwd != -1 && !fwd) {
    surf_id = curr_segment->_cmfd_surface_bwd % NUM_SURFACES;
    cell_id = curr_segment->_cmfd_surface_bwd / NUM_SURFACES;
    tally_current = true;
  }

  /* Tally current if necessary */
  if (tally_current) {

    if (_solve_3D) {
      FP_PRECISION wgt = _quadrature->getWeightInline(azim_index, polar_index);
      for (int e=0; e < _num_moc_groups; e++) {

        /* Get the CMFD group */
        cmfd_group = getCmfdGroup(e);

        /* Increment the surface group */
        currents[cmfd_group] += track_flux[e] * wgt;
      }

      /* Increment currents */
      _surface_currents->incrementValues
          (cell_id, surf_id*ncg, (surf_id+1)*ncg - 1, currents);
    }
    else {
      int pe = 0;
      for (int e=0; e < _num_moc_groups; e++) {

        /* Get the CMFD group */
        cmfd_group = getCmfdGroup(e);

        for (int p = 0; p < _num_polar/2; p++) {
          currents[cmfd_group] += track_flux[pe]
              * _quadrature->getWeightInline(azim_index, p);
          pe++;
        }
      }

      /* Increment currents */
      _surface_currents->incrementValues
          (cell_id, surf_id*ncg, (surf_id+1)*ncg - 1, currents);
    }
  }
}

#endif /* CMFD_H_ */
