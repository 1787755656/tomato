Summary: User-space IPsec tools for the Linux IPsec implementation
Name: ipsec-tools
Version: 0.8.1
Release: 1
Epoch: 1
License: BSD
Group: System Environment/Base
URL: http://ipsec-tools.sourceforge.net/
Source: http://prdownloads.sourceforge.net/%{name}/%{name}-%{version}.tar.gz
Requires: kernel >= 2.5.54

#BuildRequires: kernel-source >= 2.5.54
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

%description
IPsec-Tools is a port of the KAME Project's IPsec tools to the Linux
IPsec implementation. IPsec-Tools provides racoon, an IKE daemon; libipsec,
a PFKey implementation; and setkey, a security policy and security
association database configuration utility.

%prep
%setup -q

%build
./configure --prefix=/usr --sysconfdir=/etc --exec-prefix=/ --mandir=%{_mandir} --libdir=/%{_lib}
make

%install
rm -rf %{buildroot}
mkdir %{buildroot}
make install DESTDIR=%{buildroot}

%post -p /sbin/ldconfig  

%postun -p /sbin/ldconfig 

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%doc NEWS README ChangeLog
%dir %{_sysconfdir}/racoon
%config %{_sysconfdir}/racoon/*
/sbin/*
/%{_lib}/*
%{_includedir}/*
%{_mandir}/man[358]/*
%{_sbindir}/racoon

%changelog
* Fri Mar 07 2003 Derek Atkins <derek@ihtfp.com> 0.2.1-1
- Insert into code base.  Dynamically generate the version string.

* Fri Mar 07 2003 Chris Ricker <kaboom@gatech.edu> 0.2.1-1
- Rev to 0.2.1 release
- Remove unneeded patch

* Thu Mar 06 2003 Chris Ricker <kaboom@gatech.edu> 0.2-1
- initial package
