# generate lists of files for easy use as dependencies
pymol := pymol

cmake_threads := 6

source := $(addprefix source/, $(shell find source/ -printf "%P "))
include := $(addprefix include/, $(shell find include/ -printf "%P "))

#################################################################################
###				EXECUTABLES					 ###
#################################################################################
.SECONDARY:

docs: build/Makefile
	@ make -C build doc
	firefox build/docs/html/index.html 

.PHONY:
gui: build/source/gui/gui
	build/gui

gwidth := 1
bwidth := 1
ra := 2.4
rh := 1.5
ps := Radial
hydrate/%: build/executable/new_hydration
	$< data/$*.pdb output/$*.pdb --grid_width ${gwidth} --radius_a ${ra} --radius_h ${rh} --placement_strategy ${ps}
	$(pymol) output/$*.pdb -d "hide all; show spheres, hetatm; color orange, hetatm"

view/%: 
	@ structure=$(shell find data/ -name "$*.pdb"); \
	$(pymol) $${structure}

hist/%: build/executable/hist
	@structure=$(shell find data/ -name "$*.pdb"); \
	$< $${structure} figures/ --grid_width ${gwidth} --radius_a ${ra} --radius_h ${rh} --bin_width ${bwidth} --placement_strategy ${ps}

order := ""
rotate/%: build/executable/rotate_map
	$< data/$* ${order}

main/%: build/executable/main
	$< $*

em/%: build/executable/em
	@ structure=$(shell find data/ -name "$*.pdb"); \
	measurement=$(shell find data/ -name "$*.RSR" -or -name "$*.dat"); \
	emmap=$(shell find data/ -name "$*.map" -or -name "$*.ccp4"); \
	$< $${emmap} $${structure} $${measurement}

optimize_radius/%: build/source/scripts/optimize_radius
	$< data/$*.pdb figures/

rigidbody/%: build/executable/rigidbody_opt
	$< data/$*.pdb data/$*.RSR figures/

qlow := 0
qhigh := 1000
center := center
options :=
intensity_fit/%: build/executable/intensity_fitter
	@ structure=$(shell find data/ -name "$*.pdb"); \
	measurement=$(shell find data/ -name "$*.RSR" -or -name "$*.dat"); \
	$< $${structure} $${measurement} -o figures/ --qlow ${qlow} --qhigh ${qhigh} --${center} --radius_a ${ra} --radius_h ${rh} --grid_width ${gwidth} --bin_width ${bwidth} --placement_strategy ${ps} ${options}

consistency/%: build/executable/consistency
	@ map=$(shell find data/ -name "$*.map" -or -name "$*.ccp4"); \
	$< $${map}

res := 20
# usage: make fit_consistency/2epe map=10
fit_consistency/%: build/executable/fit_consistency
	@ structure=$(shell find data/ -name "$*.pdb"); \
	measurement=$(shell find data/ -name "$*.RSR" -or -name "$*.dat"); \
	emmap=$(shell find sim/ -name "$*_${res}.ccp4" -or -name "$*_${res}.mrc"); \
	echo "./fit_consistency $${structure} $${measurement} $${emmap}\n"; \
	$< $${emmap} $${structure} $${measurement}

rebin/%: build/executable/rebin
	@ measurement=$(shell find data/ -name "$*.RSR" -or -name "$*.dat"); \
	$< $${measurement}

unit_cell/%: build/executable/unit_cell
	@ structure=$(shell find data/ -name "$*.pdb"); \
	$< $${structure}

#################################################################################
###			     SIMULATIONS					 ###
#################################################################################
simprog := ~/tools/EMAN/bin/pdb2mrc
simulate/%: 
	@ structure=$(shell find data/ -name "$*.pdb"); \
	$(simprog) $${structure} sim/$*_$(res).mrc res=$(res) het center

simfit/%: build/executable/fit_consistency
	@ structure=$(shell find data/ -name "$*.pdb"); \
	measurement=$(shell find data/ -name "$*.RSR" -or -name "$*.dat"); \
	$(simprog) $${structure} sim/$*_$(res).mrc res=$(res) het center; \
	echo "./fit_consistency $${structure} $${measurement} sim/$*_$(res).mrc\n"; \
	$< sim/$*_$(res).mrc $${structure} $${measurement}

old_simulate/%: 
	@ structure=$(shell find data/ -name "$*.pdb"); \
	phenix.fmodel $${structure} high_resolution=$(res);\
	phenix.mtz2map mtz_file=$(*F).pdb.mtz labels=FMODEL,PHIFMODEL output.prefix=$(*F) pdb_file=$${structure};\
	rm $(*F).pdb.mtz;\
	mv $(*F)_fmodel.ccp4 sim/$(*F)_$(res).ccp4;\

stuff/%: build/executable/stuff data/%.pdb
#	@$< data/$*.pdb sim/native_20.ccp4 sim/native_21.ccp4 sim/native_22.ccp4 sim/native_23.ccp4
	@$< data/$*.pdb $(shell find sim/ -name "$**" -printf "%p\n" | sort | awk '{printf("%s ", $$0)}')

#################################################################################
###				TESTS						 ###
#################################################################################
tags := ""
exclude_tags := "~[broken] ~[manual] ~[slow] ~[disable]"
memtest/%: $(shell find source/ -print) test/%.cpp	
	@ make -C build test -j${cmake_threads}
	valgrind --suppressions=suppressions.txt --track-origins=yes --log-file="valgrind.txt" build/test [$(*F)] ${tags}

test/%: $(shell find source/ -print) test/%.cpp
	@ make -C build test -j${cmake_threads}
	build/test [$(*F)] ~[slow] ${tags}

tests: $(shell find source/ -print) $(shell find test/ -print)
	@ make -C build test -j${cmake_threads}
	build/test $(exclude_tags) ~[memtest]

# special build target for our tests since they obviously depend on themselves, which is not included in $(source_files)
build/source/tests/%: $(shell find source/ -print) build/Makefile
	@ make -C build $* -j${cmake_threads}

#################################################################################
###				BUILD						 ###
#################################################################################
.PHONY: build
build: 
	@ mkdir -p build; 
	@ cd build; cmake ../

build/executable/%: $(source) $(include) executable/%.cpp
	@ cmake --build build/ --target $(*F) -j${cmake_threads} 

build/%: $(source) $(include)
	@ cmake --build build/ --target $(*F) -j${cmake_threads} 
	
build/Makefile: $(shell find -name "CMakeLists.txt" -printf "%P ")
	@ mkdir -p build
	@ cd build; cmake ../
	
clean/build: 
	@ rmdir -f build

