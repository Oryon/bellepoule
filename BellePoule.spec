%define vcs_rev 269
Name:           BellePoule
Version:        2.0
Release:        0.beta5.%{vcs_rev}%{?dist}
Summary:        Fencing tournaments management software / Logiciel de gestion de tournois d'escrime.

Group:          Applications/Productivity
License:        GNU GPL v3
URL:            http://betton.escrime.free.fr/index.php/bellepoule
Source0:        BellePoule-%{version}.beta5-r%{vcs_rev}.tgz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  desktop-file-utils gettext gtk2-devel glib2-devel libxml2-devel goocanvas-devel
Requires:       gtk2 glib2 libxml2 goocanvas

%description


%prep
%setup -n BellePoule-%{version}.beta5-r%{vcs_rev}

%build
make %{?_smp_mflags}

## compile all of the message files
cd resources/translations
for lll in ar de es fr it nl ru ; do 
  msgfmt --output-file=$lll/LC_MESSAGES/BellePoule.mo $lll.po
done

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT prefix=usr
%find_lang %{name}
desktop-file-install --dir=${RPM_BUILD_ROOT}%{_datadir}/applications %{name}.desktop

%clean
rm -rf $RPM_BUILD_ROOT

%files -f %{name}.lang
%defattr(-,root,root,-)
%{_bindir}/*
%doc Exemples setup/COPYING.txt

%changelog
* Mon May 9 2011 ajsfedora <ajsfedora@gmail.com> 1.92-0.268
- Update to BellePoule-Fedora rev 277 (BellePoule trunk r268 with the XDM
  utility functions applied

* Mon Apr 18 2011 ajsfedora <ajsfedora@gmail.com> 2.0-0.92
- Initial .spec file
