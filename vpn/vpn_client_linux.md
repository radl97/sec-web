Client VPN config:

Delete all firewall rules :'(
Most probably Forward accept is not needed at all
Probably a full regular setup works too (as everything is over IP)

```C
$ sudo iptables -S
-P INPUT ACCEPT
-P FORWARD ACCEPT
-P OUTPUT ACCEPT
```

Download ca.cert.pem

```
$ cat /etc/ipsec.conf 
# ipsec.conf - strongSwan IPsec configuration file

# basic configuration

config setup
        #charondebug="ike 2, knl 2, cfg 2, net 2, esp 2, dmn 2, mgr 2"

conn vpn-client
        auto=start
        right=<IP-address>
        # This was needed for connection (probably because of NAT)
        rightid=%any
        cacerts=ca.cert.pem
        rightsubnet=0.0.0.0/0
        rightauth=pubkey

        dpdaction=restart
        dpddelay=30
        dpdtimeout=90
        fragmentation=yes

        leftsourceip=%config
        keyexchange=ikev2
        leftauth=eap-mschapv2
        leftid=laci
        eap_identity=%identity
```

```
$ sudo cat /etc/ipsec.secrets 
# ipsec.secrets - strongSwan IPsec secrets file

user1 : EAP "pw1"
```

`$ sudo systemctl restart strongswan-starter`
`$ sudo ipsec up/down vpn-client`
