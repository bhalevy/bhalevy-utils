#!/bin/bash --norc
#
# mkinitrd
#
# Copyright 2005-2008 Red Hat, Inc.  All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Author(s): Erik Troan <ewt@redhat.com>
#            Elliot Lee <sopwith@cuc.edu>
#            Miguel de Icaza <miguel@nuclecu.unam.mx>
#            Christian 'Dr. Disk' Hechelmann <drdisk@ds9.au.s.shuttle.de>
#            Michael K. Johnson <johnsonm@redhat.com>
#            Pierre Habraken <Pierre.Habraken@ujf-grenoble.fr>
#            Jakub Jelinek <jakub@redhat.com>
#            Carlo Arenas Belon (carenas@chasqui.lared.net.pe>
#            Keith Owens <kaos@ocs.com.au>
#            Bernhard Rosenkraenzer <bero@redhat.com>
#            Matt Wilson <msw@redhat.com>
#            Trond Eivind Glomsrød <teg@redhat.com>
#            Jeremy Katz <katzj@redhat.com>
#            Preston Brown <pbrown@redhat.com>
#            Bill Nottingham <notting@redhat.com>
#            Guillaume Cottenceau <gc@mandrakesoft.com>
#            Peter Jones <pjones@redhat.com>
#

error()
{
    NONL=""
    if [ "$1" == "-n" ]; then
        NONL="-n"
        shift
    fi
    echo $NONL "$@" >&2
}

if [ $UID != 0 ]; then
    error "mkinitrd must be run as root."
    exit 1
fi

export MALLOC_PERTURB_=204

PATH=/sbin:/usr/sbin:/bin:/usr/bin:$PATH
export PATH

. /etc/rc.d/init.d/functions

# Set the umask. For iscsi, the initrd can contain plaintext
# password (chap secret), so only allow read by owner.
umask 077

VERSION=6.0.52

PROBE="yes"
MODULES=""
PREMODS=""
DMDEVS=""
ncryptodevs=0
ncryptoparts=0
ncryptolvs=0
ncryptoraids=0

NET_LIST=""
LD_SO_CONF=/etc/ld.so.conf
LD_SO_CONF_D=/etc/ld.so.conf.d/

[ -e /etc/sysconfig/mkinitrd ] && . /etc/sysconfig/mkinitrd

CONFMODS="$MODULES"
MODULES=""
ARCH=$(uname -m | sed -e 's/s390x/s390/')

compress=1
allowmissing=""
target=""
kernel=""
force=""
verbose=""
img_vers=""
builtins=""
modulefile=/etc/modules.conf
[ "$ARCH" != "s390" ] && withusb=1
rc=0
dynamic=""
nolvm=""
nodmraid=""

IMAGESIZE=8000
PRESCSIMODS=""
fstab="/etc/fstab"

vg_list=""
net_list="$NET_LIST"

vecho()
{
    NONL=""
    if [ "$1" == "-n" ]; then
        NONL="-n"
        shift
    fi
    [ -n "$verbose" ] && echo $NONL "$@"
}

usage () {
    if [ "$1" == "-n" ]; then
        cmd=echo
    else
        cmd=error
    fi

    $cmd "usage: `basename $0` [--version] [--help] [-v] [-f] [--preload <module>]"
    $cmd "       [--force-scsi-probe | --omit-scsi-modules]"
    $cmd "       [--image-version] [--force-raid-probe | --omit-raid-modules]"
    $cmd "       [--with=<module>] [--force-lvm-probe | --omit-lvm-modules]"
    $cmd "       [--builtin=<module>] [--omit-dmraid] [--net-dev=<interface>]"
    $cmd "       [--fstab=<fstab>] [--nocompress] <initrd-image> <kernel-version>"
    $cmd ""
    $cmd "       (ex: `basename $0` /boot/initrd-2.2.5-15.img 2.2.5-15)"

    if [ "$1" == "-n" ]; then
        exit 0
    else
        exit 1
    fi
}

moduledep() {
    vecho -n "Looking for deps of module $1"
    deps=""
    deps=$(modprobe --set-version $kernel --show-depends $1 2>/dev/null| awk '/^insmod / { print gensub(".*/","","g",$2) }' | while read foo ; do [ "${foo%%.ko}" != "$1" ] && echo -n "${foo%%.ko} " ; done)
    [ -n "$deps" ] && vecho ": $deps" || vecho
}

locatemodule() {
    fmPath=$(modprobe --set-version $kernel --show-depends $1 2>/dev/null | awk '/^insmod / { print $2; }' | tail -1)
    if [ -n "$fmPath" -a -f "$fmPath" ]; then
        return 0
    fi
    for modExt in o.gz o ko ; do
        for modDir in /lib/modules/$kernel/updates /lib/modules/$kernel ; do
            if [ -d $modDir ]; then
                fmPath=$(findone $modDir -name $1.$modExt)
                if [ -n "$fmPath" -a -f "$fmPath" ]; then
                    return 0
                fi
            fi
        done
    done
    return 1
}

expandModules() {
    items=$*

    for m in $items ; do
   char=$(echo $m | cut -c1)
   if [ $char = '=' ]; then
       NAME=$(echo $m | cut -c2-)
       if [ "$NAME" = "ata" ]; then
       MODS="$MODS $(cat /lib/modules/$kernel/modules.block |egrep '(ata|ahci)' |sed -e 's/.ko//')"
       else
       MODS="$MODS $(cat /lib/modules/$kernel/modules.$NAME |sed -e 's/.ko//')"
       fi
   else
       MODS="$MODS $m"
   fi
    done
    echo $MODS
}

qpushd() {
    pushd "$1" >/dev/null 2>&1
}

qpopd() {
    popd >/dev/null 2>&1
}

findone() {
    echo nash-find "$@" | /sbin/nash --force --quiet \
        | /bin/awk '{ print $1; exit; }'
}

findall() {
    echo nash-find "$@" | /sbin/nash --force --quiet
}

dm_get_uuid() {
    echo nash-dm get_uuid "$1" | /sbin/nash --force --quiet
}

resolve_device_name() {
    echo nash-resolveDevice "$1" | /sbin/nash --forcequiet
}

RTLD=""
DSO_DEPS=""
get_dso_deps() {
    bin="$1" ; shift
    DSO_DEPS=""

    declare -a FILES
    declare -a NAMES

    LDSO=$(echo nash-showelfinterp $bin | /sbin/nash --forcequiet)
    [ -z "$LDSO" -o "$LDSO" == "$bin" ] && LDSO="$RTLD"
    [ -z "$LDSO" -o "$LDSO" == "$bin" ] && return 1
    [ -z "$RTLD" ] && RTLD="$LDSO"

    # I still hate shell.
    declare -i n=0
    while read NAME I0 FILE ADDR I1 ; do
        [ "$FILE" == "not" ] && FILE="$FILE $ADDR"
        NAMES[$n]="$NAME"
        FILES[$n]="$FILE"
        let n++
    done << EOF
        $(LD_TRACE_PRELINKING=1 LD_WARN= LD_TRACE_LOADED_OBJECTS=1 \
            $LDSO $bin 2>/dev/null)
EOF

    [ ${#FILES[*]} -eq 0 ] && return 1

    # we don't want the name of the binary in the list
    if [ "${FILES[0]}" == "$bin" ]; then
        FILES[0]=""
        NAMES[0]=""
        [ ${#FILES[*]} -eq 1 ] && return 1
    fi

    declare -i n=0
    while [ $n -lt ${#FILES[*]} ]; do
        FILE="${FILES[$n]}"
        if [ "$FILE" == "not found" ]; then
            cat 1>&2 <<EOF
There are missing files on your system.  The dynamic object $bin
requires ${NAMES[$n]} n order to properly function.  mkinitrd cannot continue.
EOF
            exit 1
        fi
        case "$FILE" in
            /lib*)
                TLIBDIR=`echo "$FILE" | sed 's,\(/lib[^/]*\)/.*$,\1,'`
                BASE=`basename "$FILE"`
                # Prefer nosegneg libs over direct segment accesses on i686.
                if [ -f "$TLIBDIR/i686/nosegneg/$BASE" ]; then
                    FILE="$TLIBDIR/i686/nosegneg/$BASE"
                # Otherwise, prefer base libraries rather than their optimized
                # variants.
                elif [ -f "$TLIBDIR/$BASE" ]; then
                    FILE="$TLIBDIR/$BASE"
                fi
                FILES[$n]="$FILE"
                ;;
        esac
        dynamic="yes"
        let n++
    done

    DSO_DEPS="${FILES[@]}"
}

scsi_wait_scan="no"
findmodule() {
    skiperrors=""

    if [ $1 == "--skiperrors" ]; then
        skiperrors=--skiperrors
        shift
    fi

    local modName=$1

    if [ "$modName" = "off" -o "$modName" = "null" ]; then
        return
    fi

    if [ "$modName" != "${modName##-}" ]; then
        skiperrors=--skiperrors
        modName="${modName##-}"
    fi

    case "$MODULES " in
        *"/$modName.ko "*) return ;;
    esac

    if echo $builtins | egrep -q '(^| )'$modName'( |$)' ; then
        vecho "module $modName assumed to be built in"
        return
    fi

    # special cases
    if [ "$modName" = "i2o_block" ]; then
        findmodule i2o_core
        findmodule -i2o_pci
        modName="i2o_block"
    elif [ "$modName" = "ppa" ]; then
        findmodule parport
        findmodule parport_pc
        modName="ppa"
    elif [ "$modName" = "sbp2" ]; then
        findmodule ieee1394
        findmodule ohci1394
        modName="sbp2"
    elif [ "$modName" = "fw-sbp2" ]; then
        findmodule fw-core
        findmodule fw-ohci
        modName="fw-sbp2"
    elif [ "$modName" = "firewire-sbp2" ]; then
        findmodule firewire-core
        findmodule firewire-ohci
        modName="firewire-sbp2"
    elif [ "$modName" = "gfs2" ]; then
        findmodule lock_nolock
        modName="gfs2"
    elif [ "$modName" = "nfs" ]; then
        findmodule sunrpc
        modName="nfs"
    elif [ "$modName" = "usb-storage" -o "$modName" = "ub" ]; then
        usbModName="$modName"
    fi

    if [ -n "$usbModName" \
            -a "$modName" != "uhci-hcd" \
            -a "$modName" != "ohci-hcd" \
            -a "$modName" != "ehci-hcd" ]; then
        withusb=1
        findmodule ehci-hcd
        findmodule ohci-hcd
        findmodule uhci-hcd

        usbModName=""
    fi


    locatemodule $modName

    if [ ! -f "$fmPath" ]; then
        if [ -n "$skiperrors" ]; then
            return
        fi

        # ignore the absence of the scsi modules
        for n in $PRESCSIMODS; do
            if [ "$n" = "$modName" ]; then
                return;
            fi
        done;

        if [ -n "$allowmissing" ]; then
            error "WARNING: No module $modName found for kernel $kernel, continuing anyway"
            return
        fi

        error "No module $modName found for kernel $kernel, aborting."
        exit 1
    fi

    # only need to add each module once
    [[ "$MODULES" =~ "$modName" ]] || MODULES="$MODULES $modName"

    # need to handle prescsimods here -- they need to go _after_ scsi_mod
    if [ "$modName" = "scsi_mod" ]; then
        for n in $PRESCSIMODS ; do
            findmodule $n
        done
        locatemodule scsi_wait_scan
        if [ -n "$fmPath" -a -f "$fmPath" ]; then
            scsi_wait_scan="yes"
        fi
    fi
}

readlink() {
    echo nash-readlink "$1" | /sbin/nash --force --quiet
}

access() {
    echo nash-access "$@" | /sbin/nash --force --quiet
}

indent_chars=""
inst() {
    if [ "$#" != "2" -a "$#" != "3" ];then
        echo "usage: inst <file> <root> [<destination file>]"
        return 1
    fi
    local file="$1" ; shift
    local root="${1%%/}/" ; shift
    local dest="${1##/}" ; shift
    [ -z "$dest" ] && dest="${file##/}"

    local old_indent_chars=${indent_chars}
    indent_chars="${indent_chars}  "
    indent=${indent_chars:2}

    mkdir -p "$root/$(dirname $dest)"

    local RET=0
    local target=""
    [ -L "$file" ] && target=$(readlink "$file")
    if [ -n "$target" -a "$dest" != "$target" ]; then
        if [ -e "$root$dest" ]; then
            #vecho "${indent}$root/$dest already exists"
            RET=0
        else
            vecho "${indent}$file -> $root$dest"
            ln -sf "$target" "$root$dest"

            inst "$target" "$root"
            l=`echo "$x" | sed -n 's,\(/lib[^/]*\)/.*$,\1,p'`
            if [ -n "$l" ]; then
                inst "$x" "$root" "$l"/`basename "$x"`
            else
                inst "$x" "$root"
            fi
            RET=$?
            indent_chars=${old_indent_chars}
            return $RET
        fi
    fi

    local SHEBANG=$(dd if="$file" bs=2 count=1 2>/dev/null)
    if [ "$SHEBANG" == '#!' ]; then
        # We're intentionally not playing the "what did this moron run
        # in his shell script" game.  There's nothing but pain in that.
        local interp=$(head -1 "$file" | sed 's/^#! *//')
        inst "$interp" "$root"
        RET=$?
        indent_chars=${old_indent_chars}
        return $RET
    fi

    if [ -e "$root$dest" ]; then
        #vecho "${indent}$root$dest already exists"
        RET=0
    else
        if [ -n "$target" -a -L "$target" ]; then
            inst "$target" "$root"
            RET=$?
        else
            vecho "${indent}$file -> $root$dest"
            cp -aL "$file" "$root$dest"

            get_dso_deps "$file"
            local DEPS="$DSO_DEPS"
            for x in $DEPS ; do
                TLIBDIR=`echo "$x" | sed 's,\(/lib[^/]*\)/.*$,\1,'`
                BASE=`basename "$x"`
                inst "$x" "$root" "$TLIBDIR/$BASE"
            done
            RET=$?
        fi
    fi
    indent_chars=${old_indent_chars}
    return $RET
}

finddevnoinsys() {
    majmin="$1"
    if [ -n "$majmin" ]; then
        dev=$(for x in /sys/block/* ; do findall $x/ -name dev ; done | while read device ; do \
              echo "$majmin" | cmp -s $device && echo $device ; done)
        if [ -n "$dev" ]; then
            dev=${dev%%/dev}
            dev=${dev%%/}
            echo "$dev"
            return 0
        fi
    fi
    return 1
}

findblockdevinsys() {
    devname=$(resolve_device_name "$1")
    if [[ "$devname" =~ ^/sys/block/ ]]; then
        echo "$devname"
    fi
    # check if it's a dm-crypt device. if so, just return the /dev/mapper path
    if [[ "$devname" =~ ^/dev/mapper/ ]]; then
        type=$(/sbin/dmsetup table $(basename $devname) | awk '{print $3}')
        if [ "$type" == "crypt" ]; then
            echo "$devname"
            return 0
        fi
    fi
    majmin=$(get_numeric_dev dec $devname)
    finddevnoinsys "$majmin"
}

slavestried=""
findstoragedriverinsys () {
    while [ ! -L device ]; do
        for slave in $(ls -d slaves/* 2>/dev/null) ; do
            slavename=${slave##*/}
            case " $slavestried " in
                *" $slavename "*)
                    continue
                    ;;
                *)
                    slavestried="$slavestried $slavename"
                    qpushd $slave
                    findstoragedriverinsys
                    qpopd
                    ;;
            esac
        done
        [ "$PWD" = "/sys" ] && return
        cd ..
    done
    cd $(readlink ./device)
    if is_iscsi $PWD; then
        handleiscsi "$PWD"
        return
    fi
    if echo $PWD | grep -q /virtio-pci/ ; then
        findmodule virtio_pci
    fi
    while [ "$PWD" != "/sys/devices" ]; do
        deps=
        if [ -f modalias ]; then
            moduledep $(cat modalias)
        fi

        [ -z "$deps" -a -L driver/module ] && \
            deps=$(basename $(readlink driver/module))
        for driver in $deps ; do
            findmodule $driver
        done
        cd ..
    done
}

findstoragedriver () {
    for device in $@ ; do
        case " $handleddevices " in
            *" $device "*)
                continue ;;
            *) handleddevices="$handleddevices $device" ;;
        esac
        if [[ "$device" =~ "^md[0-9]+" ]]; then
            vecho "Found RAID component $device"
            handleraid "$device"
            continue
        fi
        vecho "Looking for driver for device $device"
        if [[ "$device" =~ ^(dm-|mapper/) ]]; then
            majmin=$(get_numeric_dev dec "/dev/$device")
            sysfs=$(finddevnoinsys $majmin)
            handledm $(echo "$majmin" |cut -d : -f 1) $(echo "$majmin" |cut -d : -f 2)
        else
            if [[ "$device" =~ ^/sys ]]; then
                device=${device##*/}
            fi
            sysfs=""
            device=$(echo "$device" | sed 's,/,!,g')
            if [ -d /sys/block/$device/ ]; then
                sysfs="/sys/block/$device"
            else
                sysfs=$(for x in /sys/block/* ; do findone -type d $x/ -name $device; done)
            fi
        fi
        [ -z "$sysfs" ] && return
        qpushd $sysfs
        findstoragedriverinsys
        qpopd
    done
}

findnetdriver() {
    for device in $@ ; do
        case " $handleddevices " in
            *" $device "*)
                continue ;;
            *) handleddevices="$handleddevices $device" ;;
        esac
        if [ -f /sys/class/net/$device/device/modalias ]; then
            modalias=$(cat /sys/class/net/$device/device/modalias)
	    moduledep $modalias
	    for driver in $deps ; do
		findmodule $driver
	    done
	elif [ "$(basename $(readlink /sys/class/net/$device/device/bus) 2>/dev/null)" = "xen" ]; then
	    findmodule xennet # FIXME: hack for xennet sucking
	else
	    findmodule $(ethtool -i $device | awk '/^driver:/ { print $2 }')
 	fi
    done
}

iscsi_get_rec_val() {

    # The open-iscsi 742 release changed to using flat files in
    # /var/lib/iscsi.

    result=$(grep "^${2} = " "$1" |  sed -e s'/.* = //')
}

iscsi_set_parameters() {
    path=$1
    vecho setting iscsi parameters

    # Check once before getting explicit values, so we can output a decent
    # error message.
    declare -a iparams=( $(/sbin/iscsiadm -m session 2>/dev/null) )
    if [[ -z "${iparams[*]}" ]]; then
        echo Unable to find iscsi record for $path
        exit 1
    fi
    nit_name=$(grep "^InitiatorName=" /etc/iscsi/initiatorname.iscsi | \
        sed -e "s/^InitiatorName=//")

    tgt_name=${iparams[3]}
    # edit ipaddr:port,portalgrp -> ipaddr,port,portalgrp
    iportal=${iparams[2]/:/,}
    declare -a ipt=( $(echo ${iportal//,/ }) )
    tgt_ipaddr=${ipt[0]}
    tgt_port=${ipt[1]}
    tpgt=${ipt[2]}

    path=/var/lib/iscsi/nodes/${tgt_name}/${tgt_ipaddr},${tgt_port}

    # Note: we get chap secrets (passwords) in plaintext, and also store
    # them in the initrd.

    iscsi_get_rec_val $path "node.session.auth.username"
    chap=${result}
    if [ -n "${chap}" -a "${chap}" != "<empty>" ]; then
        chap="-u ${chap}"
        iscsi_get_rec_val $path "node.session.auth.password" 
        chap_pw="-w ${result}"
    else
	chap=""
    fi

    iscsi_get_rec_val $path "node.session.auth.username_in"
    chap_in=${result}
    if [ -n "${chap_in}" -a "${chap_in}" != "<empty>" ]; then
        chap_in="-U ${chap_in}"
        iscsi_get_rec_val $path "node.session.auth.password_in" 
        chap_in_pw="-W ${result}"
    else
	chap_in=""
    fi
}

emit_iscsi () {
    if [ -n "${iscsi_devs}" ]; then
        inst /sbin/iscsistart "$MNTIMAGE" /bin/iscsistart
        emit "echo Attaching to iSCSI storage"
        for dev in ${iscsi_devs}; do
            iscsi_set_parameters $dev
            # recid is not really used, just use 0 for it
            emit "/bin/iscsistart -t ${tgt_name} -i ${nit_name} \
                -g ${tpgt} -a ${tgt_ipaddr} ${chap} ${chap_pw} \
                ${chap_in} ${chap_in_pw}"
        done
    fi
}

is_iscsi() {
    path=$1
    if echo $path | grep -q "/platform/host[0-9]*/session[0-9]*/target[0-9]*:[0-9]*:[0-9]*/[0-9]*:[0-9]*:[0-9]*:[0-9]*"; then
        return 0
    else 
        return 1
    fi
}

handledm() {
    major=$1
    minor=$2
    while read dmstart dmend dmtype r0 r1 r2 r3 r4 r5 r6 r7 r8 r9 ; do
        case "$dmtype" in
            crypt)
                slavedev=$(finddevnoinsys $r3)
                slavedev=${slavedev##*/}
                cryptsetup isLuks "/dev/$slavedev" 2>/dev/null || continue
                find_base_dm_mods
                findmodule dm-crypt
                for mod in $(echo $r0 | tr ':-' '  ') ; do
                    findmodule --skiperrors $mod
                done
                dmname=$(dmsetup info -j $major -m $minor -c --noheadings -o name)
                # do the device resolution dance to get /dev/mapper/foo
                # since 'lvm lvs' doesn't like dm-X device nodes
                if [[ "$slavedev" =~ ^dm- ]]; then
                    majmin=$(get_numeric_dev dec "/dev/$slavedev")
                    for dmdev in /dev/mapper/* ; do
                        dmnum=$(get_numeric_dev dev $dmdev)
                        if [ $dmnum = $majmin ]; then
                            slavedev=${dmdev#/dev/}
                            break
                        fi
                    done
                fi

                # determine if $slavedev is an LV
                #  if so, add the device to latecryptodevs
                #  if not, add the device to cryptodevs
                local vg=$(lvshow /dev/$slavedev)
                if [ -n "$vg" ]; then
                    eval cryptolv${ncryptolvs}='"'/dev/$slavedev $dmname'"'
                    let ncryptolvs++
                elif grep -q "^$slavedev :" /proc/mdstat ; then
                    eval cryptoraid${ncryptoraids}='"'/dev/$slavedev $dmname'"'
                    let ncryptoraids++
                else
                    eval cryptoparts${ncryptoparts}='"'/dev/$slavedev $dmname'"'
                    let ncryptoparts++
                fi

                let ncryptodevs++
                handlelvordev "/dev/$slavedev"
                ;;
        esac
    done << EOF
        $(dmsetup table -j $major -m $minor 2>/dev/null)
EOF
}

handleiscsi() {
    vecho "Found iscsi component $1"
    findmodule iscsi_tcp

    # We call iscsi_set_parameters once here to figure out what network to
    # use (it sets tgt_ipaddr), and once again to emit iscsi values,
    # not very efficient.
    iscsi_set_parameters $1
    iscsi_devs="$iscsi_devs $1"
    netdev=$(/sbin/ip route get to $tgt_ipaddr | \
        sed 's|.*dev \(.*\).*|\1|g' | awk '{ print $1; exit }')
    addnetdev $netdev
}

handleraid() {
    local start=0

    if [ -n "$noraid" -o ! -f /proc/mdstat ]; then
        return 0
    fi

    levels=$(awk "/^$1[	 ]*:/ { print\$4 }" /proc/mdstat)
    devs=$(gawk "/^$1[	 ]*:/ { print gensub(\"\\\\[[0-9]*\\\\]\",\"\",\"g\",gensub(\"^md.*raid[0-9]*\",\"\",\"1\")) }" /proc/mdstat)

    for level in $levels ; do
        case $level in
        linear)
            findmodule linear
            start=1
            ;;
        multipath)
            findmodule multipath
            start=1
            ;;
        raid[01456] | raid10)
            findmodule $level
            findmodule raid456
            start=1
            ;;
        *)
            error "raid level $level (in /proc/mdstat) not recognized"
            ;;
        esac
    done
    findstoragedriver $devs
    if [ "$start" = 1 ]; then
        raiddevices="$raiddevices $1"
    fi
    return $start
}

lvshow() {
    lvm lvs --ignorelockingfailure --noheadings -o vg_name \
        $1 2>/dev/null | egrep -v '^ *(WARNING:|Volume Groups with)'
}

vgdisplay() {
    lvm vgdisplay --ignorelockingfailure -v $1 2>/dev/null |
        sed -n 's/PV Name//p'
}

dmmods_found="n"
find_base_dm_mods()
{
    [ "$dmmods_found" == "n" ] || return
    findmodule -dm-mod
    
    # DM requires all of these to be there in case someone used the
    # feature.  broken.  (#132001)
    findmodule -dm-mirror
    findmodule -dm-zero
    findmodule -dm-snapshot
    dmmods_found="y"
}

handlelvordev() {
    [ -n "$1" ] || return 0
    local vg=$(lvshow $1)
    if [ -n "$vg" ]; then
        vg=`echo $vg` # strip whitespace
        case " $vg_list " in
        *" $vg "*) ;;
        *)  vg_list="$vg_list $vg"
            for device in $(vgdisplay $vg) ; do
                findstoragedriver ${device##/dev/}
            done
            [ -z "$nolvm" ] && find_base_dm_mods
            ;;
        esac
    else
        findstoragedriver ${1##/dev/}
    fi
}

handlenetdev() {
    local dev=$1

    source /etc/sysconfig/network
    if [ ! -f /etc/sysconfig/network-scripts/ifcfg-$dev ]; then
        error "unable to find network device configuration for $dev"
    else
        source /etc/sysconfig/network-scripts/ifcfg-$dev
    fi

    if [ x"$BOOTPROTO" = x ]; then
        error "bootproto not specified for $dev, assuming DHCP"
        BOOTPROTO=dhcp
    fi

    [ -n "$IPADDR" ] && IPSTR="$IPSTR --ip $IPADDR"
    [ -n "$NETMASK" ] && IPSTR="$IPSTR --netmask $NETMASK"
    [ -n "$GATEWAY" ] && IPSTR="$IPSTR --gateway $GATEWAY"
    [ -n "$ETHTOOL_OPTS" ] && IPSTR="$IPSTR --ethtool \"$ETHTOOL_OPTS\""
    [ -n "$MTU" ] && IPSTR="$IPSTR --mtu $MTU"
    if [ -n "$IPADDR" ]; then
        [ -z "$DOMAIN" ] && DOMAIN=$(awk '/^search / { print gensub("^search ","",1) }' /etc/resolv.conf)
        if [ -z "$DNS1" ]; then
            DNS1=$(awk '/^nameserver / { ORS="" ; if (x > 0) print "," ; printf "%s", $2 ; x = 1}' /etc/resolv.conf)
        fi
    fi
    [ -n "$DOMAIN" ] && IPSTR="$IPSTR --domain \"$DOMAIN\""
    if [ -n "$DNS1" ]; then
        if [ -n "$DNS2" ]; then
            IPSTR="$IPSTR --dns $DNS1,$DNS2"
        else
            IPSTR="$IPSTR --dns $DNS1"
        fi
    fi
    network="network --device $dev --bootproto $BOOTPROTO $IPSTR"
    if [ "$BOOTPROTO" = "dhcp" ]; then
        dhclient_leases_cmd="cp /var/lib/dhclient/dhclient.leases /sysroot/dev/.dhclient-$dev.leases"
        mkdir -p $MNTIMAGE/var/lib/dhclient
    fi
}

addnetdev() {
    vecho "Adding network device $1"
    findnetdriver "$1"
    net_list="$net_list $1"
}

handlenfs() {
    remote=${1%%:*}
    remoteip=$(host $remote | awk '/ address / { print $4 ; exit 0; }')
    # assume, if it didn't resolve, that it's an IP
    [ -z "$remoteip" ] && remoteip=$remote
    netdev=`/sbin/ip route get to $remoteip |sed 's|.*dev \(.*\).*|\1|g' |awk '{ print $1; exit }'`
    addnetdev $netdev
}

savedargs=$*
while [ $# -gt 0 ]; do
    case $1 in
        --fstab*)
            if [ "$1" != "${1##--fstab=}" ]; then
                fstab=${1##--fstab=}
            else
                fstab=$2
                shift
            fi
            ;;

        --with-usb*)
            if [ "$1" != "${1##--with-usb=}" ]; then
                usbmodule=${1##--with-usb=}
            else
                usbmodule="usb-storage"
            fi
            basicmodules="$basicmodules $usbmodule"
            unset usbmodule
            ;;

        --without-usb)
            withusb=0
            ;;

        --with-avail*)
            if [ "$1" != "${1##--with-avail=}" ]; then
                modname=${1##--with-avail=}
            else
                modname=$2
                shift
            fi

            availmodules="$availmodules $modname"
            ;;

        --without*)
            if [ "$1" != "${1##--without=}" ]; then
                modname=${1##--without=}
            else
                modname=$2
                shift
            fi

            excludemodules="$excludemodules $modname"
            ;;

        --with*)
            if [ "$1" != "${1##--with=}" ]; then
                modname=${1##--with=}
            else
                modname=$2
                shift
            fi

            basicmodules="$basicmodules $modname"
            ;;

        --builtin*)
            if [ "$1" != "${1##--builtin=}" ]; then
                modname=${1##--builtin=}
            else
                modname=$2
                shift
            fi
            builtins="$builtins $modname"
            ;;

        --version)
            echo "mkinitrd: version $VERSION"
            exit 0
            ;;

        -v)
            verbose=-v
            ;;

        --nocompress)
            compress=""
            ;;

        --ifneeded)
            # legacy
            ;;

        -f)
            force=1
            ;;
        --preload*)
            if [ "$1" != "${1##--preload=}" ]; then
                modname=${1##--preload=}
            else
                modname=$2
                shift
            fi
            PREMODS="$PREMODS $modname"
            ;;
        --force-scsi-probe)
            forcescsi=1
            ;;
        --omit-scsi-modules)
            PRESCSIMODS=""
            noscsi=1
            ;;
        --force-raid-probe)
            forceraid=1
            ;;
        --omit-raid-modules)
            noraid=1
            ;;
        --force-lvm-probe)
            forcelvm=1
            ;;
        --omit-lvm-modules)
            nolvm=1
            ;;
        --omit-dmraid)
            nodmraid=1
            ;;
        --image-version)
            img_vers=yes
            ;;
        --allow-missing)
            allowmissing=yes
            ;;
        --net-dev*)
            if [ "$1" != "${1##--net-dev=}" ]; then
                net_list="$net_list ${1##--net-dev=}"
            else
                net_list="$net_list $2"
                shift
            fi
            ;;
        --noresume)
            noresume=1
            ;;
	--rootdev*)
            if [ "$1" != "${1##--rootdev=}" ]; then
                rootdev="${1##--rootdev=}"
            else
                rootdev="$2"
                shift
            fi
	    ;;
	--rootfs*)
            if [ "$1" != "${1##--rootfs=}" ]; then
                rootfs="${1##--rootfs=}"
            else
                rootfs="$2"
                shift
            fi
	    ;;
	--rootopts*)
            if [ "$1" != "${1##--rootopts=}" ]; then
                rootopts="${1##--rootopts=}"
            else
                rootopts="$2"
                shift
            fi
	    ;;
	--loopdev*)
            if [ "$1" != "${1##--loopdev=}" ]; then
                loopdev="${1##--loopdev=}"
            else
                loopdev="$2"
                shift
            fi
	    ;;
	--loopfs*)
            if [ "$1" != "${1##--loopfs=}" ]; then
                loopfs="${1##--loopfs=}"
            else
                loopfs="$2"
                shift
            fi
	    ;;
	--loopopts*)
            if [ "$1" != "${1##--loopopts=}" ]; then
                loopopts="${1##--loopopts=}"
            else
                loopopts="$2"
                shift
            fi
	    ;;
	--looppath*)
            if [ "$1" != "${1##--looppath=}" ]; then
                looppath="${1##--looppath=}"
            else
                looppath="$2"
                shift
            fi
	    ;;
        --help)
            usage -n
            ;;
        *)
            if [ -z "$target" ]; then
                target=$1
            elif [ -z "$kernel" ]; then
                kernel=$1
            else
                usage
            fi
            ;;
    esac

    shift
done

if [ -z "$target" -o -z "$kernel" ]; then
    usage
fi

if [ -n "$img_vers" ]; then
    target="$target-$kernel"
fi

if [ -z "$force" -a -f $target ]; then
    error "$target already exists."
    exit 1
fi

if [ -n "$forcescsi" -a -n "$noscsi" ]; then
    error "Can't both force scsi probe and omit scsi modules"
    exit 1
fi

if [ -n "$forceraid" -a -n "$noraid" ]; then
    error "Can't both force raid probe and omit raid modules"
    exit 1
fi

if [ -n "$forcelvm" -a -n "$nolvm" ]; then
    error "Can't both force LVM probe and omit LVM modules"
    exit 1
fi

if [ ! -d /lib/modules/$kernel ]; then
    error 'No modules available for kernel "'${kernel}'".'
    exit 1
fi

if [ "$LIVEOS" = "yes" ]; then
   # this is a live system, we need to fork off to mkliveinitrd for now
   exec /usr/libexec/mkliveinitrd $savedargs
fi

vecho "Creating initramfs"
modulefile=/etc/modprobe.conf

# find a temporary directory which doesn't use tmpfs
if [ -z "$loopfs" ]; then
    TMPDIR="/tmp"
else
    TMPDIR=""
    for t in /tmp /var/tmp /root ${PWD}; do
        if [ ! -d $t ]; then continue; fi
        if ! access -w $t ; then continue; fi
    
        fs=$(df -T $t 2>/dev/null | awk '{line=$1;} END {printf $2;}')
        if [ "$fs" != "tmpfs" ]; then
            TMPDIR=$t
            break
        fi
    done
fi

if [ -z "$TMPDIR" ]; then
    error "no temporary directory could be found."
    exit 1
fi

if [ $TMPDIR = "/root" -o $TMPDIR = "${PWD}" ]; then
    error "WARNING: using $TMPDIR for temporary files"
fi

PREMODS=$(expandModules $PREMODS)
PRESCSIMODS=$(expandModules $PRESCSIMODS)
availmodules=$(expandModules $availmodules)
basicmodules=$(expandModules $basicmodules)

for n in $PREMODS; do
        findmodule $n
done

if [ "$withusb" == "1" ]; then
    findmodule ehci-hcd
    findmodule ohci-hcd
    findmodule uhci-hcd
fi

if [ "x$PROBE" == "xyes" ]; then
    [ -z "$rootfs" ] && rootfs=$(awk '{ if ($1 !~ /^[ \t]*#/ && $2 == "/") { print $3; }}' $fstab)
    [ -z "$rootopts" ] && rootopts=$(awk '{ if ($1 !~ /^[ \t]*#/ && $2 == "/") { print $4; }}' $fstab)
    [ -z "$rootopts" ] && rootopts="defaults"

    # in case the root filesystem is modular
    findmodule -${rootfs}

    [ -z "$rootdev" ] && rootdev=$(awk '/^[ \t]*[^#]/ { if ($2 == "/") { print $1; }}' $fstab)
    # check if it's nfsroot
    physdev=""
    if [ "$rootfs" == "nfs" -a "x$net_list" == "x" ]; then
        handlenfs $rootdev
    else
        # check if it's root by label
        rdev=$rootdev
        if [[ "$rdev" =~ ^(UUID=|LABEL=) ]]; then
            rdev=$(resolve_device_name "$rdev")
        fi
        rootopts=$(echo $rootopts | sed -e 's/^r[ow],//' -e 's/,_netdev//' -e 's/_netdev//' -e 's/,r[ow],$//' -e 's/,r[ow],/,/' -e 's/^r[ow]$/defaults/' -e 's/$/,ro/')
        physdev=$(findblockdevinsys "$rdev")
        physdev=${physdev##*/dev/}
        if [ -n "$physdev" ]; then
            vecho "Found root device $physdev for $rdev"
        else
            physdev="$rdev"
        fi
    fi
    if [ "$rootfs" != "nfs" ]; then
        if [ -n "$physdev" -a "$physdev" != "$rdev" ]; then
            handlelvordev "$physdev"
        fi
        handlelvordev $rdev
    fi

    # find the first swap dev which would get used for swsusp
    swsuspdev=$(awk '/^[ \t]*[^#]/ { if ($3 == "swap") { print $1; exit }}' $fstab)
    if [ -n "$swsuspdev" ]; then
        if [[ "$swsuspdev" =~ ^(UUID=|LABEL=) ]]; then
            swsuspdev=$(resolve_device_name "$swsuspdev")
        fi

        suspdev=$(findblockdevinsys "$swsuspdev")
        suspdev=${suspdev##*/dev/}
        if [ -n "$suspdev" ]; then
	    vecho "Found swsusp device $suspdev for $swsuspdev"
        fi
        if [ -n "$suspdev" -a "$suspdev" != "$swsuspdev" ]; then
            handlelvordev "$suspdev"
        fi
	handlelvordev "$swsuspdev"
    fi
fi

if [ -n "$forcescsi" -o -z "$noscsi" -a "x$PROBE" == "xyes" ]; then
    if [ ! -f $modulefile ]; then
        modulefile=/etc/conf.modules
    fi

    if [ -f $modulefile ]; then
        scsimodules=`grep "alias[[:space:]]\+scsi_hostadapter" $modulefile | grep -v '^[ 	]*#' | LC_ALL=C sort -u | awk '{ print $3 }'`

        if [ -n "$scsimodules" ]; then
            for n in $scsimodules; do
    # for now allow scsi modules to come from anywhere.  There are some
    # RAID controllers with drivers in block/
                findmodule $n
            done
        fi
    fi
fi

# If we have dasd devices, include the necessary modules (S/390)
if [ "x$PROBE" == "xyes" -a -d /proc/dasd ]; then
    findmodule -dasd_mod
    findmodule -dasd_eckd_mod
    findmodule -dasd_fba_mod
fi

# Loopback root support
# loopdev : device or nfs server:path file is on
# looppath : filename
# loopfs : filesystem of loopdev
# loopots : options to mount loopfs

if [ -n "${loopfs}" ] || [[ "$rootopts" =~ "loop" ]]; then
    	# FIXME: probe this somehow?
	
	rootdev=/dev/loop0
	
	[ -z "$rootopts" ] && rootopts="defaults"
	
	findmodule loop
	findmodule -${loopfs}
	
	if [ "$loopfs" == "nfs" -a "x$net_list" == "x" ]; then
	    handlenfs $loopdev
	fi
	# FIXME: label support
	
	if [ "$loopfs" != "nfs" ]; then
            handlelvordev $loopdev
        fi
fi

# If we use LVM or dm-based raid, include dm-mod
# XXX: dm not really supported yet.
testdm=""
[ -z "$nodmraid" ] && testdm="yes"
[ "x$PROBE" != "xyes" ] && testdm=""

if [ -n "$testdm" \
        -a -x /sbin/dmsetup \
        -a -x /sbin/dmraid \
        -a -e /dev/mapper/control ]; then
    dmout=$(/sbin/dmsetup ls 2>/dev/null)
    if [ "$dmout" != "No devices found" -a "$dmout" != "" ]; then
        RAIDS=$(/sbin/dmraid -s -craidname 2>/dev/null | grep -vi "no raid disks") 
    
        # I fucking hate shell. 
        lineno=1
        PREV=""
        LINE=""
        while :; do
            PREV="$LINE"
            LINE=$(/sbin/dmsetup table | head -$lineno | tail -1)
            if [ "$LINE" == "$PREV" ]; then
                break;
            fi
    
            eval $(echo $LINE | \
                while read NAME START END TYPE TABLE ; do
                    echo NAME=\"$(sed 's/:$//'<<< "$NAME")\"
                    echo START=\"$START\"
                    echo END=\"$END\"
                    echo TYPE=\"$TYPE\"
                    echo TABLE=\"$TABLE\"
                done)
    
            case "$TYPE" in
                multipath|emc)
                    # ugggh.  We could try to fish the module name out, but it
                    # requires real parsing... 
                    # XXX also covered by #132001
                    for mod in $TABLE ; do 
                        DMMODS="$DMMODS $([[ "$mod" =~ "[[:alpha:]]" ]] && echo "$mod")"
                    done
                    DMDEVS="$DMDEVS $NAME"
                    ;;
                *)
                    for raid in $RAIDS ; do
                        if [ "$raid" == "$NAME" ]; then
                            dmname=$(resolve_dm_name $NAME)
                            DMDEVS="$DMDEVS $dmname"
                            RAIDS=$(sed 's/ $NAME //' <<< "$RAIDS")
                            break
                        fi
                    done
                    ;;
            esac
            lineno=$(($lineno + 1))
        done

        [ -n "$DMDEVS" ] && find_base_dm_mods
    
        DMDEVS=$(tr ' ' '\n' <<< $DMDEVS | sort -u)
        for mod in $(tr ' ' '\n' <<< $DMMODS | sort -u) ; do
            findmodule -dm-$mod
        done
    fi
fi

for n in $basicmodules; do
    findmodule $n
done

for n in $CONFMODS; do
    findmodule $n
done

vecho "Using modules: $MODULES"

MNTIMAGE=`mktemp -d ${TMPDIR}/initrd.XXXXXX`
IMAGE=`mktemp ${TMPDIR}/initrd.img.XXXXXX`
RCFILE=$MNTIMAGE/init

cemit()
{
    cat >> $RCFILE
}

emit()
{
    NONL=""
    if [ "$1" == "-n" ]; then
        NONL="-n"
        shift
    fi
    echo $NONL "$@" >> $RCFILE
}

emitdm()
{
    vecho "Adding dm map \"$1\""
    UUID=$(dm_get_uuid "$1")
    if [ -n "$UUID" ]; then
        UUID="--uuid $UUID"
    fi
    emit dm create "$1" $UUID $(/sbin/dmsetup table "$1")
}

emitdms()
{
    [ -z "$DMDEVS" ] && return 0
    echo dm list $DMDEVS | nash --force --quiet | while read ACTION NAME ; do
        case $ACTION in
        rmparts)
            emit rmparts "$NAME"
            ;;
        create)
            emitdm "$NAME"
            ;;
        part)
            emit dm partadd "$NAME"
            ;;
        esac
    done
}

if [ -z "$MNTIMAGE" -o -z "$IMAGE" ]; then
    error "Error creating temporaries.  Try again"
    exit 1
fi

mkdir -p $MNTIMAGE
mkdir -p $MNTIMAGE/firmware
mkdir -p $MNTIMAGE/lib/modules/$kernel
mkdir -p $MNTIMAGE/bin
mkdir -p $MNTIMAGE/etc
mkdir -p $MNTIMAGE/dev
mkdir -p $MNTIMAGE/lib
mkdir -p $MNTIMAGE/proc
mkdir -p $MNTIMAGE/sys
mkdir -p $MNTIMAGE/sysroot
ln -s bin $MNTIMAGE/sbin

vecho "Building initrd in $MNTIMAGE"
inst /sbin/nash "$MNTIMAGE" /bin/nash
inst /sbin/modprobe "$MNTIMAGE" /bin/modprobe
inst /sbin/rmmod "$MNTIMAGE" /bin/rmmod

if [ -e /etc/fstab.sys ]; then
    inst /etc/fstab.sys "$MNTIMAGE"
fi

installmodule()
{
    MODULE=$1
    fmPath=""
    locatemodule $MODULE
    MODULE=$fmPath
    if [ -z "$MODULE" ]; then
        return
    fi
    if [ -x /usr/bin/strip ]; then
        /usr/bin/strip -g $verbose $MODULE -o $MNTIMAGE/lib/modules/$kernel/$(basename $MODULE)
    else
        inst "$MODULE" "$MNTIMAGE" "/lib/modules/$kernel/$(basename $MODULE)"
    fi
    for fw in $(/sbin/modinfo -F firmware $MODULE 2>/dev/null); do
        if [ -f /lib/firmware/$fw ]; then
            inst "/lib/firmware/$fw" "$MNTIMAGE" "/firmware/$fw"
        fi
    done
}

# This loops to make sure it resolves dependencies of dependencies of...
resdeps () {
    items="$*"

    before=1
    after=2

    while [ $before != $after ]; do
    before=`echo $items | wc -c`
    list=""

    for i in $items; do
       deps=""
       moduledep $i
       list="$list $deps"

       # need to handle prescsimods here -- they need to go _after_ scsi_mod
       if [ "$i" = "scsi_mod" ]; then
	list="$list $PRESCSIMODS"
	MODULES="$MODULES $PRESCSIMODS"
	PRESCSIMODS=""

        locatemodule scsi_wait_scan
        if [ -n "$fmPath" -a -f "$fmPath" ]; then
            scsi_wait_scan="yes"
        fi
    fi

    done
    items=$(for n in $items $list; do echo $n; done | sort -u)
    after=`echo $items | wc -c`
    done

    resolved=$items
}

# exclude specific modules
# (must be done before resolution of deps)
excludemods() {
    items="$*"
    output=""
    for i in $items; do
        for x in $excludemodules; do
            if [ "$i" = "$x" ]; then
                continue 2
            fi
        done
        output="$output $i"
    done
    echo $output
}

if [ -n "$excludemodules" ]; then
    MODULES=$(excludemods $MODULES)
    availmodules=$(excludemods $availmodules)
fi
resdeps $MODULES $availmodules
for MODULE in $resolved; do
    installmodule $MODULE
done

# mknod'ing the devices instead of copying them works both with and
# without devfs...
mkdir $MNTIMAGE/dev/mapper

mknod $MNTIMAGE/dev/ram0 b 1 0
mknod $MNTIMAGE/dev/ram1 b 1 1
ln -sf ram1 $MNTIMAGE/dev/ram

mknod $MNTIMAGE/dev/null c 1 3
mknod $MNTIMAGE/dev/zero c 1 5
mknod $MNTIMAGE/dev/systty c 4 0
if ! echo "$(uname -m)" | grep -q "s390"; then 
  for i in 0 1 2 3 4 5 6 7 8 9 10 11 12 ; do
    mknod $MNTIMAGE/dev/tty$i c 4 $i
  done
fi
for i in 0 1 2 3 ; do
    mknod $MNTIMAGE/dev/ttyS$i c 4 $(($i + 64))
done
mknod $MNTIMAGE/dev/tty c 5 0
mknod $MNTIMAGE/dev/console c 5 1
mknod $MNTIMAGE/dev/ptmx c 5 2

if [ -n "$raiddevices" ]; then
    inst /sbin/mdadm "$MNTIMAGE"
    if [ -f /etc/mdadm.conf ]; then
        inst /etc/mdadm.conf "$MNTIMAGE"
    fi
fi

# FIXME -- this can really go poorly with clvm or duplicate vg names.
# nash should do lvm probing for us and write its own configs.
if [ -z "$nolvm" -a -n "$vg_list" ]; then
    inst /sbin/lvm "$MNTIMAGE" /bin/lvm
    if [ -f /etc/lvm/lvm.conf ]; then
        cp $verbose --parents /etc/lvm/lvm.conf $MNTIMAGE/
    fi
fi

findkeymap () {
    local MAP=$1

    if [ ! -f "$MAP" ]; then
        MAP=$(find /lib/kbd/keymaps -type f -name $MAP -o -name $MAP.\* | head -n1)
    fi

    case " $KEYMAPS " in
        *" $MAP "*)
            return
    esac

    KEYMAPS="$KEYMAPS $MAP"

    case $MAP in
        *.gz)
            cmd=zgrep
            ;;
        *.bz2)
            cmd=bzgrep
            ;;
        *)
            cmd=grep
            ;;
    esac

    for INCL in $($cmd "^include " $MAP | cut -d' ' -f2 | tr -d '"'); do
        for FN in $(find /lib/kbd/keymaps -type f -name $INCL\*); do
            findkeymap $FN
        done
    done
}

if [ $ncryptodevs -ne 0 ]; then
    inst /sbin/cryptsetup "$MNTIMAGE"

    KEYTABLE=
    KEYMAP=
    if [ -f /etc/sysconfig/console/default.kmap ]; then
        KEYMAP=/etc/sysconfig/console/default.kmap
    else
        if [ -f /etc/sysconfig/keyboard ]; then
            . /etc/sysconfig/keyboard
        fi
        if [ -n "$KEYTABLE" -a -d "/lib/kbd/keymaps" ]; then
            KEYMAP="$KEYTABLE.map"
        fi
    fi

    if [ -n "$KEYMAP" ]; then
        LOADKEYS=loadkeys
        if [ -f /etc/sysconfig/i18n ]; then
            . /etc/sysconfig/i18n
        fi
        if [ "${LANG}" != "${LANG%%.UTF-8}" -o "${LANG}" != "${LANG%%.utf8}" ]; then
            LOADKEYS="loadkeys -u"
        fi

        inst /bin/loadkeys "$MNTIMAGE"
        findkeymap $KEYMAP

        for FN in $KEYMAPS; do
            inst $FN "$MNTIMAGE"
            case "$FN" in
                *.gz)
                    gzip -d "$MNTIMAGE$FN"
                    ;;
                *.bz2)
                    bzip2 -d "$MNTIMAGE$FN"
                    ;;
            esac
        done
    fi
fi

echo -n >| $RCFILE
cemit << EOF
#!/bin/nash

mount -t proc /proc /proc
setquiet
echo Mounting proc filesystem
echo Mounting sysfs filesystem
mount -t sysfs /sys /sys
echo Creating /dev
mount -o mode=0755 -t tmpfs /dev /dev
mkdir /dev/pts
mount -t devpts -o gid=5,mode=620 /dev/pts /dev/pts
mkdir /dev/shm
mkdir /dev/mapper
echo Creating initial device nodes
mknod /dev/null c 1 3
mknod /dev/zero c 1 5
mknod /dev/systty c 4 0
mknod /dev/tty c 5 0
mknod /dev/console c 5 1
mknod /dev/ptmx c 5 2
EOF

# XXX really we need to openvt too, in case someting changes the
# color palette and then changes vts on fbcon before gettys start.
# (yay, fbcon bugs!)
if ! echo "$(uname -m)" | grep -q "s390"; then 
    for i in 0 1 2 3 4 5 6 7 8 9 10 11 12 ; do
        emit "mknod /dev/tty$i c 4 $i"
    done
fi

for i in 0 1 2 3 ; do
    emit "mknod /dev/ttyS$i c 4 $(($i + 64))"
done

emit "echo Setting up hotplug."
emit "hotplug"

emit "echo Creating block device nodes."
emit "mkblkdevs"

if [ "$scsi_wait_scan" == "yes" ]; then
    vecho "Adding module scsi_wait_scan"
    installmodule scsi_wait_scan
fi

/sbin/depmod -a -b $MNTIMAGE $kernel
if [ $? -ne 0 ]; then
    error "\"/sbin/depmod -a $kernel\" failed."
    exit 1
fi

usb_mounted="prep"
for MODULE in $MODULES; do
    text=""
    module=`echo $MODULE | sed -e "s|.*/||" -e "s/\.k\?o$//"`
    fullmodule=`echo $MODULE | sed "s|.*/||"`

    options=`sed -n -e ':a' -e '/\\\\$/N; s/\\\\\n//; ta' -e "s/^options[ 	][ 	]*$module[ 	][ 	]*//p" $modulefile 2>/dev/null`
    
    if [ -n "$options" ]; then
        vecho "Adding module $module$text with options $options"
    else
        vecho "Adding module $module$text"
    fi

    # we mount usbfs before the first module *after* the HCDs
    if [ "$usb_mounted" == "prep" ]; then
        if [[ "$module" =~ ".hci[_-]hcd" ]]; then
            usb_mounted="no"
        fi
    elif [ "$usb_mounted" == "no" ]; then
        if [[ ! "$module" =~ ".hci[_-]hcd" ]]; then
            usb_mounted=yes
            emit "mount -t usbfs /proc/bus/usb /proc/bus/usb"
        fi
    fi

    if [ -n "$options" ]; then
        echo "options $module $options" >> $MNTIMAGE/etc/modprobe.conf
    fi

    emit "echo \"Loading $module module\""
    emit "modprobe -q $module"
        
    # Hack - we need a delay after loading usb-storage to give things
    #        time to settle down before we start looking a block devices
    if [ "$module" = "usb-storage" -o "$module" = "ub" ]; then
        emit "echo Waiting for driver initialization."
        emit "stabilized /proc/bus/usb/devices"
    fi
    # Firewire likes to change the subsystem name every 3 hours. :/
    if [ "$module" = "sbp2" ]; then
        emit "echo Waiting for driver initialization."
        emit "stabilized /sys/bus/ieee1394/drivers/sbp2"
    fi
    if [ "$module" = "fw-sbp2" -o "$module" = "firewire-sbp2" ]; then
        emit "echo Waiting for driver initialization."
        emit "stabilized /sys/bus/firewire/drivers/sbp2"
    fi
    if [ "$module" = "zfcp" -a -f /etc/zfcp.conf ]; then
        emit "echo Waiting 2 seconds for driver initialization."
        emit "sleep 2"
        cat /etc/zfcp.conf | grep -v "^#" | tr "A-Z" "a-z" | while read DEVICE TWO THREE FOUR FIVE; do
	    if [ -z "$FIVE" ]; then
		WWPN=$TWO
		FCPLUN=$THREE
	    else
		WWPN=$THREE
		FCPLUN=$FIVE
	    fi
	    cemit <<EOF 
echo -n $WWPN > /sys/bus/ccw/drivers/zfcp/${DEVICE/0x/}/port_add
echo -n $FCPLUN > /sys/bus/ccw/drivers/zfcp/${DEVICE/0x/}/$WWPN/unit_add
echo -n 1 > /sys/bus/ccw/drivers/zfcp/${DEVICE/0x/}/online
EOF
        done
    fi
    if [ "${module::5}" == "pata_" -o "$module" == "ata_piix" -o "$module" == "ahci" -o "${module::5}" == "sata_" -o "$module" == "ibmvscsic" ]; then
        emit "echo Waiting for driver initialization."
        emit "stabilized --hash --interval 250 /proc/scsi/scsi"
    fi
done
unset usb_mounted

if [ -n "$availmodules" ]; then
  cemit <<EOF
echo "Loading available PCI drivers"
loadDrivers
EOF
fi

if [ -z "$nolvm" -a -n "$vg_list" -o $ncryptodevs -ne 0 ]; then
    emit "echo Making device-mapper control node"
    emit "mkdmnod"
fi

if [ -n "$net_list" ]; then
    for netdev in $net_list; do 
        emit "echo Bringing up $netdev"
        handlenetdev $netdev
        emit $network
    done
fi

emit_iscsi

if [ "$scsi_wait_scan" == "yes" ]; then
    emit "modprobe scsi_wait_scan"
    emit "rmmod scsi_wait_scan"
fi

# HACK: module loading + device creation isn't necessarily synchronous...
# this will make sure that we have all of our devices before trying
# things like RAID or LVM
emit "mkblkdevs"

# bhalevy: HACK needed starting 2.6.29 kernel, resolved by newer versions of nash
emit "mkblkdevs"

emitdms

emitcrypto()
{
    emit "echo Setting up disk encryption: $1"
    emit "cryptsetup luksOpen $1 $2"
}

if [ -n "$KEYMAP" ]; then
    emit "echo Loading keymap."
    emit "$LOADKEYS $KEYMAP"
fi

for cryptdev in ${!cryptopart@} ; do
    emitcrypto `eval echo '$'$cryptdev`
done

if [ -n "$raiddevices" ]; then
    for dev in $raiddevices; do
        emit "mdadm -As --auto=yes --run /dev/${dev}"
    done
fi

for cryptdev in ${!cryptoraid@} ; do
    emitcrypto `eval echo '$'$cryptdev`
done

if [ -z "$nolvm" -a -n "$vg_list" ]; then
    emit "echo Scanning logical volumes"
    emit "lvm vgscan --ignorelockingfailure"
    emit "echo Activating logical volumes"
    emit "lvm vgchange -ay --ignorelockingfailure $vg_list"
fi

for cryptdev in ${!cryptolv@} ; do
    emitcrypto `eval echo '$'$cryptdev`
done

if [ -z "$noresume" -a -n "$swsuspdev" ]; then
    emit "resume $swsuspdev"
fi

if [ -n "$loopfs" ]; then
    emit "echo Mounting loop backing store."
    emit "mkdir /tmpmount"
    emit "mount -t $loopfs -o ${loopopts:-defaults} $loopdev /tmpmount"
    emit "echo Creating loop device."
    emit "losetup /dev/loop0 /tmpmount/$looppath"
fi

emit "echo Creating root device."
# mkrootdev does "echo /dev/root /sysroot ext3 defaults,ro 0 0 >/etc/fstab"
emit "mkrootdev -t $rootfs -o $rootopts $rootdev"

emit "echo Mounting root filesystem."
emit "mount /sysroot"

if [ -n "$loopfs" ]; then
     emit "Cleaning up loop mount."
     emit "umount /tmpmount"
fi

emit "echo Setting up other filesystems."
emit "setuproot"

if [ -n "$dhclient_leases_cmd" ]; then
    emit "echo Copying DHCP lease"
    emit "$dhclient_leases_cmd"
fi

emit "loadpolicy"
emit "echo Switching to new root and running init."
emit "switchroot"
emit "echo Booting has failed."
emit "sleep -1"

chmod +x $RCFILE

if [ "$dynamic" == "yes" ]; then
    vecho "This initrd uses dynamic shared objects."
    vecho "Adding dynamic linker configuration files."
    [ -n "$LD_SO_CONF" ] && inst "$LD_SO_CONF" "$MNTIMAGE" /etc/ld.so.conf
    mkdir -p $MNTIMAGE/etc/ld.so.conf.d
    for x in $(find $LD_SO_CONF_D -type f) ; do
        inst "$x" "$MNTIMAGE" "/etc/ld.so.conf.d/$(basename "$x")"
    done

    vecho "Running ldconfig"
    /sbin/ldconfig -r "$MNTIMAGE"
    if [ $? -ne 0 ]; then
        error tmpdir is $MNTIMAGE
        exit 1
    fi
fi

(cd $MNTIMAGE; findall . | cpio -H newc --quiet -o) >| $IMAGE || exit 1

if [ -n "$compress" ]; then
    gzip -9 < $IMAGE >| $target || rc=1
else
    cp -a $IMAGE $target || rc=1
fi
rm -rf $MNTIMAGE $IMAGE
if [ -n "$MNTPOINT" ]; then rm -rf $MNTPOINT ; fi

exit $rc

# vim:ts=8:sw=4:sts=4:et
