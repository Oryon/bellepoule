%define vcs_rev 264
Name:           BellePoule
Version:        1.92
Release:        0.%{vcs_rev}%{?dist}
Summary:        Fencing tournaments management software / Logiciel de gestion de tournois d'escrime.

Group:          Applications/Productivity
License:        GNU GPL v3
URL:            http://betton.escrime.free.fr/index.php/bellepoule
Source0:        BellePoule-%{version}-r%{vcs_rev}.tgz
Patch1:         f14_compile.patch
Patch2:         64_bit.patch
Patch3:         rpm.patch
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  gtk2-devel glib2-devel libxml2-devel goocanvas-devel
Requires:       gtk2 glib2 libxml2 goocanvas

%description


%prep
%setup -n BellePoule-%{version}-r%{vcs_rev}
%patch1 -p1 -b .f14_compile
%patch2 -p1 -b .64_bit
%patch3 -p1 -b .rpm_patch


%build
make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT bindir=%{_bindir}


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%{_bindir}/*
%doc Exemples setup/COPYING.txt

%changelog
* Mon Apr 18 2011 ajsfedora <ajsfedora@gmail.com> 2.0-0.92
- Initial .spec file

