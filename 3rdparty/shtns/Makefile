## path prefix for make install (installs to $(PREFIX)/lib and $(PREFIX)/include)
PREFIX=/usr/local
SED=/usr/bin/sed
LIBS=-lfftw3 -lm 
PYTHON=
shtns_ver := $(shell grep -m 1 -e "^* v" CHANGELOG.md | sed -e "s/^* v\([^ ]*\).*/-DSHTNS_VER=\\\\\"\1\\\\\"/")
git_id := $(shell git describe --tags --always --dirty=*)

# compiler command with options for the sht codelets
shtcc=gcc -O2 -I/usr/local/include -L/usr/local/lib -march=native -mtune=skylake -O3 -ffast-math -std=gnu99 -fopenmp-simd -fno-tree-loop-distribute-patterns -mno-vzeroupper -D_GNU_SOURCE
# compiler command with options for other source (initialization, ...)
cc=gcc -O2 -I/usr/local/include -L/usr/local/lib -march=native -mtune=skylake -ffast-math -std=gnu99 -D_GNU_SOURCE -DSHTNS_GIT=\"$(git_id)\" $(shtns_ver) 
fc=gfortran -O2 -I/usr/local/include -L/usr/local/lib 

hfiles = sht_private.h sht_config.h shtns.h shtns_simd.h
objs = sht_init.o sht_kernels_a.o sht_kernels_s.o sht_odd_nlat.o sht_fly.o
libname = libshtns.a

default : libshtns.a

install : install-lib

test : test_suite test_rot
	./test_suite
	./test_rot

libshtns.a : Makefile $(objs)
	ar rcs libshtns.a $(objs)
	@echo " "
	@cat COPYRIGHT
	@echo " "
	@echo "**** link with :  -lfftw3 -lm "

install-lib : $(libname)
	@mkdir -p $(PREFIX)/lib/
	@mkdir -p $(PREFIX)/include/
	cp $(libname) $(PREFIX)/lib/$(libname)
	cp shtns.h $(PREFIX)/include/
	cp shtns_cuda.h $(PREFIX)/include/
	cp shtns.f shtns.f03 $(PREFIX)/include/
	@echo " "
	@cat COPYRIGHT
	@echo " "
	@echo "**** link with : -L$(PREFIX)/lib  -lfftw3 -lm "

# codelets :
SHT/SH_to_spat.c : SHT/hyb_SH_to_spat.gen.c
	$(MAKE) SH_to_spat.c -C SHT SED=$(SED)
SHT/spat_to_SH.c : SHT/hyb_spat_to_SH.gen.c
	$(MAKE) spat_to_SH.c -C SHT SED=$(SED)
SHT/SH_to_spat_fly.c : SHT/fly_SH_to_spat.gen.c
	$(MAKE) SH_to_spat_fly.c -C SHT SFX=fly SED=$(SED)
SHT/spat_to_SH_fly.c : SHT/fly_spat_to_SH.gen.c
	$(MAKE) spat_to_SH_fly.c -C SHT SFX=fly SED=$(SED)
SHT/SH_to_spat_omp.c : SHT/omp_SH_to_spat.gen.c
	$(MAKE) SH_to_spat_omp.c -C SHT SFX=omp SED=$(SED)
SHT/spat_to_SH_omp.c : SHT/omp_spat_to_SH.gen.c
	$(MAKE) spat_to_SH_omp.c -C SHT SFX=omp SED=$(SED)
SHT/SH_to_spat_kernel.c : SHT/kernel_SH_to_spat.gen.c
	$(MAKE) SH_to_spat_kernel.c -C SHT SFX=kernel SED=$(SED)
SHT/spat_to_SH_kernel.c : SHT/kernel_spat_to_SH.gen.c
	$(MAKE) spat_to_SH_kernel.c -C SHT SFX=kernel SED=$(SED)

# objects :
sht_init.o : sht_init.c Makefile sht_legendre.c sht_func.c sht_com.c $(hfiles)
	$(cc) -c $< -o $@

sht_odd_nlat.o : sht_odd_nlat.c Makefile sht_private.h
	$(cc) -c $< -o $@
sht_fly.o : sht_fly.c Makefile $(hfiles) SHT/SH_to_spat_fly.c SHT/spat_to_SH_fly.c
	$(cc) -c $< -o $@
sht_omp.o : sht_omp.c Makefile $(hfiles) SHT/SH_to_spat_omp.c SHT/spat_to_SH_omp.c
	$(cc) -c $< -o $@
sht_kernels_s.o : sht_kernels_s.c Makefile $(hfiles) SHT/SH_to_spat_kernel.c
	$(shtcc) -c $< -o $@
sht_kernels_a.o : sht_kernels_a.c Makefile $(hfiles) SHT/spat_to_SH_kernel.c
	$(shtcc) -c $< -o $@
sht_gpu.o : sht_gpu.cu sht_gpu_kernels.cu Makefile $(hfiles)
	nvcc -c sht_gpu.cu -lineinfo -Xptxas=-v 

# programs & Examples
time_SHT : time_SHT.c $(libname) shtns.h
	$(cc) time_SHT.c -I. ./$(libname) $(LIBS) -o time_SHT

test_suite : examples/test_suite.c $(libname) shtns.h
	$(cc) examples/test_suite.c -I. ./$(libname) $(LIBS) -o test_suite
test_rot : examples/test_rot.c $(libname) shtns.h
	$(cc) examples/test_rot.c -I. ./$(libname) $(LIBS) -o test_rot

SHT_example : examples/SHT_example.c $(libname) shtns.h
	$(cc) examples/SHT_example.c -I. ./$(libname) $(LIBS) -o SHT_example

SHT_fort_ex : examples/SHT_example.f $(libname) shtns.f
	$(fc) -L$(PREFIX)/lib -I$(PREFIX)/include -I. -fdefault-real-8 examples/SHT_example.f ./$(libname) $(LIBS) -lc -o SHT_fort_ex
SHT_f90_ex : examples/SHT_example.f90 $(libname) shtns.f03
	$(fc) -L$(PREFIX)/lib -I$(PREFIX)/include -I. -fdefault-real-8 examples/SHT_example.f90 ./$(libname) $(LIBS) -lc -o SHT_f90_ex

wigner : examples/wigner.c $(libname) shtns.h
	$(cc) examples/wigner.c -I. ./$(libname) $(LIBS) -o wigner

#documentation :
docs :
	( cat doc/doxygen.conf ; grep -m 1 -e "^* v" CHANGELOG.md | sed -e "s/^* v\([^ ]*\).*/PROJECT_NUMBER=\1/" ) | doxygen -

clean :
	$(MAKE) clean -C SHT
	rm -f $(objs) $(libname)
	rm -rf doc/html
	rm -rf doc/latex
	rm -rf build


# build a python interface using SWIG.
# use it with "from shtns import *" in a python program/shell
_shtns.so : shtns_numpy_wrap.c Makefile $(hfiles) $(objs)
	CC="$(CC)" $(PYTHON) setup.py build
	@echo "*****************************************************************************************"
	@echo "*** Now, for system wide install, run 'make install' as root.                           *"
	@echo "*** Otherwise run '$(PYTHON) setup.py install --user' to install for current user only. *"
	@echo "*****************************************************************************************"

# generate python and c glue code with SWIG.
shtns_numpy_wrap.c : shtns_numpy.i sht_private.h shtns.h
	swig -python shtns_numpy.i

install-py : _shtns.so
	$(PYTHON) setup.py install

.PHONY : install install-py install-lib clean docs


#fftw compiling options :
#-O3 -fomit-frame-pointer -fstrict-aliasing -ffast-math -fno-schedule-insns -fno-web -fno-loop-optimize --param inline-unit-growth=1000 --param large-function-growth=1000

