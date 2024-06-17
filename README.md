![title_light](../media/title_dark.png?raw=true#gh-light-mode-only)
![title_dark](../media/title_light.png?raw=true#gh-dark-mode-only)

# Main features
- **Simple foundation**: We have implemented the methods in the simplest possible way, making as few assumptions about your data as possible. With the Debye equation as the basis for the scattering profiles, the only loss of accuracy is through the histogram approximation, where we support using both weighted and unweighted bins depending on your preferences. By implementing the technique in modern C++ with efficiency in mind, we have managed to achieve some of the [best performance available](https://github.com/klytje/AUSAXS/blob/media/benchmark.png).
- **Fitting of high-resolution models to SAXS curves**: Fit atomic structure files using experimental SAXS data using an efficient implementation of the Debye equation. Various options are available regarding the handling of both the excluded volume and hydration shell. 
- **Validation of electron microscopy maps**: Validate EM maps using experimental SAXS data. By using the information contained within the EM map itself, dummy structures can be constructed and compared against the SAXS data, serving as a quick quality check on the conformation of the map. 
- **Rigidbody optimization**: _(Still under development)_ Perform self-consistent and customizable rigidbody optimizations, generating a new hydration shell for each step. Optional calibration with scattering curves predicted by molecular dynamics simulations can limit the number of free parameters to just 2, dramatically reducing the capability of overfitting.

User-guides to all of these programs can be found in the [wiki](https://github.com/klytje/AUSAXS/wiki).

# Installation
The fastest way to get started is using the most recent precompiled binaries available in the [releases](https://github.com/klytje/AUSAXS/releases).  

Alternatively you can follow [compilation guide](https://github.com/AUSAXS/AUSAXS/wiki/Compilation-&-installation) to compile it yourself in just a few simple steps. 

# References
Several articles documenting the methods used in this project are currently in various stages of development. The first, on the EM validation methods, is expected to be published soon. Direct links will be provided in this section once they are publically available. 

_This project is licenced under the GNU Lesser General Public Licence v3.0. Alternative licencing arrangements can be discussed upon request. Supported by grant 1026-00209B from the Independent Research Fund Denmark._
