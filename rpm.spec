Name: bRLtTY
Version: 2.99
Release: 1
Copyright: GPL
URL: http://mielke.cc/brltty/
Packager: Dave Mielke <dave@mielke.cc>
Group: Daemons
Autoprov: 0
Summary: Braille display driver for Linux.
%description
BRLTTY is a background process which provides
access to the Linux console (when in text mode)
for a blind person using a refreshable braille display.
It drives the braille display,
and provides complete screen review functionality.
Some speech capability has also been incorporated.

%prep
%setup

%build
make

%install
make install

%files
%doc DOCS/README DOCS/FAQ DOCS/ChangeLog DOCS/TODO
%doc DOCS/Manual.sgml DOCS/Manual.txt DOCS/Manual-HTML
%doc DOCS/Manual-ger.sgml DOCS/Manual-ger.txt DOCS/Manual-ger-HTML
%config(noreplace) /etc/brltty.conf
/sbin/brltty
/sbin/install-brltty
/etc/brltty
/lib/brltty
