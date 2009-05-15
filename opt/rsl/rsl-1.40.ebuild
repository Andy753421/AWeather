inherit eutils autotools

DESCRIPTION="Radar Software Library"
HOMEPAGE="http://trmm-fc.gsfc.nasa.gov/trmm_gv/software/rsl/index.html"
SRC_URI="ftp://trmm-fc.gsfc.nasa.gov/software/rsl-v${PV}.tar.gz"
S="${WORKDIR}/rsl-v${PV}"

LICENSE="LGPL-2"
SLOT="0"
KEYWORDS="~x86"

RDEPEND="sci-libs/hdf
	media-libs/jpeg"
DEPEND="${RDEPEND}"

src_unpack() {
	unpack ${A}
	cd "${S}"
	epatch "${FILESDIR}/${PN}-warnings.patch"
	epatch "${FILESDIR}/${PN}-automake.patch"
	epatch "${FILESDIR}/${PN}-type_str.patch"
	epatch "${FILESDIR}/${PN}-valgrind.patch"
	elibtoolize
	eautoreconf
}

src_install() {
	emake DESTDIR="${D}" install || die "install failed"
	dodoc README CHANGES || die
}
