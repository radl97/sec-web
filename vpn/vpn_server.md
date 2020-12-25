Server config:

Generate keys: Server key and CA key for verifying the server
Generate CA certificate and server certificate, the latter verifying that the server is from the CA

```
ipsec pki --gen --size 4096 --type rsa --outform pem > /etc/ipsec.d/private/server.key.pem
ipsec pki --gen --size 4096 --type rsa --outform pem > /etc/ipsec.d/private/ca.key.pem
# not sure the CN is needed to be the IP
ipsec pki --self --in /etc/ipsec.d/private/ca.key.pem --type rsa --dn "CN=<IP-address>" --ca --lifetime 3650 --outform pem > /etc/ipsec.d/cacerts/ca.cert.pem
ipsec pki --pub --in /etc/ipsec.d/private/server.key.pem --type rsa | ipsec pki --issue --lifetime 2750 --cacert /etc/ipsec.d/cacerts/ca.cert.pem --cakey /etc/ipsec.d/private/ca.key.pem --dn "CN=<IP-address>" --san="<IP-address>" --flag serverAuth --flag ikeIntermediate --outform pem > /etc/ipsec.d/certs/server.cert.pem
```

```
$ sudo cat /etc/ipsec.secrets 
# This file holds shared secrets or RSA private keys for authentication.

# RSA private key for this host, authenticating it to any other host
# which knows the public part.
: RSA "server.key.pem"
user1 : EAP "pw1"
user2 : EAP "pw2"
```

Set up firewall. Note that 500, 4500 and ESP (Protocol 50 over IP) need to be accepted

```
$ sudo iptables -S
-P INPUT ACCEPT
-P FORWARD ACCEPT
-P OUTPUT ACCEPT
$ sudo iptables -t nat -S
-P PREROUTING ACCEPT
-P INPUT ACCEPT
-P OUTPUT ACCEPT
-P POSTROUTING ACCEPT
-A POSTROUTING -s <SUBNET> -o ens5 -m policy --dir out --pol ipsec -j ACCEPT
-A POSTROUTING -s <SUBNET> -o ens5 -j MASQUERADE
$ sudo iptables -t mangle -S # smaller packets because of UDP+ESP tunnel
-P PREROUTING ACCEPT
-P INPUT ACCEPT
-P FORWARD ACCEPT
-P OUTPUT ACCEPT
-P POSTROUTING ACCEPT
-A FORWARD -p tcp -m policy --dir in --pol ipsec -m tcp --tcp-flags SYN,RST SYN -m tcpmss --mss 1361:1536 -j TCPMSS --set-mss 1360
-A FORWARD -p tcp -m policy --dir out --pol ipsec -m tcp --tcp-flags SYN,RST SYN -m tcpmss --mss 1361:1536 -j TCPMSS --set-mss 1360
```

Set up IP forwarding (securely):

```
$ cat /etc/sysctl.conf | grep '^[^#]'
net.ipv4.ip_forward=1
net.ipv6.conf.all.forwarding=1
net.ipv4.conf.all.accept_redirects = 0
net.ipv4.conf.all.send_redirects = 0
$ sudo sysctl -p
```

Set up IPSec server (securely):

```
$ cat /etc/ipsec.conf 
config setup
        #charondebug="cfg 2, dmn 2, ike 2, net 2, knl 2, esp 2, mgr 2" # good for debugging!
        # see journalctl -xe if running from systemd service
        # strictcrlpolicy=yes
        uniqueids=yes

conn IPSec-IKEv2-EAP
        auto=add
        compress=no
        type=tunnel

        keyexchange=ikev2

        dpdaction=clear
        dpddelay=300s

        rekey=no

        ike=aes128-sha256-ecp256,aes256-sha384-ecp384,aes128-sha256-modp2048,aes128-sha1-modp2048,aes256-sha384-modp4096,aes256-sha256-modp4096,aes256-sha1-modp4096,aes128-sha256-modp1536,aes128-sha1-modp1536,aes256-sha384-modp2048,aes256-sha256-modp2048,aes256-sha1-modp2048,aes128-sha256-modp1024,aes128-sha1-modp1024,aes256-sha384-modp1536,aes256-sha256-modp1536,aes256-sha1-modp1536,aes256-sha384-modp1024,aes256-sha256-modp1024,aes256-sha1-modp1024!
        esp=aes128gcm16-ecp256,aes256gcm16-ecp384,aes128-sha256-ecp256,aes256-sha384-ecp384,aes128-sha256-modp2048,aes128-sha1-modp2048,aes256-sha384-modp4096,aes256-sha256-modp4096,aes256-sha1-modp4096,aes128-sha256-modp1536,aes128-sha1-modp1536,aes256-sha384-modp2048,aes256-sha256-modp2048,aes256-sha1-modp2048,aes128-sha256-modp1024,aes128-sha1-modp1024,aes256-sha384-modp1536,aes256-sha256-modp1536,aes256-sha1-modp1536,aes256-sha384-modp1024,aes256-sha256-modp1024,aes256-sha1-modp1024,aes128gcm16,aes256gcm16,aes128-sha256,aes128-sha1,aes256-sha384,aes256-sha256,aes256-sha1!

        left=%any
        #leftid=<IP-address> # disabled because of NAT?
        leftcert=server.cert.pem
        leftsendcert=always
        leftsubnet=0.0.0.0/0

        right=%any
        rightid=%any
        rightauth=eap-mschapv2
        # not DHCP (%dhcp instead of a subnet), useful only for "public" IPs
        rightsourceip=<SUBNET>
        rightdns=8.8.8.8,8.8.4.4
        rightsendcert=never

        eap_identity=%identity
```

Starting:
`sudo systemctl restart strongswan-starter`

Also `sudo systemctl enable strongswan-starter`

Sources:
Strongswan config mainly from: https://linoxide.com/security/install-and-configure-strongswan-vpn-on-ubuntu/
Additional Strongswan security tweaks from: https://wiki.archlinux.org/index.php/StrongSwan
"Firewall" settings: https://wiki.strongswan.org/projects/strongswan/wiki/ForwardingAndSplitTunneling
