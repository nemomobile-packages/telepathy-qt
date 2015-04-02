# 
# Do NOT Edit the Auto-generated Part!
# Generated by: spectacle version 0.27
# 

Name:       telepathy-qt5

# >> macros
# << macros

Summary:    Qt 5 Telepathy library
Version:    0.9.3
Release:    1
Group:      System/Libraries
License:    GPLv2
URL:        http://telepathy.freedesktop.org/wiki/
Source0:    http://telepathy.freedesktop.org/releases/telepathy-qt/%{name}-%{version}.tar.gz
Source1:    INSIGNIFICANT
Source2:    mktests.sh.in
Source3:    runDbusTest.sh.in
Source4:    runTest.sh.in
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5Network)
BuildRequires:  pkgconfig(Qt5Test)
BuildRequires:  pkgconfig(Qt5Xml)
BuildRequires:  pkgconfig(farstream-0.1)
BuildRequires:  pkgconfig(telepathy-glib) >= 0.17.5
BuildRequires:  pkgconfig(telepathy-farstream) >= 0.4.0
BuildRequires:  pkgconfig(gstreamer-0.10)
BuildRequires:  pkgconfig(gstreamer-interfaces-0.10)
BuildRequires:  pkgconfig(libxml-2.0)
BuildRequires:  python
BuildRequires:  dbus-python
BuildRequires:  doxygen
BuildRequires:  cmake

%description
Qt-based library for Telepathy components.


%package devel
Summary:    Development files for %{name}
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
This package contains libraries and header files for developing applications
that use %{name}.


%package farstream
Summary:    Qt 5 Telepathy/Farstream integration
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}
Requires:   telepathy-qt5 = %{version}
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
Obsoletes:  %{name}-farsight

%description farstream
This package provides telepathy-qt5 integration with telepathy-farstream,
which implements media stream using gstreamer and Farstream.


%package farstream-devel
Summary:    Development files for telepathy-qt5-farstream
Group:      Development/Libraries
Requires:   %{name}-farstream = %{version}-%{release}
Requires:   telepathy-qt5-devel = %{version}
Requires:   telepathy-farstream-devel
Obsoletes:  %{name}-farsight-devel

%description farstream-devel
This package contains libraries and header files for developing applications
that use telepathy-qt5-farstream.


%package tests
Summary:    Automated tests for %{name}
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}
Requires:   %{name} = %{version}
Requires:   dbus-python
Requires:   pygobject2

%description tests
This package contains automated tests and tests.xml


%prep
%setup -q -n %{name}-%{version}/telepathy-qt

# >> setup
# << setup

%build
# >> build pre
%__cp $RPM_SOURCE_DIR/INSIGNIFICANT tests/
%__cp $RPM_SOURCE_DIR/mktests.sh.in tests/
%__cp $RPM_SOURCE_DIR/runDbusTest.sh.in tests/
%__cp $RPM_SOURCE_DIR/runTest.sh.in tests/
%__chmod 0644 tests/INSIGNIFICANT
%__chmod 0755 tests/mktests.sh.in
%__chmod 0755 tests/runDbusTest.sh.in
%__chmod 0755 tests/runTest.sh.in

export QT_SELECT=5
%cmake -DENABLE_TESTS=TRUE -DENABLE_FARSIGHT=FALSE -DENABLE_FARSTREAM=TRUE -DENABLE_EXAMPLES=FALSE
# << build pre


make %{?_smp_mflags}

# >> build post
tests/mktests.sh > tests/tests.xml
# << build post

%install
rm -rf %{buildroot}
# >> install pre
export QT_SELECT=5
# << install pre
%make_install

# >> install post
# << install post

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%post farstream -p /sbin/ldconfig

%postun farstream -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
# >> files
%{_libdir}/libtelepathy-qt5.so.*
# << files

%files devel
%defattr(-,root,root,-)
# >> files devel
%{_libdir}/libtelepathy-qt5.so
%{_libdir}/pkgconfig/TelepathyQt5.pc
%{_includedir}/telepathy-qt5/TelepathyQt/*
%{_libdir}/cmake/TelepathyQt5/*.cmake
# << files devel

%files farstream
%defattr(-,root,root,-)
# >> files farstream
%{_libdir}/libtelepathy-qt5-farstream.so.*
# << files farstream

%files farstream-devel
%defattr(-,root,root,-)
# >> files farstream-devel
%{_libdir}/libtelepathy-qt5-farstream.so
%{_includedir}/telepathy-qt5/TelepathyQt/Farstream/*
%{_libdir}/cmake/TelepathyQt5Farstream/TelepathyQt5FarstreamConfig.cmake
%{_libdir}/cmake/TelepathyQt5Farstream/TelepathyQt5FarstreamConfigVersion.cmake
%{_libdir}/pkgconfig/TelepathyQt5Farstream.pc
# << files farstream-devel

%files tests
%defattr(-,root,root,-)
/opt/tests/%{name}/*
# >> files tests
# << files tests
