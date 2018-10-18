# sec-web

Note for myself to how to get internet access and some other things in Arch Linux without bigger manager programs.

Squid+iptables solution for HTTPS-only outgoing connections.

Sajat konfigok a netelereshez.

Squid+iptables beallitasok ahhoz, hogy csak HTTPS-only kapcsolatokat engedjen ki meg vissza SSH-t.

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
-A INPUT -i lo -j ACCEPT
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
```

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

## WPA Supplicant usage

Enable `wfa_cli`:
`echo update_config=1 > /etc/wpa_supplicant/wpa_supplicant.conf`

Start `wpa_supplicant`:
`wpa_supplicant -B -i wlp2s0 -c /etc/wpa_supplicant/wpa_supplicant.conf`

Start `wfa_cli`:
`wfa_cli -i wlp2s0`

Don't forget these options as they sometimes do not communicate the errors well. (e.g. fail to connect, no root access)

## Firefox

Downloaded newest binary version from site.

Installed Adblocker, opted out some settings (sending statistics, crash dumps, etc.)

At `about:config`:

Set `security.tls.version.min` higher.
Search for `ssl3` (or `security.ssl3', not really related to ssl3). Opted out some cipher modes.

## Bugs

There are some cases they do not work.
Restarting iptables while squid is alive locks out squid's open connections(?)
Of course there are sites not supporting TLS 1.2, 1.1, more with 1.0 or not even HTTPS. That's why I created `netuser`.

## TODO

Filter packets by HTTPS version and cipher suite.