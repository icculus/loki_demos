%define name loki_demos
%define version 1.0d
%define release 1

Summary: Loki Demo Launcher
Name: %{name}
Version: %{version}
Release: %{release}
Copyright: GPL
Group: Applications
Vendor: Loki Software, Inc.
Packager: Sam Lantinga <hercules@lokigames.com>
Requires: loki_update loki_uninstall

%description
This is a game demo launch utility provided by Loki Software, Inc.

%post
for dir in "$HOME/.loki" "$HOME/.loki/installed"
do test -d "$dir" || mkdir "$dir"
done
ln -sf /usr/local/Loki_Demos/.manifest/Loki_Demos.xml $HOME/.loki/installed

%postun
rm -f $HOME/.loki/installed/Loki_Update.xml

%files
/usr/local/games/Loki_Demos/
/usr/local/bin/loki_demos

%changelog
* Fri Dec 8 2000 Sam Lantinga <hercules@lokigames.com>
- First attempt at a spec file for loki_demos

# end of file
