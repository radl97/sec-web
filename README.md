# sec-web

Note for myself to how to get internet access and some other things in Arch Linux without bigger manager programs.

~Squid~ tinyproxy+iptables solution for HTTPS-only outgoing connections.

### Create unconstrained Internet access for a group

Create a group allowing unconstrained access with a user of the same name.

```
useradd -M -U netuser
iptables -A OUTPUT -m owner --gid-owner netuser -j ACCEPT
```

Log in from a sudoer user: `sudo su netuser`

### Create promiscuous mode access

```
useradd -M -g wireshark wireshark
```

Log in from a sudoer user: `sudo su wireshark -c wireshark-gtk`

Does not need any iptables config.

On first startup, it will need `xhost +local:`, enabling local applications from other users to use X. (Which might reduce security)

## Iptables configurations

Default behaviour:

```
iptables -P INPUT DROP
iptables -P FORWARD DROP
iptables -P OUTPUT DROP
```

Based on Archlinux's [simple firewall](https://wiki.archlinux.org/index.php/Simple_stateful_firewall)

```
iptables -A INPUT -m conntrack --ctstate INVALID -j DROP
iptables -A INPUT -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT
iptables -A INPUT -i lo -j ACCEPT
```

Accept port 6944 for a server
`iptables -A INPUT -p tcp -m tcp --dport 6944 -j ACCEPT`

Accept netuser:
`iptables -A OUTPUT -m owner --gid-owner netuser -j ACCEPT`

Accept outgoing data with HTTPS from squid:
`iptables -A OUTPUT -p tcp -m tcp --dport 443 -m owner --gid-owner squid -j ACCEPT`

Accept outgoing DNS:
```
iptables -A OUTPUT -p tcp -m tcp --dport 53 -j ACCEPT
iptables -A OUTPUT -p udp -m udp --dport 53 -j ACCEPT
iptables -A OUTPUT -o lo -j ACCEPT
```

iptables now:
```
-A INPUT -m conntrack --ctstate INVALID -j DROP
-A INPUT -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT
-A INPUT -i lo -j ACCEPT
-A INPUT -p icmp -j ACCEPT
-A INPUT -p tcp -m conntrack --ctstate NEW -m tcp --dport 22 -j ACCEPT
-A OUTPUT -p tcp -m tcp --dport 443 -m owner --gid-owner 186 -j ACCEPT
-A OUTPUT -m owner --gid-owner 1002 -j ACCEPT
-A OUTPUT -p tcp -m tcp --dport 53 -j ACCEPT
-A OUTPUT -p udp -m udp --dport 53 -j ACCEPT
-A OUTPUT -o lo -j ACCEPT
-A OUTPUT -m conntrack --ctstate RELATED,ESTABLISHED -j ACCEPT
```

And do not forget to save the config (and copy the behaviour for IPv6)
`iptables-save > /etc/iptables/iptables.rules`
`cp /etc/iptables/iptables.rules /etc/iptables/ip6tables.rules`
`ip6tables-restore /etc/iptables/ip6tables.rules`

## Squid config

Based on basic Squid default config from Archlinux's setup.

```
acl SSL_ports port 443
acl CONNECT method CONNECT
```

Enable only HTTPS:
```
http_access deny !SSL_ports
http_access deny CONNECT !SSL_ports
```

Only allow cachemgr access from localhost
```
http_access allow localhost manager
http_access deny manager
```

Protect who think the only one who can access services on "localhost" is a local user
`http_access deny to_localhost`

`http_access allow localhost`

Deny all other access to this proxy:
`http_access deny all`

`http_port 3128`

And default options...

Somewhy needs restart on connecting to something every time:
`systemctl restart squid`

Add `export HTTPS_PROXY=localhost:3128` to .bashrc or your favourite shell

## WPA Supplicant usage

Enable `wpa_cli`:
`echo update_config=1 > /etc/wpa_supplicant/wpa_supplicant.conf`

Start `wpa_supplicant`:
`sudo wpa_supplicant -B -i wlp2s0 -c /etc/wpa_supplicant/wpa_supplicant.conf`

Start `wpa_cli`:
`sudo wpa_cli -i wlp2s0`

Don't forget these options as they sometimes do not communicate the errors well. (e.g. fail to connect, no root access)


### Enabling the service

`wpa_supplicant -B -i wlp2s0 -c /etc/wpa_supplicant/wpa_supplicant.conf`

Open `/usr/lib/systemd/system/wpa_supplicant.service`, and modify the service's execStart to:
`execStart=/usr/bin/wpa_supplicant -u -i wlp2s0 -c /etc/wpa_supplicant/wpa_supplicant.conf`

And run `systemctl enable wpa_supplicant`

You might need to run `systemctl reload-daemon`, if it says so.

### Upon connecting, if DHCP is not enabled, you might want to start dhcpcd:

`sudo dhcpcd wlp2s0` where `wlp2s0` is the device name.

You can also enable it with systemctl:

`systemctl enable dhcpcd`

## Firefox

Downloaded newest binary version from site.

Installed Adblocker, opted out some settings (sending statistics, crash dumps, etc.)

At `about:config`:

Set `security.tls.version.min` higher.
Search for `ssl3` (or `security.ssl3', not really related to ssl3). Opted out some cipher modes.

### Install sound

`sudo pacman -Syu alsa-utils pulseaudio pulseaudio-alsa`

You must start pulseaudio on startup:

You can use `alsamixer` to control sound.

Based on some site I added a new file `/etc/dbus-1/system.d/pulseaudio.conf`
with the following content:
```
<!DOCTYPE busconfig PUBLIC
 "-//freedesktop//DTD D-BUS Bus Configuration 1.0//EN"
 "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig>
        <policy user="pulse">
            <allow own="org.pulseaudio.Server"/>
            <allow send_destination="org.pulseaudio.Server"/>
            <allow receive_sender="org.pulseaudio.Server"/>
        </policy>
</busconfig>
```

After that running pulseaudio works perfectly.

## Install clipman

For middle-button pasting, install `xfce4-clipman`

## Bugs

There are some cases they do not work.
Restarting iptables while squid is alive locks out squid's open connections(?)
Of course there are sites not supporting TLS 1.2, 1.1, more with 1.0 or not even HTTPS. That's why I created `netuser`.

## Connecting to BME Eduroam

`sudo mkdir -p /etc/wpa_supplicant/ca`
Copy `eduroam-ca-cert.pem` to `/etc/wpa_supplicant/ca/eduroam-ca-cert.pem`.

Add this to `/etc/wpa_supplicant/wpa_supplicant.conf`:
```
network={
	ssid="eduroam"
	scan_ssid=1
	key_mgmt=WPA-EAP
	eap=TTLS
	identity="aa0000@hszk.bme.hu"
	anonymous_identity="@bme.hu"
	password="<password>"
	ca_cert="/etc/wpa_supplicant/ca/eduroam-ca-cert.pem"
	phase2="auth=PAP"
}
```

Note the `"auth=PAP"` part, what contradicts all the websites which say `"auth=MSCHAPV2"` is good. EAP authentication would fail this way.

You can also configure it through `wpa_cli`.

## Start PulseAudio on startup

`xfce4-session-settings`, application autostart and tick PulseAudio

## Squid too slow

`pacman -Syu tinyproxy`

Squid tries to cache, but its useless on HTTPS... It's gotten very slow with time (shutdown time).

So I've removed squid and installed tinyproxy instead:
You have to rewrite the rule about 
`iptables -I OUTPUT 1 -p tcp -m tcp --dport 443 -m owner --gid-owner 186 -j ACCEPT`
and remove the squid's rule.

You have to allow tinyproxy PID to run as a systemctl service.
Of course, `systemctl enable tinyproxy` is needed.

## TODO

- Enable logging
- Filter packets by SSL version and cipher suite.
- Adblock might sell your data [citation needed](), install Ublock origin?
- Squid SOCKSv5?
- DNSSec
- Redirect HTTP connections to HTTPS with the proxy?
- Enable/configure VPN, IPSec?
- What to install for these to work
- Daemonize
- Enable sound control buttons on the laptop.
- Fix clipboard...

## Internet status board

I use XFCE4 and hacked the `xfce4-datetime-plugin`.

I've created two scripts: `wifistart` to start `wpa_supplicant`, the other: `wifistat`.
This collects data from network: WiFi SSID, scan on/off, eth/wifi running DHCP instances (server or client), IP addresses, and prints it in a very compact format.

Added the specific commands to sudoers so it can be used by anyone to poll net data or start wifi.
