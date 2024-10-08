# This file was automatically generated by SWIG (http://www.swig.org).
# Version 4.0.2
#
# Do not make changes to this file unless you know what you are doing--modify
# the SWIG interface file instead.

"""Python/NumPy interface to the SHTns spherical harmonic transform library"""

from sys import version_info as _swig_python_version_info
if _swig_python_version_info < (2, 7, 0):
    raise RuntimeError("Python 2.7 or later required")

# Import the low-level C/C++ module
if __package__ or "." in __name__:
    from . import _shtns
else:
    import _shtns

try:
    import builtins as __builtin__
except ImportError:
    import __builtin__

def _swig_repr(self):
    try:
        strthis = "proxy of " + self.this.__repr__()
    except __builtin__.Exception:
        strthis = ""
    return "<%s.%s; %s >" % (self.__class__.__module__, self.__class__.__name__, strthis,)


def _swig_setattr_nondynamic_instance_variable(set):
    def set_instance_attr(self, name, value):
        if name == "thisown":
            self.this.own(value)
        elif name == "this":
            set(self, name, value)
        elif hasattr(self, name) and isinstance(getattr(type(self), name), property):
            set(self, name, value)
        else:
            raise AttributeError("You cannot add instance attributes to %s" % self)
    return set_instance_attr


def _swig_setattr_nondynamic_class_variable(set):
    def set_class_attr(cls, name, value):
        if hasattr(cls, name) and not isinstance(getattr(cls, name), property):
            set(cls, name, value)
        else:
            raise AttributeError("You cannot add class attributes to %s" % cls)
    return set_class_attr


def _swig_add_metaclass(metaclass):
    """Class decorator for adding a metaclass to a SWIG wrapped class - a slimmed down version of six.add_metaclass"""
    def wrapper(cls):
        return metaclass(cls.__name__, cls.__bases__, cls.__dict__.copy())
    return wrapper


class _SwigNonDynamicMeta(type):
    """Meta class to enforce nondynamic attributes (no new attributes) for a class"""
    __setattr__ = _swig_setattr_nondynamic_class_variable(type.__setattr__)



import numpy as np

SHTNS_INTERFACE = _shtns.SHTNS_INTERFACE

sht_orthonormal = _shtns.sht_orthonormal

sht_fourpi = _shtns.sht_fourpi

sht_schmidt = _shtns.sht_schmidt

sht_for_rotations = _shtns.sht_for_rotations

SHT_NO_CS_PHASE = _shtns.SHT_NO_CS_PHASE

SHT_REAL_NORM = _shtns.SHT_REAL_NORM

sht_gauss = _shtns.sht_gauss

sht_auto = _shtns.sht_auto

sht_reg_fast = _shtns.sht_reg_fast

sht_reg_dct = _shtns.sht_reg_dct

sht_quick_init = _shtns.sht_quick_init

sht_reg_poles = _shtns.sht_reg_poles

sht_gauss_fly = _shtns.sht_gauss_fly

SHT_THETA_CONTIGUOUS = _shtns.SHT_THETA_CONTIGUOUS

SHT_PHI_CONTIGUOUS = _shtns.SHT_PHI_CONTIGUOUS

SHT_SOUTH_POLE_FIRST = _shtns.SHT_SOUTH_POLE_FIRST

SHT_SCALAR_ONLY = _shtns.SHT_SCALAR_ONLY

SHT_LOAD_SAVE_CFG = _shtns.SHT_LOAD_SAVE_CFG

SHT_ALLOW_GPU = _shtns.SHT_ALLOW_GPU

SHT_ALLOW_PADDING = _shtns.SHT_ALLOW_PADDING

class sht(object):
    r"""Proxy of C shtns_info struct."""

    thisown = property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc="The membership flag")
    __repr__ = _swig_repr
    nlm = property(_shtns.sht_nlm_get, doc=r"""nlm : q(const).unsigned int""")
    lmax = property(_shtns.sht_lmax_get, doc=r"""lmax : q(const).unsigned short""")
    mmax = property(_shtns.sht_mmax_get, doc=r"""mmax : q(const).unsigned short""")
    mres = property(_shtns.sht_mres_get, doc=r"""mres : q(const).unsigned short""")
    nlat = property(_shtns.sht_nlat_get, doc=r"""nlat : q(const).unsigned int""")
    nphi = property(_shtns.sht_nphi_get, doc=r"""nphi : q(const).unsigned int""")
    nspat = property(_shtns.sht_nspat_get, doc=r"""nspat : q(const).unsigned int""")
    nlat_padded = property(_shtns.sht_nlat_padded_get, doc=r"""nlat_padded : q(const).unsigned int""")
    nlm_cplx = property(_shtns.sht_nlm_cplx_get, doc=r"""nlm_cplx : q(const).unsigned int""")

    def __init__(self, lmax, mmax=-1, mres=1, norm=sht_orthonormal, nthreads=0):
        r"""__init__(sht self, int lmax, int mmax=-1, int mres=1, int norm=sht_orthonormal, int nthreads=0) -> sht"""
        _shtns.sht_swiginit(self, _shtns.new_sht(lmax, mmax, mres, norm, nthreads))

        		## array giving the degree of spherical harmonic coefficients.
        self.l = np.zeros(self.nlm, dtype=np.int32)
        ## array giving the order of spherical harmonic coefficients.
        self.m = np.zeros(self.nlm, dtype=np.int32)
        for mloop in range(0, self.mmax*self.mres+1, self.mres):
        	for lloop in range(mloop, self.lmax+1):
        		ii = self.idx(lloop,mloop)
        		self.m[ii] = mloop
        		self.l[ii] = lloop
        self.m.flags.writeable = False		# prevent writing in m and l arrays
        self.l.flags.writeable = False



    __swig_destroy__ = _shtns.delete_sht

    def set_grid(self, nlat=0, nphi=0, flags=sht_quick_init, polar_opt=1.0e-10, nl_order=1):
        r"""set_grid(sht self, int nlat=0, int nphi=0, int flags=sht_quick_init, double polar_opt=1.0e-10, int nl_order=1)"""
        val = _shtns.sht_set_grid(self, nlat, nphi, flags, polar_opt, nl_order)

        		## array giving the cosine of the colatitude for the grid.
        self.cos_theta = self.__ct()
        self.cos_theta.flags.writeable = False
        ## shape of a spatial array for the grid (tuple of 2 values).
        self.spat_shape = tuple(self.__spat_shape())
        if self.nphi == 1:		# override spatial shape when nphi==1
        	self.spat_shape = (self.nlat, 1)
        	if flags & SHT_THETA_CONTIGUOUS: self.spat_shape = (1, self.nlat)


        return val


    def print_info(self):
        r"""print_info(sht self)"""
        return _shtns.sht_print_info(self)

    def sh00_1(self):
        r"""sh00_1(sht self) -> double"""
        return _shtns.sht_sh00_1(self)

    def sh10_ct(self):
        r"""sh10_ct(sht self) -> double"""
        return _shtns.sht_sh10_ct(self)

    def sh11_st(self):
        r"""sh11_st(sht self) -> double"""
        return _shtns.sht_sh11_st(self)

    def shlm_e1(self, l, m):
        r"""shlm_e1(sht self, unsigned int l, unsigned int m) -> double"""
        return _shtns.sht_shlm_e1(self, l, m)

    def robert_form(self, v=1):
        r"""for v>0, use Robert form, ie spatial VECTOR fields are multiplied by sin(colatitude). The scalar transforms are unaffected."""
        return _shtns.sht_robert_form(self, v)

    def __ct(self):
        r"""__ct(sht self) -> PyObject *"""
        return _shtns.sht___ct(self)

    def gauss_wts(self):
        r"""gauss_wts(sht self) -> PyObject *"""
        return _shtns.sht_gauss_wts(self)

    def mul_ct_matrix(self):
        r"""mul_ct_matrix(sht self) -> PyObject *"""
        return _shtns.sht_mul_ct_matrix(self)

    def st_dt_matrix(self):
        r"""st_dt_matrix(sht self) -> PyObject *"""
        return _shtns.sht_st_dt_matrix(self)

    def __spat_shape(self):
        r"""__spat_shape(sht self)"""
        return _shtns.sht___spat_shape(self)

    def spec_array(self, im=-1):
    	"""return a numpy array of spherical harmonic coefficients (complex). Adress coefficients with index sh.idx(l,m)
    	   if optional argument im is given, the spectral array is restricted to order im*mres."""
    	if im<0:
    		return np.zeros(self.nlm, dtype=complex)
    	else:
    		return np.zeros(self.lmax + 1 - im*self.mres, dtype=complex)

    def spat_array(self):
    	"""return a numpy array of 2D spatial field."""
    	if self.nlat == 0: raise RuntimeError("Grid not set. Call .set_grid() mehtod.")
    	return np.zeros(self.spat_shape)


    def idx(self, l, m):
        r"""idx(sht self, unsigned int l, unsigned int m) -> int"""
        return _shtns.sht_idx(self, l, m)

    def spat_to_SH(self, Vr, Qlm):
        r"""spat_to_SH(sht self, PyObject * Vr, PyObject * Qlm)"""
        return _shtns.sht_spat_to_SH(self, Vr, Qlm)

    def SH_to_spat(self, Qlm, Vr):
        r"""SH_to_spat(sht self, PyObject * Qlm, PyObject * Vr)"""
        return _shtns.sht_SH_to_spat(self, Qlm, Vr)

    def spat_cplx_to_SH(self, z, alm):
        r"""spat_cplx_to_SH(sht self, PyObject * z, PyObject * alm)"""
        return _shtns.sht_spat_cplx_to_SH(self, z, alm)

    def SH_to_spat_cplx(self, alm, z):
        r"""SH_to_spat_cplx(sht self, PyObject * alm, PyObject * z)"""
        return _shtns.sht_SH_to_spat_cplx(self, alm, z)

    def spat_to_SHsphtor(self, Vt, Vp, Slm, Tlm):
        r"""spat_to_SHsphtor(sht self, PyObject * Vt, PyObject * Vp, PyObject * Slm, PyObject * Tlm)"""
        return _shtns.sht_spat_to_SHsphtor(self, Vt, Vp, Slm, Tlm)

    def SHsphtor_to_spat(self, Slm, Tlm, Vt, Vp):
        r"""SHsphtor_to_spat(sht self, PyObject * Slm, PyObject * Tlm, PyObject * Vt, PyObject * Vp)"""
        return _shtns.sht_SHsphtor_to_spat(self, Slm, Tlm, Vt, Vp)

    def SHsph_to_spat(self, Slm, Vt, Vp):
        r"""SHsph_to_spat(sht self, PyObject * Slm, PyObject * Vt, PyObject * Vp)"""
        return _shtns.sht_SHsph_to_spat(self, Slm, Vt, Vp)

    def SHtor_to_spat(self, Tlm, Vt, Vp):
        r"""SHtor_to_spat(sht self, PyObject * Tlm, PyObject * Vt, PyObject * Vp)"""
        return _shtns.sht_SHtor_to_spat(self, Tlm, Vt, Vp)

    def spat_cplx_to_SHsphtor(self, Vt, Vp, Slm, Tlm):
        r"""spat_cplx_to_SHsphtor(sht self, PyObject * Vt, PyObject * Vp, PyObject * Slm, PyObject * Tlm)"""
        return _shtns.sht_spat_cplx_to_SHsphtor(self, Vt, Vp, Slm, Tlm)

    def SHsphtor_to_spat_cplx(self, Slm, Tlm, Vt, Vp):
        r"""SHsphtor_to_spat_cplx(sht self, PyObject * Slm, PyObject * Tlm, PyObject * Vt, PyObject * Vp)"""
        return _shtns.sht_SHsphtor_to_spat_cplx(self, Slm, Tlm, Vt, Vp)

    def spat_to_SHqst(self, Vr, Vt, Vp, Qlm, Slm, Tlm):
        r"""spat_to_SHqst(sht self, PyObject * Vr, PyObject * Vt, PyObject * Vp, PyObject * Qlm, PyObject * Slm, PyObject * Tlm)"""
        return _shtns.sht_spat_to_SHqst(self, Vr, Vt, Vp, Qlm, Slm, Tlm)

    def SHqst_to_spat(self, Qlm, Slm, Tlm, Vr, Vt, Vp):
        r"""SHqst_to_spat(sht self, PyObject * Qlm, PyObject * Slm, PyObject * Tlm, PyObject * Vr, PyObject * Vt, PyObject * Vp)"""
        return _shtns.sht_SHqst_to_spat(self, Qlm, Slm, Tlm, Vr, Vt, Vp)

    def spat_cplx_to_SHqst(self, Vr, Vt, Vp, Qlm, Slm, Tlm):
        r"""spat_cplx_to_SHqst(sht self, PyObject * Vr, PyObject * Vt, PyObject * Vp, PyObject * Qlm, PyObject * Slm, PyObject * Tlm)"""
        return _shtns.sht_spat_cplx_to_SHqst(self, Vr, Vt, Vp, Qlm, Slm, Tlm)

    def SHqst_to_spat_cplx(self, Qlm, Slm, Tlm, Vr, Vt, Vp):
        r"""SHqst_to_spat_cplx(sht self, PyObject * Qlm, PyObject * Slm, PyObject * Tlm, PyObject * Vr, PyObject * Vt, PyObject * Vp)"""
        return _shtns.sht_SHqst_to_spat_cplx(self, Qlm, Slm, Tlm, Vr, Vt, Vp)

    def synth(self,*arg):
    	"""
    	spectral to spatial transform, for scalar or vector data.
    	v = synth(qlm) : compute the spatial representation of the scalar qlm
    	vtheta,vphi = synth(slm,tlm) : compute the 2D spatial vector from its spectral spheroidal/toroidal scalars (slm,tlm)
    	vr,vtheta,vphi = synth(qlm,slm,tlm) : compute the 3D spatial vector from its spectral radial/spheroidal/toroidal scalars (qlm,slm,tlm)
    	"""
    	if self.nlat == 0: raise RuntimeError("Grid not set. Call .set_grid() mehtod.")
    	n = len(arg)
    	if (n>3) or (n<1): raise RuntimeError("1,2 or 3 arguments required.")
    	q = list(arg)
    	for i in range(0,n):
    		if q[i].size != self.nlm: raise RuntimeError("spectral array has wrong size.")
    		if q[i].dtype.num != np.dtype('complex128').num: raise RuntimeError("spectral array should be dtype=complex.")
    		if q[i].flags.contiguous == False: q[i] = q[i].copy()		# contiguous array required.
    	if n==1:	#scalar transform
    		vr = np.empty(self.spat_shape)
    		self.SH_to_spat(q[0],vr)
    		return vr
    	elif n==2:	# 2D vector transform
    		vt = np.empty(self.spat_shape)		# v_theta
    		vp = np.empty(self.spat_shape)		# v_phi
    		self.SHsphtor_to_spat(q[0],q[1],vt,vp)
    		return vt,vp
    	else:		# 3D vector transform
    		vr = np.empty(self.spat_shape)		# v_r
    		vt = np.empty(self.spat_shape)		# v_theta
    		vp = np.empty(self.spat_shape)		# v_phi
    		self.SHqst_to_spat(q[0],q[1],q[2],vr,vt,vp)
    		return vr,vt,vp

    def analys(self,*arg):
    	"""
    	spatial to spectral transform, for scalar or vector data.
    	qlm = analys(q) : compute the spherical harmonic representation of the scalar q
    	slm,tlm = analys(vtheta,vphi) : compute the spectral spheroidal/toroidal scalars (slm,tlm) from 2D vector components (vtheta, vphi)
    	qlm,slm,tlm = synth(vr,vtheta,vphi) : compute the spectral radial/spheroidal/toroidal scalars (qlm,slm,tlm) from 3D vector components (vr,vtheta,vphi)
    	"""
    	if self.nlat == 0: raise RuntimeError("Grid not set. Call .set_grid() mehtod.")
    	if abs(self.cos_theta[0]) == 1: raise RuntimeError("Analysis not allowed with sht_reg_poles grid.")
    	n = len(arg)
    	if (n>3) or (n<1): raise RuntimeError("1,2 or 3 arguments required.")
    	v = list(arg)
    	for i in range(0,n):
    		if v[i].shape != self.spat_shape: raise RuntimeError("spatial array has wrong shape.")
    		if v[i].dtype.num != np.dtype('float64').num: raise RuntimeError("spatial array should be dtype=float64.")
    		if v[i].flags.contiguous == False: v[i] = v[i].copy()		# contiguous array required.
    	if n==1:
    		q = np.empty(self.nlm, dtype=complex)
    		self.spat_to_SH(v[0],q)
    		return q
    	elif n==2:
    		s = np.empty(self.nlm, dtype=complex)
    		t = np.empty(self.nlm, dtype=complex)
    		self.spat_to_SHsphtor(v[0],v[1],s,t)
    		return s,t
    	else:
    		q = np.empty(self.nlm, dtype=complex)
    		s = np.empty(self.nlm, dtype=complex)
    		t = np.empty(self.nlm, dtype=complex)
    		self.spat_to_SHqst(v[0],v[1],v[2],q,s,t)
    		return q,s,t

    def synth_grad(self,slm):
    	"""(vtheta,vphi) = synth_grad(sht self, slm) : compute the spatial representation of the gradient of slm"""
    	if self.nlat == 0: raise RuntimeError("Grid not set. Call .set_grid() mehtod.")
    	if slm.size != self.nlm: raise RuntimeError("spectral array has wrong size.")
    	if slm.dtype.num != np.dtype('complex128').num: raise RuntimeError("spectral array should be dtype=complex.")
    	if slm.flags.contiguous == False: slm = slm.copy()		# contiguous array required.
    	vt = np.empty(self.spat_shape)
    	vp = np.empty(self.spat_shape)
    	self.SHsph_to_spat(slm,vt,vp)
    	return vt,vp

    def synth_cplx(self,*arg):
    	"""
    	spectral to spatial transform, for complex-valued scalar or vector data.
    	z = synth(zlm) : compute the complex-valued spatial representation of the scalar zlm
    	vtheta,vphi = synth(slm,tlm) : compute the complex-valued 2D spatial vector from its spectral spheroidal/toroidal scalars (slm,tlm)
    	vr,vtheta,vphi = synth(qlm,slm,tlm) : compute the complex-valued 3D spatial vector from its spectral radial/spheroidal/toroidal scalars (qlm,slm,tlm)
    	"""
    	if self.nlat == 0: raise RuntimeError("Grid not set. Call .set_grid() mehtod.")
    	if self.lmax != self.mmax: raise RuntimeError("complex SH requires lmax=mmax and mres=1.")

    	n = len(arg)
    	if (n>3) or (n<1): raise RuntimeError("1,2 or 3 arguments required.")
    	q = list(arg)
    	for i in range(0,n):
    		if q[i].size != (self.lmax+1)**2: raise RuntimeError("spectral array has wrong size.")
    		if q[i].dtype.num != np.dtype('complex128').num: raise RuntimeError("spectral array should be dtype=complex.")
    		if q[i].flags.contiguous == False: q[i] = q[i].copy()		# contiguous array required.
    	if n==1:	#scalar transform
    		z = np.empty(self.spat_shape, dtype=complex)
    		self.SH_to_spat_cplx(q[0],z)
    		return z
    	elif n==2:	# 2D vector transform
    		zt = np.empty(self.spat_shape, dtype=complex)       # v_theta
    		zp = np.empty(self.spat_shape, dtype=complex)       # v_phi
    		self.SHsphtor_to_spat_cplx(q[0],q[1],zt,zp)
    		return zt,zp
    	else:		# 3D vector transform
    		zr = np.empty(self.spat_shape, dtype=complex)		# v_r
    		zt = np.empty(self.spat_shape, dtype=complex)		# v_theta
    		zp = np.empty(self.spat_shape, dtype=complex)		# v_phi
    		self.SHqst_to_spat(q[0],q[1],q[2],zr,zt,zp)
    		return vr,vt,vp

    def analys_cplx(self,*arg):
    	"""
    	spatial to spectral transform, for complex-valued scalar or vector data.
    	zlm = analys(z) : compute the spherical harmonic representation of the complex scalar z
    	slm,tlm = analys(vtheta,vphi) : compute the spectral spheroidal/toroidal scalars (slm,tlm) from complex-valued 2D vector components (vtheta, vphi)
    	qlm,slm,tlm = synth(vr,vtheta,vphi) : compute the spectral radial/spheroidal/toroidal scalars (qlm,slm,tlm) from complex-valued 3D vector components (vr,vtheta,vphi)
    	"""
    	if self.nlat == 0: raise RuntimeError("Grid not set. Call .set_grid() mehtod.")
    	if self.lmax != self.mmax: raise RuntimeError("complex SH requires lmax=mmax and mres=1.")

    	n = len(arg)
    	if (n>3) or (n<1): raise RuntimeError("1,2 or 3 arguments required.")
    	v = list(arg)
    	for i in range(0,n):
    		if v[i].shape != self.spat_shape: raise RuntimeError("spatial array has wrong shape.")
    		if v[i].dtype.num != np.dtype('complex128').num: raise RuntimeError("spatial array should be dtype=complex128.")
    		if v[i].flags.contiguous == False: v[i] = v[i].copy()		# contiguous array required.
    	if n==1:
    		q = np.empty((self.lmax+1)**2, dtype=complex)
    		self.spat_cplx_to_SH(v[0],q)
    		return q
    	elif n==2:
    		s = np.empty((self.lmax+1)**2, dtype=complex)
    		t = np.empty((self.lmax+1)**2, dtype=complex)
    		self.spat_cplx_to_SHsphtor(v[0],v[1],s,t)
    		return s,t
    	else:
    		q = np.empty((self.lmax+1)**2, dtype=complex)
    		s = np.empty((self.lmax+1)**2, dtype=complex)
    		t = np.empty((self.lmax+1)**2, dtype=complex)
    		self.spat_cplx_to_SHqst(v[0],v[1],v[2],q,s,t)
    		return q,s,t


    def zidx(self, l,m):
    	"""
    	zidx(sht self, int l, int m) -> int : compute the index l*(l+1)+m in a complex spherical harmonic expansion
    	"""
    	l = np.asarray(l)
    	m = np.asarray(m)
    	if (l>self.lmax).any() or (abs(m)>l).any() : raise RuntimeError("invalid range for l,m")
    	return l*(l+1)+m

    def zlm(self, idx):
    	"""
    	zlm(sht self, int idx) -> (int,int) : returns the l and m corresponding to the given index in complex spherical harmonic expansion
    	"""
    	idx = np.asarray(idx)
    	if (idx >= (self.lmax+1)**2).any() or (idx < 0).any() : raise RuntimeError("invalid range for l,m")
    	l = np.sqrt(idx).astype(int)
    	m = idx - l*(l+1)
    	return l,m

    def spec_array_cplx(self):
    	"""return a numpy array that can hold the spectral representation of a complex scalar spatial field."""
    	return np.zeros((self.lmax+1)**2, dtype=complex)

    def spat_array_cplx(self):
    	"""return a numpy array of 2D complex spatial field."""
    	if self.nlat == 0: raise RuntimeError("Grid not set. Call .set_grid() mehtod.")
    	return np.zeros(self.spat_shape, dtype='complex128')


    def SH_to_point(self, Qlm, cost, phi):
        r"""evaluate spherical harmonic expansion Qlm of a real-valued scalar field at point given by cost=cos(theta) and phi."""
        return _shtns.sht_SH_to_point(self, Qlm, cost, phi)

    def SH_to_point_cplx(self, alm, cost, phi):
        r"""evaluate spherical harmonic expansion alm of a complex-valued scalar field at point given by cost=cos(theta) and phi."""
        return _shtns.sht_SH_to_point_cplx(self, alm, cost, phi)

    def SH_to_grad_point(self, DrSlm, Slm, cost, phi):
        r"""SH_to_grad_point(sht self, PyObject * DrSlm, PyObject * Slm, double cost, double phi)"""
        return _shtns.sht_SH_to_grad_point(self, DrSlm, Slm, cost, phi)

    def SHqst_to_point(self, Qlm, Slm, Tlm, cost, phi):
        r"""SHqst_to_point(sht self, PyObject * Qlm, PyObject * Slm, PyObject * Tlm, double cost, double phi)"""
        return _shtns.sht_SHqst_to_point(self, Qlm, Slm, Tlm, cost, phi)

    def SH_to_lat(self, Qlm, cost, Vr):
        r"""SH_to_lat(sht self, PyObject * Qlm, double cost, PyObject * Vr)"""
        return _shtns.sht_SH_to_lat(self, Qlm, cost, Vr)

    def SHqst_to_lat(self, Qlm, Slm, Tlm, cost, Vr, Vt, Vp):
        r"""SHqst_to_lat(sht self, PyObject * Qlm, PyObject * Slm, PyObject * Tlm, double cost, PyObject * Vr, PyObject * Vt, PyObject * Vp)"""
        return _shtns.sht_SHqst_to_lat(self, Qlm, Slm, Tlm, cost, Vr, Vt, Vp)

    def Zrotate(self, Qlm, alpha):
        r"""Zrotate(sht self, PyObject * Qlm, double alpha) -> PyObject *"""
        return _shtns.sht_Zrotate(self, Qlm, alpha)

    def Yrotate(self, Qlm, alpha):
        r"""Yrotate(sht self, PyObject * Qlm, double alpha) -> PyObject *"""
        return _shtns.sht_Yrotate(self, Qlm, alpha)

    def Yrotate90(self, Qlm):
        r"""Yrotate90(sht self, PyObject * Qlm) -> PyObject *"""
        return _shtns.sht_Yrotate90(self, Qlm)

    def Xrotate90(self, Qlm):
        r"""Xrotate90(sht self, PyObject * Qlm) -> PyObject *"""
        return _shtns.sht_Xrotate90(self, Qlm)

    def SH_mul_mx(self, mx, Qlm):
        r"""SH_mul_mx(sht self, PyObject * mx, PyObject * Qlm) -> PyObject *"""
        return _shtns.sht_SH_mul_mx(self, mx, Qlm)

    def spat_to_SH_m(self, Vr, Qlm, im):
        r"""spat_to_SH_m(sht self, PyObject * Vr, PyObject * Qlm, PyObject * im)"""
        return _shtns.sht_spat_to_SH_m(self, Vr, Qlm, im)

    def SH_to_spat_m(self, Qlm, Vr, im):
        r"""SH_to_spat_m(sht self, PyObject * Qlm, PyObject * Vr, PyObject * im)"""
        return _shtns.sht_SH_to_spat_m(self, Qlm, Vr, im)

    def spat_to_SHsphtor_m(self, Vt, Vp, Slm, Tlm, im):
        r"""spat_to_SHsphtor_m(sht self, PyObject * Vt, PyObject * Vp, PyObject * Slm, PyObject * Tlm, PyObject * im)"""
        return _shtns.sht_spat_to_SHsphtor_m(self, Vt, Vp, Slm, Tlm, im)

    def SHsphtor_to_spat_m(self, Slm, Tlm, Vt, Vp, im):
        r"""SHsphtor_to_spat_m(sht self, PyObject * Slm, PyObject * Tlm, PyObject * Vt, PyObject * Vp, PyObject * im)"""
        return _shtns.sht_SHsphtor_to_spat_m(self, Slm, Tlm, Vt, Vp, im)

    def SHsph_to_spat_m(self, Slm, Vt, Vp, im):
        r"""SHsph_to_spat_m(sht self, PyObject * Slm, PyObject * Vt, PyObject * Vp, PyObject * im)"""
        return _shtns.sht_SHsph_to_spat_m(self, Slm, Vt, Vp, im)

    def SHtor_to_spat_m(self, Tlm, Vt, Vp, im):
        r"""SHtor_to_spat_m(sht self, PyObject * Tlm, PyObject * Vt, PyObject * Vp, PyObject * im)"""
        return _shtns.sht_SHtor_to_spat_m(self, Tlm, Vt, Vp, im)

    def spat_to_SHqst_m(self, Vr, Vt, Vp, Qlm, Slm, Tlm, im):
        r"""spat_to_SHqst_m(sht self, PyObject * Vr, PyObject * Vt, PyObject * Vp, PyObject * Qlm, PyObject * Slm, PyObject * Tlm, PyObject * im)"""
        return _shtns.sht_spat_to_SHqst_m(self, Vr, Vt, Vp, Qlm, Slm, Tlm, im)

    def SHqst_to_spat_m(self, Qlm, Slm, Tlm, Vr, Vt, Vp, im):
        r"""SHqst_to_spat_m(sht self, PyObject * Qlm, PyObject * Slm, PyObject * Tlm, PyObject * Vr, PyObject * Vt, PyObject * Vp, PyObject * im)"""
        return _shtns.sht_SHqst_to_spat_m(self, Qlm, Slm, Tlm, Vr, Vt, Vp, im)

# Register sht in _shtns:
_shtns.sht_swigregister(sht)
cvar = _shtns.cvar
__version__ = cvar.__version__


def nlm_calc(lmax, mmax, mres):
    r"""nlm_calc(long lmax, long mmax, long mres) -> long"""
    return _shtns.nlm_calc(lmax, mmax, mres)

def nlm_cplx_calc(lmax, mmax, mres):
    r"""nlm_cplx_calc(long lmax, long mmax, long mres) -> long"""
    return _shtns.nlm_cplx_calc(lmax, mmax, mres)

def set_verbosity(arg1):
    r"""set_verbosity(int arg1)"""
    return _shtns.set_verbosity(arg1)

def print_version():
    r"""print_version()"""
    return _shtns.print_version()

def build_info():
    r"""build_info() -> char const *"""
    return _shtns.build_info()
class rotation(object):
    r"""Proxy of C shtns_rot_ struct."""

    thisown = property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc="The membership flag")
    __repr__ = _swig_repr
    lmax = property(_shtns.rotation_lmax_get, doc=r"""lmax : q(const).int""")
    mmax = property(_shtns.rotation_mmax_get, doc=r"""mmax : q(const).int""")
    alpha = property(_shtns.rotation_alpha_get, doc=r"""alpha : q(const).double""")
    beta = property(_shtns.rotation_beta_get, doc=r"""beta : q(const).double""")
    gamma = property(_shtns.rotation_gamma_get, doc=r"""gamma : q(const).double""")

    def __init__(self, lmax, mmax=-1, norm=0):
        r"""__init__(rotation self, int lmax, int mmax=-1, int norm=0) -> rotation"""
        _shtns.rotation_swiginit(self, _shtns.new_rotation(lmax, mmax, norm))
    __swig_destroy__ = _shtns.delete_rotation

    def set_angles_ZYZ(self, alpha, beta, gamma):
        r"""define a rotation with the 3 intrinsic Euler angles (radians) using ZYZ convention."""
        return _shtns.rotation_set_angles_ZYZ(self, alpha, beta, gamma)

    def set_angles_ZXZ(self, alpha, beta, gamma):
        r"""define a rotation with the 3 intrinsic Euler angles (radians) using ZXZ convention."""
        return _shtns.rotation_set_angles_ZXZ(self, alpha, beta, gamma)

    def set_angle_axis(self, theta, Vx, Vy, Vz):
        r"""define a rotation along axis of cartesian coorinates (Vx,Vy,Vz) and of angle theta (radians)."""
        return _shtns.rotation_set_angle_axis(self, theta, Vx, Vy, Vz)

    def wigner_d_matrix(self, l):
        r"""get the Wigner d-matrix associated with rotation around Y axis (in ZYZ Euler angle convention and for orthonormal harmonics)."""
        return _shtns.rotation_wigner_d_matrix(self, l)

    def apply_real(self, Qlm):
        r"""apply a rotation (previously defined by set_angles_ZYZ(), set_angles_ZXZ() or set_angle_axis()) to a spherical harmonic expansion of a real field with 'orthonormal' convention."""
        return _shtns.rotation_apply_real(self, Qlm)

    def apply_cplx(self, Qlm):
        r"""apply a rotation (previously defined by set_angles_ZYZ(), set_angles_ZXZ() or set_angle_axis()) to a spherical harmonic expansion of a complex-valued field with 'orthonormal' convention."""
        return _shtns.rotation_apply_cplx(self, Qlm)

# Register rotation in _shtns:
_shtns.rotation_swigregister(rotation)



