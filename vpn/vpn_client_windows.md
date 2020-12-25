# VPN client for Windows (10)

TODO If you need this in english, feel free to ping me :D I don't think it is that useful...

On a linux box, convert PEM to DER
`$ openssl x509 -outform der -in ca.cert.pem -out ca.cert.cer`

- Fájl letöltés
- Fájlra duplakatt
- install for local machine
  - specify certificate store -> personal # nem biztos h kell
- Fájlra megint duplakatt
- install for local machine
  - specify certificate store -> Trusted root CA
- Új VPN létrehozása
- VPN adatok:
  - VPN Provider: Windows built-in
  - Név: VPN client
  - Host: `<IP-address>`
  - VPN típusa: IKEv2
  - (Usernevet/jelszót, hasonló beállítást még ne adj meg)
  - Remember sign-in info

A password kézzel van generálva/megadva, ezt kell csatlakozáskor beírni. Csatlakozás után ipconfig kiírja a `<SUBNET>` subnetet.

VPN beállítása default-nak:

- VPN -> Change adapter options
- Jobb gomb a VPN-re -> Properties
- Networking tab -> IPv4 settings -> Properties -> Advanced
- `Use default gateway on remote network` bepipálása -> OK
- (Test: What's my IP -> `<IP-address>`)
