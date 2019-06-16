# Configuring the mouse

There is not really much into it, but throwing one or two links here will be useful:

`xinput` from `xorg-xinput` is useful for configuring things XFCE4 GUI cannot...

[Configuring a touchpad](https://wiki.archlinux.org/index.php/Touchpad_Synaptics)

I have some other module, not synaptic, but it was still useful.

This thing is useful for:

 - (manually) enabling/disabling a specific mouse/keyboard (e.g. disabling the one on the laptop, because you have an other already plugged in)
 - configuring the touchpad: what does simple-tap, double-tap, etc. do

### Listing inputs

`xinput list`

The ID's of the slaves will be useful later.

### Listing properties (configuration) of a "module"

`xinput list-props <id>`

### Setting a property

`xinput set-prop <id> <property id> <value>`

---

`man xinput`

## Persistent changes (untested)

Based on [this](https://askubuntu.com/questions/20298/how-to-make-xinput-settings-persist-after-devices-are-unplugged-replugged-and)

### Solution?

So there are some configuration files which are called in some cases. There are 3 places where we want to set these options: when sleep mode ends, when computer starts (but only after X is up) and immediately.

This sets the configuration immediately:

`xinput --set-prop 'Synaptics TM3096-006' 'libinput Disable While Typing Enabled' 0`

---

After writing 1 line here: `/usr/share/X11/xorg.conf.d/40-libinput.conf`, I got it working for restart.

```
Section "InputClass"
        Identifier "libinput touchpad catchall"
        MatchIsTouchpad "on"
        MatchDevicePath "/dev/input/event*"
        Option "DisableWhileTypingEnabled" "0"
        Driver "libinput"
EndSection
    Option "DisableWhileTypingEnabled" "0"
```

So the *TODO* is: after going back from sleep, how can the ~driver~ libinput be configured.
