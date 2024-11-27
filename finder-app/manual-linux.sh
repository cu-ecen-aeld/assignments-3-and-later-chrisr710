#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u
full_run=1


OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
SYSROOT_DIR="/home/school/cross_compile_bin/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu"
START_DIR="$(pwd)"
if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

#if [ $full_run == 1 ]; then
mkdir -p ${OUTDIR}
#sudo chmod 777 ${OUTDIR}
cd "$OUTDIR"
echo "moved to outdir"
pwd
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd ${OUTDIR}/linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}
    echo "making the kernel"
    #cd ./arch/${ARCH} || echo "cannot find the arch dir!"
    echo "runing mrproper to clean..."
    make mrproper  
    echo "making defconfig"
    make ARCH=$ARCH CROSS_COMPILE=${CROSS_COMPILE} defconfig
    echo "making arch"
    make -j4 ARCH=$ARCH CROSS_COMPILE=${CROSS_COMPILE} all
    #echo "making modules..."
    #make modules arch=$ARCH CROSS_COMPILE=${CROSS_COMPILE}
    echo "making dev tree"
    make ARCH=$ARCH CROSS_COMPILE=${CROSS_COMPILE} dtbs
    echo "completed building kernel"
    # TODO: Add your kernel build steps here
fi
echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}
echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
    echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
cd "$OUTDIR"
list="bin,dev,etc,home,lib,lib64,proc,sbin,sys,tmp,usr,var,./usr/bin,./usr/lib,./usr/sbin,./var/log"
IFS=','
for dir in $list
do
[ -d ./rootfs/$dir ] || mkdir -p ./rootfs/$dir  
done

if [ ! -d "${OUTDIR}/busybox" ]
then

git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
else
    cd busybox
fi
make distclean
make defconfig
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install
# TODO: Make and install busybox
# TODO: Add library dependencies to rootfs
echo "Library dependencies"
dep_names="libm.so.6,libresolv.so.2,libc.so.6"
lib_dep_names="ld-linux-aarch64.so.1"
#${CROSS_COMPILE}readelf -a "${OUTDIR}/linux-stable/rootfs/"bin/busybox | grep "program interpreter"
#${CROSS_COMPILE}readelf -a "${OUTDIR}/linux-stable/rootfs/"bin/busybox | grep "Shared library"
IFS=','
for name in $dep_names
do
	f="$(find ${SYSROOT_DIR} -name ${name})" || echo "COULD NOT FIND ${name}"
	echo "f=$f"
	cp $f "${OUTDIR}/rootfs/lib64/" || echo "ERROR in copying $f to ${OUTDIR}/rootfs/lib64"

done

for name in $lib_dep_names
do
        f="$(find ${SYSROOT_DIR} -name ${name})" || echo "COULD NOT FIND ${name}"
        echo "f=$f"
        cp $f "${OUTDIR}/rootfs/lib/" || echo "ERROR in copying $f to ${OUTDIR}/rootfs/lib"

done

# TODO: Make device nodes
echo "making device nodes..."
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/null c 1 3
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/console c 5 1

# TODO: Clean and build the writer utility
pushd "/home/school/school-repository/finder-app" 
echo "cleaning writer util"
make clean || echo "nothing to clean"
make CROSS_COMPILE=aarch64-none-linux-gnu-
mkdir -p ${OUTDIR}/rootfs/home/
sudo chmod 755 ${OUTDIR}/rootfs/home/writer || echo "writer does not exist yet in home on target."
cp writer ${OUTDIR}/rootfs/home/
echo "HERE IS CHMOD"
sudo chmod 755 ${OUTDIR}/rootfs/home/writer
echo "HERE IS RESULT:""$(ls -ll ${OUTDIR}/rootfs/home/writer)"
popd
# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
mkdir -p ${OUTDIR}/rootfs/home/conf
#sudo chmod -R 755 ${OUTDIR}/rootfs
echo "done full run section"
#fi #full_run
echo "MY PWD IS "$(pwd)""
pushd $FINDER_APP_DIR
#echo "/bin/sh" > ${OUTDIR}/rootfs/init
#chmod 755 ${OUTDIR}/rootfs/init
mkdir -p ${OUTDIR}/rootfs/home/conf
cp ./finder.sh ${OUTDIR}/rootfs/home/
cp ./autorun-qemu.sh ${OUTDIR}/rootfs/home/
chmod +x ${OUTDIR}/rootfs/home/autorun-qemu.sh
sudo chmod +x ${OUTDIR}/rootfs/home/finder.sh
sed -i 's;/bin/bash;/bin/sh;' ${OUTDIR}/rootfs/home/finder.sh
cp ./conf/username.txt ${OUTDIR}/rootfs/home/conf/
cp ./conf/assignment.txt ${OUTDIR}/rootfs/home/conf
cp ./finder-test.sh  ${OUTDIR}/rootfs/home/
sed -i 's;/bin/bash;/bin/sh;g' ${OUTDIR}/rootfs/home/finder-test.sh
sudo chmod +x ${OUTDIR}/rootfs/home/finder-test.sh
sed -i 's;../conf/;conf/;g' ${OUTDIR}/rootfs/home/finder-test.sh
sed -i 's;../conf/;conf/;g' ${OUTDIR}/rootfs/home/finder.sh
# TODO: Chown the root directory
#sudo chown -R root ${OUTDIR}/rootfs
#sudo chmod -R 777 ${OUTDIR}/rootfs  #This is insecure. But security is not one of our objectives here.
echo "find:"
pushd ${OUTDIR}/rootfs
find . | cpio -H newc -ov --owner=root:root > ${OUTDIR}/initramfs.cpio
gzip -f ${OUTDIR}/initramfs.cpio
# TODO: Create initramfs.cpio.gz
cd $START_DIR
