%define vcs_rev 268
Name:           BellePoule
Version:        1.92
Release:        0.%{vcs_rev}%{?dist}
Summary:        Fencing tournaments management software / Logiciel de gestion de tournois d'escrime.

Group:          Applications/Productivity
License:        GNU GPL v3
URL:            http://betton.escrime.free.fr/index.php/bellepoule
Source0:        BellePoule-%{version}-r%{vcs_rev}.tgz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  gtk2-devel glib2-devel libxml2-devel goocanvas-devel
Requires:       gtk2 glib2 libxml2 goocanvas

%description


%prep
%setup -n BellePoule-%{version}-r%{vcs_rev}

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
* Mon May 9 2011 ajsfedora <ajsfedora@gmail.com> 1.92-0.268
- Update to BellePoule-Fedora rev 277 (BellePoule trunk r268 with the XDM
  utility functions applied

* Mon Apr 18 2011 ajsfedora <ajsfedora@gmail.com> 2.0-0.92
- Initial .spec file
