pkgname=__APPLICATION__
pkgver=beta__MAJOR__.__MINOR__
pkgrel=1
pkgdesc="Fencing tournament management software"
arch=('i686' 'x86_64')
url="http://betton.escrime.free.fr/fencing-tournament-software/en/bellepoule/index.html"
license=('GPL3')
depends=('gtk2>=2.24.0' 'xml2' 'curl' 'libmicrohttpd' 'goocanvas1' 'qrencode' 'openssl' 'lighttpd' 'php-cgi' 'gksu')
source=("https://launchpad.net/~betonniere/+archive/ubuntu/__APPLICATION__/+files/${pkgname}_${pkgver}ubuntu1~xenial1.tar.gz")
sha256sums=('__SHA256__')

build() {
    cd "${pkgname}_${pkgver}"
    make
}

package() {
    cd "${pkgname}_${pkgver}"
    make DESTDIR="$pkgdir/" install
}
