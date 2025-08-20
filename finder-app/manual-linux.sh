#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

# Ass 3 V01.5

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}


#testtt
USER_NAM=$(whoami)
echo "User -> ${USER_NAM}"

#PATHLIB=/home/ernst/arm-cross-compiler/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib
PATHLIB=/home/ernst/CU/assignments/assignment-3-and-later/libs
FNAM=${PATHLIB}/ld-linux-aarch64.so.1

echo "FNAM=${FNAM}"

if [ -f "${FNAM}" ]
then
    echo "Lib - Datei existiert"
else
    echo "Lib - Datei nicht vorhanden"
fi
 

cd /
echo "Root:"
ls -l /
echo "======="

if [ -d "/home" ]
then
    echo "/home vorhanden:"
    ls -l /home
    echo "--------"
  
else
    echo "/home kein Laufwerk"
fi
if [ -d "/home/ernst" ]
then
   echo "/home/ernst vorhanden"
else
   echo "/home/ernst nicht vorhanden"
fi

if [ -d "/home/ernst/CU" ]
then
   echo "/home/ernst/CU  vorhanden"
else
   echo "/home/ernst/CU nicht vorhanden"
fi

if [ -d "/home/ernst/CU/assignments" ]
then
   echo "/home/ernst/CU/assignments vorhanden" 
else
   echo "/home/ernst/CU/assignments nict  vorhanden" 
fi

if [ -d "/home/ernst/CU/assignments/assignment-3-and-later" ]
then
   echo "/home/ernst/CU/assignments/assignment-3-and-later vorhanden" 
else
   echo "/home/ernst/CU/assignments/assignment-3-and-later nicht vorhanden" 
fi

if [ -d "/home/ernst/CU/assignments/assignment-3-and-later/libs" ]
then
   echo "/home/ernst/CU/assignments/assignment-3-and-later/libs vorhanden" 
else
   echo "/home/ernst/CU/assignments/assignment-3-and-later/libs nicht vorhanden" 
fi

echo "try sudo cp"
sudo cp ${PATHLIB}/ld-linux-aarch64.so.1 ${OUTDIR}/
echo "with sudo fine !!!"
cp ${PATHLIB}/ld-linux-aarch64.so.1 ${OUTDIR}/

#-----------------

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here

    # clean
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper 
    # insert make defconfig
    make ARCH=${ARCH}  CROSS_COMPILE=${CROSS_COMPILE}  defconfig

    # here defconfig used
    # make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} menuconfig
    
    # build
    make -j4  ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all

    # Included in male ... all
    # make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}  modules

    # Included in male ... all
    # make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
   
fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/arm64/boot/Image ${OUTDIR}


echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi


# TODO: Create necessary base directories
mkdir ${OUTDIR}/rootfs
cd ${OUTDIR}/rootfs
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log


cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}

    # TODO:  Configure busybox
    
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} distclean
    #make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} menuconfig
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig      # default configuration

else
    cd busybox
fi


# TODO: Make and install busybox
# -> see Config   make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} distclean
# -> see config   make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} 
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install



echo "Library dependencies"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "Shared library"


# TODO: Add library dependencies to rootfs
cp /home/ernst/arm-cross-compiler/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib/ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib
cp /home/ernst/arm-cross-compiler/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib64/libm.so.6 ${OUTDIR}/rootfs/lib64
cp /home/ernst/arm-cross-compiler/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib64/libresolv.so.2 ${OUTDIR}/rootfs/lib64
cp /home/ernst/arm-cross-compiler/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib64/libc.so.6 ${OUTDIR}/rootfs/lib64


# TODO: Make device nodes 
cd ${OUTDIR}/rootfs/dev
sudo mknod -m 622 console c 5 1
sudo mknod -m 666 null c 1 3


# TODO: Clean and build the writer utility
cd /home/ernst/CU/assignments/assignment-3-and-later/assignment-3-and-later-woernoe/finder-app


make clean
make CROSS_COMPILE=${CROSS_COMPILE} all


# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp /home/ernst/CU/assignments/assignment-3-and-later/assignment-3-and-later-woernoe/finder-app/writer ${OUTDIR}/rootfs/home
mkdir -p ${OUTDIR}/rootfs/home/conf
cp /home/ernst/CU/assignments/assignment-3-and-later/assignment-3-and-later-woernoe/finder-app/conf/assignment.txt ${OUTDIR}/rootfs/home/conf
cp /home/ernst/CU/assignments/assignment-3-and-later/assignment-3-and-later-woernoe/finder-app/conf/username.txt ${OUTDIR}/rootfs/home/conf
cp /home/ernst/CU/assignments/assignment-3-and-later/assignment-3-and-later-woernoe/finder-app/finder.sh ${OUTDIR}/rootfs/home
cp /home/ernst/CU/assignments/assignment-3-and-later/assignment-3-and-later-woernoe/finder-app/finder-test.sh ${OUTDIR}/rootfs/home
cp /home/ernst/CU/assignments/assignment-3-and-later/assignment-3-and-later-woernoe/finder-app/autorun-qemu.sh ${OUTDIR}/rootfs/home


# TODO: Chown the root directory
cd ${OUTDIR}/rootfs
sudo chown -R root:root .


# TODO: Create initramfs.cpio.gz
#find . -print0 | cpio --null -ov --format=newc | gzip -n > ../initramfs.cpio.gz
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
cd ${OUTDIR}
gzip -f initramfs.cpio



