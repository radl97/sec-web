# Sniffing an Android application's communication in Linux

For debugging, I've needed to sniff an application's communication with their server.
This sounds easy at first (my solution is not that hard either), but there were
two main problems: **the application uses HTTPS**. This is usually solved by a proxy
with a trusted certificate (e.g. mitmproxy, burpSuite), but Android applications
explicitly enable proxy support. So a standard **HTTPS proxy cannot be used** here.

**Note:** HTTPS proxy is not a security threat, because the user has to install it
first. Android manages these certificates in a separate way than official ones,
and the user is regularly alerted that there is a trusted non-official cert.

And there is another quirk: I've installed the application in Android emulator.
I've had a hard time configuring it, so I've used it as a black box which interacts
as an application. (It probably has some settings I've yet to find)

What softwares I used:

- BurpSuite community version, which has an "invisible proxy"
- `dnsmasq` for a simple DNS server. The main goal was to redirect everything to the proxy.
- `netns` for isolating the emulator from the network. This gives us a position in the middle of communication.
- `socat` is used for a simple redirection. This is one alternative to running BurpSuite as root.

## Who this is for?

This document lists some tricks I've found while digging through options.
What this document contains:

- **Isolating an application** from the network
- Using BurpSuite's **invisible proxy** with a **fake DNS server**
- Setting up and **uploading a fake certificate to** an **Android** device or an Android emulator.
- Using `socat` for redirecting all communication of a given port to an other machine or port (**"plaintext" tunneling**).

**Note:** There are other (more efficient) alternatives to using `socat`.

## General notes

I've used this for production, and documented these to be able to reproduce it later.
Reproduction is the main goal, so nothing is changed, some notes are there for probably
working easier solutions.

I will try to add links about how to find more information.

I use ArchLinux, some settings might differ.

## Creating a network namespace

The Linux kernel has capability to virtual networks. It's not whole isolation,
which comes in handy because I did not need to install everything to e.g. a Docker
container. This isolation can be handled as root with `netns` (network namespace).

Run these as root (or as sudoer with `sudo`):

```bash
ip netns add alma
ip link add veth0 type veth peer veth1
ip link set veth1 netns alma
ip address add 10.0.42.1/24 dev veth0
ip link set dev veth0 up
iptables -A INPUT -i veth0 -j ACCEPT
```

Note that these must be run on the "main" network (as I will call it later on), not inside the namespace.

The first three lines create a network namespace `alma`, a virtual ethernet
bridge (with two interfaces `veth0` and `veth1`, and `veth1` moves to the newly created namespace `alma`.

The next line `ip address add 10.0.42.1/24 dev veth0` sets up the network layer, and 5th line
sets up the link layer. (These lines swapped would be more logical. Probably that works too)

**Note:** Probably 10.0.42.1/30` would have been better. Later it is used (sorry for
the inconsistency).

The last line tells the firewall to enable incoming connection from the interface.

**Note:** I'm using iptables directly, and all interfaces are disabled by default. You might
or might not need it. Also, if you use an "other firewall", you should probably configure
the firewall there.

## Using the new namespace

After setting up everything, you can create a new terminal session with `netns exec`:

`ip netns exec alma bash`

Note that you have to run this as root.

where alma is the name of the newly created namespace.

Configuration within the namespace:

```bash
ip address add 10.0.42.2/30 dev veth1
ip address add 8.8.8.8/24 dev lo
ip link set dev lo up
ip link set dev veth1 up
```

This namespace was handed the veth1 interface of the virtual ethernet.

We set up the address to be used with the first line and enable it with the last line.

The third line is needed because loopback is not enabled by default in the new namespace (for my computer).

The second line is the last to be explained: `ip address add 8.8.8.8/24 dev lo`
For our use, it can be simplified as "8.8.8.8 is this machine". Note that this line also
hides 255 other IP addresses. This will be our fake DNS server's address.

The idea was that if android emulator forcably uses 8.8.8.8, then it's not a problem for us.
This would not work in common conditions, but that struck me only later.

You can now test the network with `ping 10.0.42.1` from within the namespace,
and `ping 10.0.42.2` from the outside to see if everything works.

## Configuring the services inside the net namespace

All configuration files that could be found in `/etc/netns/alma/` where alma is the netns name.
This is done by mounting `/etc/netns/alma/` to `/etc/` when using `ip netns exec alma something`.

**Note:** you may have to create the directory `/etc/netns/alma/` first.

### DnsMasq

**Note:** Uhmm, maybe a simple `hosts` file would have done everything needed here. -.-
Or maybe using `iptables` PRE-ROUTING table could have been used here. 

Dnsmasq is a simple but powerful tool for setting up a small network.
It serves a DHCP server, but we will not need it.

Cherry-picking from the example configuration file, I've come up with these options:

```bash
cat >/etc/netns/alma/dnsmasq.conf <<EOF
no-resolv
address=/www.example.com/10.0.42.2
address=/test.example.com/10.0.42.2
interface=lo
no-dhcp-interface=
no-hosts
EOF
```

`no-resolv` and `no-hosts` were (I think) crucial for this to work. `no-dhcp-interface=`
disables the network.

There can be multiple `address=` lines, use all domain names that you need.

The goal was to map every domain name to the same address.

Dnsmasq uses [wildcards](https://stackoverflow.com/questions/22313142/wildcard-subdomains-with-dnsmasq),
but `/etc/hosts` only works for a given domain without subdomains. Maybe `address=/./10.0.42.2` would have worked.

Note that if you do uses `hosts`, then you should place it in `/etc/netns/alma/hosts` instead,
so it does not affect the main namespace.

Setting up the DNS server with `resolv.conf`:

```bash
cat >/etc/netns/alma/resolv.conf <<EOF
nameserver 8.8.8.8
EOF
```

Running the DNS server is as simple as `dnsmasq -kqC /etc/netns/alma/dnsmasq.conf`.

Note that this way the program does not run as a daemon. You can run multiple applications
in the same namespace: just start `ip netns exec alma bash` again in an other terminal.

### Redirecting some privileged ports

[This](https://www.papercut.com/support/resources/manuals/ng-mf/common/topics/customize-enable-additional-ports.html#customization) might be a more useful resource.

I've used an ugly solution, with `socat` (which definitely more CPU consuming).

The point was to redirect all connections to the main network, from the ports 80 or 443,
to the BurpSuite's 8080 unprivileged port (which does not run as root).

Using socat is easy: for every connection to the TCP port 80 ("every" -> `reuseaddr,fork` options),
connect to `10.42.0.1:8080` as TCP connection.

```bash
socat TCP-listen:80,reuseaddr,fork,su=nobody TCP:10.0.42.1:8080&
socat TCP-listen:443,reuseaddr,fork,su=nobody TCP:10.0.42.1:8080
```

The same goes for 443 port too.

### Running android emulator

Do not run as root: `su username`.

Then running `android-studio` (or directly android-emulator) will see a network
where every request goes to `10.0.42.1:8080`.

Running android emulator: `emulator -netdelay none -netspeed full -avd Pixel_2_API_29`

### Running Burp Suite

There are a whole lot of tutorials, but the main points are:

Enable 8080 port on the main network: `iptables -A INPUT -i veth0 -p tcp -m tcp --dport 8080 -j ACCEPT`.

Run simply the burp suite. The "Proxy" tab is what we will need.

Here you have the option for run as "invisible proxy".
A HTTP(S) or SOCKS proxy is a standard, visible way to intercept data.
However, Android applications do not respect proxy settings (which is quite a pain...).

So we will need the invisible proxy. That's what all the fuss was about.
How it works: from the HTTP header "Host:" or the TLS connection data, the burpsuite
can determine the host to redirect it to.

## Conclusion

Using `socat` was not needed, as `iptables` (the Linux firewall) is capable of handling that.

Also fake DNS server is not needed either.
