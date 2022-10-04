# Todo
 * [ ] Tests: manager & table works fine individually, but breaks when run with the other tests. Probably some setting getting messed up somewhere (qmin, qmax?)
 * [ ] Hydrogens are still not handled correctly - if e.g. 2HB is listed, it is not automatically parsed as a hydrogen and subsequently crashes. 
 * [ ] MinimumExplorer: Fails if started in an area with no voxels
 * [ ] ImageStack: prepare_function() can only be bound to a single fitter for some reason - performing a second fit with the same function leads to a crash.
 * [ ] Grid: Binning range seems to be exclusive the top-most atom
 * [ ] EM: R factors http://pd.chem.ucl.ac.uk/pdnn/refine1/rfacs.htm
 * [ ] EM: Compare maps from different simulation methods
 * [ ] ResidueParser: Consider creating 1 parsed file per residue and then dynamically loading only those that are needed for the current file. 
 * [ ] DebyeLookupTable: Sometimes fails because it arbitrarily switches to non-default table lookups, but apparently only for A2M_ma. Create more tests and see if it can be reproduced. (Failed at Step 6: Evaluated cutoff value 0.220487 with chi2 468285
)
 * [ ] Protein: Figure out how getMass should work in without effective charge enabled
 * [ ] EM: Figure out why the test partial_histogram_manager::comparison with standard approach doesn't work with data/A2M/A2M_ma.ccp4. Probably something to do with assumed negative staining?
 * [ ] Slice: Change storage to be Matrix<T>* such that it is always kept up to date. Currently Slices are invalidated when Matrix data is changed (like with extend). 
 * [ ] EMFitter: Handle absolute scale properly.
 * [ ] gentag alt for de andre filer
 * [ ] flow diagram med hvad vi rent faktisk laver
 * [ ] General: Consistency check of DrhoM
 * [ ] EM: Research and implement Electron Transfer Function (if not too difficult)
 * [ ] IO: Support multiple terminate statements
 * [ ] Memory test all other executables.

# Stuff to consider
## EM
 * Consider the effects of discretization. 
 * e2pdb2mrc.py: 
 	eval "$(/home/au561871/tools/eman2/bin/conda shell.bash hook)"
	python3 ~/tools/eman2/bin/e2pdb2mrc.py
	
 * Fitter can be optimized. Current implementation spends too much time on the lower half of the chi2 curve, where there's no oscillations. 

## Compiler flags:
 * fno-finite-math-only: Can probably be removed, not sure of performance benefits. Note that its removal would specifically break limit handling of plots where std::isinf checks are used. 

## Grid:
 * Consider how to improve culling method
 * Consider removing all bounds checks (or maybe use a compile-flag to enable them)
 * The grid map stores chars = 1 byte or 8 bits, but we only use 5 different values (0, 'h', 'H', 'a', 'A'). If we can reduce this to 4 values, it can be stores in just 2 bits, in which case the remaining 6 bits can be used by other threads. That's literally free multithreading. Possible race condition. 

## ScatteringHistogram:
 * Optional argument of q-values to calculate I(q) for - this would remove the necessity of splicing in the IntensityFitter
 * Take a closer look at the form factor
 * Convert `a` to a more sensible output (units)

## Protein: 
 * When calculating the volume of the acids, the calculation is simply delegated to each individual body. However, if an amino acid happens to be cut in two halves in two different bodies, its volume is counted twice. 

# Dependencies
Maybe bundle them somehow to make it easier to install?
 * ROOT (compile options: `cmake -DCMAKE_INSTALL_PREFIX=<install> -Dminuit2=ON -DCMAKE_CXX_STANDARD=17 -Dbuiltin_gsl=ON <source>`)
 * Elements
 * CLI11
 * catch2 for tests

# FITTING:
FOXS SAXS fitting program
ATSAS CRYSOL

# Other personal notes
## Articles
 * Joachim (Jochen) Hub - molecular dynamics
 * Aquaporin DDM
 * B. Jacrot rep prog phys ~ 1980
 * Peter Zipper ~1980
 * ATSAS Crysol
