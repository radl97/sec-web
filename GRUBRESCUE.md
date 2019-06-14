# fixing windows install Breaking the partition system Bad

I installed a Windows system to an empty partition. I had UEFI + unused linux + linux + swap + some other partitionbefore, installing over the unused linux.

It is common knowledge that windows 10 creates 2 partitions in the selected one's place. This messes up some things in the Linux system.

I used the broken grub and broken linux without any pendrive or CD as it wasn't avaliable.

Eventually I fixed it.

## Installing windows

Could not find any settings to have only one partition or anything else, **chose the simple way**: regular install, deleted the unused partition and placed it there.

## Running the broken Linux system

First (I think) I had to overwrite the default windows boot in the ROM (UEFI or whatsoever) settings. Trying to boot, the grub did not find its modules because it shifted by one.

**Note:** I had a broken grub because 1) the grub's configured partition shifted because of the install. (this causes grub rescue showing up) 2) system changed, windows instead of the old linux (this causes some grub options to break).


## Rescue usage

`grub rescue>`

### Determining the place of the grub directory

`ls` helps list all the partitions.

You can then `ls (hd0,gpt1)/` to list what files are in the partition. Grub rescue only knows read-only ext2 (and ext3 and ext4). Of course the configuration files (`/boot/grub/*`) are on ext2 system.

I knew where to find it, but trying `ls <partition>/boot/grub` in some of the partitions help find it.

### Loading the first module

We have to tell grub where it's config files (and modules) are.

```
prefix=(hd1,gpt5)/boot/grub
root=(hd1,gpt5)
```

Then we can get to normal mode:

```
insmod normal
normal
```

This (for me) showed the grub with the broken settings.

## Booting the linux in rescue-mode

As the system is still broken, but grub cannot really edit files, I wanted to boot the linux into some rescue-mode. Maybe `init 1` would work, but I used another method:

First, you have to set the root to the linux system to be booted:

```
root=(hd1,gpt5)
```

(For me it was in the same partition as grub, so I could skip this)

Then, tell Linux that 1) I want a rescue, 2) the system is not where it is supposed to be:

```
linux /boot/vmlinuz<tab><tab> root=/dev/sda5 init=/bin/bash
```

We could use an rw flag, more on that later.

Setting root tells that you can find the linux partition in sda5. Init tells that run bash from the system.

**Note:** I think it did not work without explicitly specifying root here too

We could add rw, which means mount it read-write. This has a quirk:

- without the rw, the system is mounted at /
- if I use rw:
  - the system will mounted be at /something (forgot what it was, maybe /system)
  - you can chroot /something to call some inner programs.

**Note:** I had an error message stating that it couldn't find the new root, probably that is why this happened...

So if you need the system read-only, then not using rw helps a bit.

I forgot this at the time of usage, but we can remount the partition to be read-write:

```
mount -o remount,rw /
```

## Fixing everything

There are two things broken (if you do not hack too much in the system): `/etc/fstab` and grub, which specifies default mounts (which are always mounted when the system starts). We have to rewrite it if it uses `/dev/sd*` format instead of the `UUID=...` one. Shift the values by one after the windows partition, and delete the windows' partition's entry (or rewrite it to use ntfs-3g, or something else you want to do with it in `/etc/fstab`).

Fixing grub is just reinstalling the system:

After mounting the UEFI partition:

```
grub-install --root-directory=/ --boot-directory=/boot/grub --efi-directory=/efi-mount /dev/sda
```

Root directory and boot directory (maybe one of them is not even needed) tell Grub where to install the grub config and module files in the mounted file systems.

After mounting some other partition, one could install grub to an other partition with

`grub-install --root-directory=/other-mountpoint --boot-directory=/other-mountpoint/boot/grub --efi-directory=/efi-mount /dev/sda`

After that, restarting with Ctrl+Alt+Delete fixed every problem I had. Maybe I should delete the UEFI option of the old, overwritten system...

---

**Note:** A not so successful ubuntu installation tried to install i386-pc version of GRUB in the MBR (so very far away from UEFI with `x86_64`...). Fixed it with an additional option: `--target x86_64-efi`.
It needed an additional package installed with `apt install grub-efi-amd64-signed`.

