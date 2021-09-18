# sec-web
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
