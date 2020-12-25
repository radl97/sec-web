# Setting up a VPN server and clients

There were 4 main goals with the choice of which VPN I used:
- No additional software for Windows
- VPN can be set as default gateway
- Support both UDP and TCP
- Support for communication between clients
- (Additionally, be secure please)
- Server is available for Linux


What I've found is:
- Windows supports 4 different VPN types: PPTP, something, something and IKEv2
- PPTP is inherently insecure
- IKEv2 (IPSec) is supported by Strongswan for Linux (both for client and server).

This decided the choice
