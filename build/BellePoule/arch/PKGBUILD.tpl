pkgname=#APPLICATION#PHASE
pkgver=#MAJOR.#MINOR
pkgrel=#MATURITY
pkgdesc="Fencing tournament management software"
arch=('i686' 'x86_64')
url="http://betton.escrime.free.fr/fencing-tournament-software/en/bellepoule/index.html"
license=('GPL3')
depends=(#DEPS)
source=("https://launchpad.net/~betonniere/+archive/ubuntu/#APPLICATION/+files/${pkgname}_${pkgver}ubuntu${pkgrel}~xenial1.tar.gz")
sha256sums=('#SHA256')

build() {
    cd "${pkgname}_${pkgver}"
    make
}

package() {
    cd "${pkgname}_${pkgver}"
    make DESTDIR="$pkgdir/" install
}
