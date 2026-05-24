#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u
# set -x

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
XCOMPILE_LIB64=$HOME/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib64
XCOMPILE_LIB=$HOME/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

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

    echo "Cleaning the build environment..."
    make clean
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE mrproper

    echo "Building configurations..."
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE defconfig

    echo "Building the kernel image..."
    make -j4 ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE all

    echo "Building the kernel modles..."
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE modules

    echo "Building the device tree..."
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE dtbs

fi

echo "Adding the Image in outdir..."
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/Image

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# Use an array of directory names so that the list can be easily modified
# This code was developed using the following stack overflow post as a reference:
# https://stackoverflow.com/questions/8880603/loop-through-an-array-of-strings-in-bash
declare -a roodirs=("home"
                    "bin"
                    "etc"
                    "sbin"
                    "var"
                    "lib"
                    "proc"
                    "sys"
                    "tmp"
                    "usr"
                    "usr/sbin"
                    "usr/bin"
                    "var/log"
                    "lib64"
                    "dev"
)
for root_dirname in "${roodirs[@]}"
do
    dir="${OUTDIR}/rootfs/${root_dirname}"
    echo "Creating directory: ${dir}"
    mkdir -p $dir
done

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
else
    cd busybox
fi

echo "Building busybox..."
make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE defconfig
make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE
make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE CONFIG_PREFIX="${OUTDIR}/rootfs" install
chmod u+s ${OUTDIR}/rootfs/bin/busybox

echo "Copying Library dependencies"
mapfile -t lib64s < <(${CROSS_COMPILE}readelf -a "${OUTDIR}/rootfs/bin/busybox" | grep "Shared library" | grep -oP '(?<=\[).*?(?=\])')
lib=`${CROSS_COMPILE}readelf -a "${OUTDIR}/rootfs/bin/busybox" | grep "program interpreter" | grep -oP '(?<=lib/).*\.so.*(?=\])'`
for lib64 in "${lib64s[@]}"
do
    echo "Copying the Lib64 file: ${XCOMPILE_LIB64}/${lib64} to ${OUTDIR}/rootfs/lib64"
    cp ${XCOMPILE_LIB64}/${lib64} ${OUTDIR}/rootfs/lib64
done

echo "Copying the Library file: ${XCOMPILE_LIB}/${lib} to ${OUTDIR}/rootfs/lib"
cp ${XCOMPILE_LIB}/${lib} ${OUTDIR}/rootfs/lib

echo "Creating device nodes..."
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/null c 1 3
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/console c 5 1
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/tty c 5 0

echo "Building the writer utility..."
cd ${FINDER_APP_DIR}
make clean
make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE

echo "Copying finder and writer scripts..."
mkdir -p ${OUTDIR}/rootfs/home/finder-app ${OUTDIR}/rootfs/home/conf
cd ..
cp -R finder-app conf ${OUTDIR}/rootfs/home

echo "Making root the owner of the filesystem root..."
sudo chown -R root:root ${OUTDIR}/rootfs

# TODO: Create initramfs.cpio.gz
echo "Creating the filesystem ramdisk file..."
cd ${OUTDIR}/rootfs
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio.gz
