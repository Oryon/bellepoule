%define debug_package %{nil}

Name:           __PRODUCT__
Version:        __VERSION__
Release:        __VERSION_MICRO__%{?dist}
Summary:        Fencing tournament management software

Group:          Application/Productivity
License:        GPLv3+
URL:            http://betton.escrime.free.fr/fencing-tournament-software/en/bellepoule/index.html
Source0:        %{name}_%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  gcc-c++
BuildRequires:  gtk2-devel >= 2.24.0
BuildRequires:  libxml2-devel
BuildRequires:  libcurl-devel
BuildRequires:  libmicrohttpd-devel
BuildRequires:  goocanvas-devel
BuildRequires:  qrencode-devel
BuildRequires:  openssl-devel
BuildRequires:  json-glib-devel
BuildRequires:  webkitgtk-devel
Requires:       gtk2 >= 2.24.0
Requires:       libxml2
Requires:       libcurl
Requires:       libmicrohttpd
Requires:       goocanvas
Requires:       qrencode-libs
Requires:       openssl-libs
Requires:       lighttpd
Requires:       json-glib
Requires:       lighttpd-fastcgi
Requires:       webkitgtk

%description


%prep
%setup -q -n %{name}_%{version}


%build
make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT


%clean
rm -rf $RPM_BUILD_ROOT


%files
%{_bindir}/%{name}-supervisor
%{_bindir}/%{name}-marshaller
%{_libdir}/%{name}/lib%{name}.so
%{_datadir}/applications/%{name}-supervisor.desktop
%{_datadir}/applications/%{name}-marshaller.desktop
%{_datadir}/%{name}


%changelog
