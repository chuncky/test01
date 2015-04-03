#!/bin/bash
# Copyright (c) Nuvoton Tech. Corp. All rights reserved.
# Description:  Nuvoton Linux platform toolchain install script
# Version:      2010-09-28      first release version
# Author:       C.C. Chang
ROOT_UID=0
if [ "$UID" -ne "$ROOT_UID" ] ;then
    echo 'Sorry, you are not root !!'
    exit 1
fi

ARM_TOOL_ROOT="/usr/local"
ARM_TOOL_NANE="arm_linux_4.3"
ARM_TOOL_SUBDIR="usr"
ARM_TOOL_PATH="$ARM_TOOL_ROOT/$ARM_TOOL_NANE/$ARM_TOOL_SUBDIR"
KERNEL_VER="linux-2.6.35.4.tar.gz"

NO_TOOL=0
if [ -d $ARM_TOOL_PATH ]; then
        echo "The folder \"$ARM_TOOL_PATH\" is already existed, continue the tool chain installation?(Y/N)"
        INSTALL=1
        while [ $INSTALL == 1 ]
        do
                read install
                case "$install" in
                [yY]|[yY][eE][sS] )
                        INSTALL=0
                        ;;
                [nN]|[nN][oO] )
                        echo "Skip toolchain installation"
                        INSTALL=2
                        NO_TOOL=1
                        ;;
                * )
                        echo "Please type yes or no!"
                        ;;
                esac
        done
        if [ $NO_TOOL == 0 ]; then
                echo "Do you want to remove this folder first?(Y/N)"
                REMOVE=1
                while [ $REMOVE == 1 ]
                do
                        read remove
                        case "$remove" in
                        [yY]|[yY][eE][sS] )
                                echo "Now deleteing the folder \"$ARM_TOOL_PATH\"..."
                                rm -rf $ARM_TOOL_PATH
                                REMOVE=0
                                ;;
                        [nN]|[nN][oO] )
                                echo "This installation will overwirte folder \"$ARM_TOOL_PATH\"!"
                                REMOVE=0
                                ;;
                        * )
                                echo "Please type yes or no!"
                                ;;
                        esac
                done
        fi
fi

if [ $NO_TOOL == 0 ]; then
        echo "Now installing $ARM_TOOL_NANE toolchain to $ARM_TOOL_ROOT"
        echo 'Please wait for a while... '
        mkdir -p $ARM_TOOL_ROOT
        tar --directory=$ARM_TOOL_ROOT -zxf $ARM_TOOL_NANE.tar.gz&
        pid=`echo $!`
        str="| / - \\"
        count=1
        while [ -d /proc/$pid ]
        do
                if [ $count == 5 ]; then
                        count=1
                fi
                echo -e -n "\r"`echo $str | awk '{print $'$count'}'`
                count=`expr $count + 1`
                sleep 1
        done
        chown root:root -R $ARM_TOOL_PATH
        echo -e -n '\r'
fi

echo "Now setting toolchain environment"
PROFILE=/etc/profile
NVTFILE=/etc/profile.d/nvt_arm_linux.sh
TMPFILE=`mktemp -q /tmp/$0.XXXXXX`
if [ $? -ne 0 ]; then
        echo "$0: Can't create temp file, exiting..."
        exit 1
fi

# Check toolchain environment setting in PROFILE
REC=$(cat $PROFILE|tr -d '\r'|tr -t '\n' '\r'|sed '1,$s/\\//g' |tr -t '\r' '\n'|grep "PATH="|sed '1,$s/PATH=//g'|tr -t '\n' ':')
NUM=$(echo $REC|awk -F: 'END {print NF}')
i=1
RES=0
while (test $i -le $NUM)
do
        TMP=$(echo $REC | awk -F: "{print \$$i}")
        if [ "$TMP" = "$ARM_TOOL_PATH/bin" ]; then
                RES=1
        elif [ "$TMP" = "$ARM_TOOL_PATH/bin/" ]; then
                RES=1
        fi
        i=`expr $i + 1`
done

# Check toolchain environment setting in NVTFILE
if [ $RES == 0 ]; then
        if [ -f $NVTFILE ]; then
                cat $NVTFILE | grep "^[ ]*export PATH=$ARM_TOOL_PATH/bin" > /dev/null
                if [ $? == 0 ]; then
                        RES=1
                fi
        fi
fi

if [ $RES == 1 ]; then
        echo > /dev/null
else
        if [ -f $NVTFILE ]; then
                cat $NVTFILE | grep  "^[ ]*export " > /dev/null
                if [ $? == 0 ]; then
                        ARM_TOOL_PATH_TMP=`echo $ARM_TOOL_PATH | sed -e 's/\//\\\\\//g'`
                        cat $NVTFILE | grep  "^[ ]*export " | grep "$ARM_TOOL_PATH/bin" > /dev/null
                        if [ $? == 0 ]; then
                                sed -e 's/'$ARM_TOOL_PATH_TMP'\/bin[/]*://' $NVTFILE > $TMPFILE
                                cp -f $TMPFILE $NVTFILE
                        fi
                        sed -e 's/PATH=/PATH='$ARM_TOOL_PATH_TMP'\/bin:/' $NVTFILE > $TMPFILE
                        cp -f $TMPFILE $NVTFILE
                        rm -f $TMPFILE
                else
                        echo 'export PATH='$ARM_TOOL_PATH'/bin:$PATH' >> $NVTFILE
                fi
        else
                echo '## Nuvoton toolchain environment export' >> $NVTFILE
                echo 'export PATH='$ARM_TOOL_PATH'/bin:$PATH' >> $NVTFILE
                chmod +x $NVTFILE
        fi
fi

echo "Installing $ARM_TOOL_NANE toolchain successfully"

echo "Install rootfs.tar.gz,applications.tar.gz and $KERNEL_VER"
echo 'Please enter absolute path for installing(eg:/home/<user name>) :'
read letter

if [ -d $letter ] ;then
        echo ''$letter' has existed';
else
        echo 'Create '$letter' ';
        mkdir $letter;
        if [ -z $SUDO_USER ];then
                chown -R $USER:$USER $letter;
        else
                chown $SUDO_USER:$SUDO_USER $letter;
        fi
fi


if [ -d $letter/nuc900bsp ];then
        echo ''$letter'/nuc900bsp/ existed, (o)verwite, (r)emove or (a)bort?[y/n]';
        read remove;
        case "$remove" in
        [oO] )
                echo "Overwrite $letter/nuc900bsp folder"
                ;;
        [rR] )
                echo "Remove $letter/nuc900bsp folder"
                rm -rf $letter/nuc900bsp/*;
                ;;
        [aA] )
                echo 'Abort installation process';
                ;;
        * )
                echo 'Abort installation process';
                ;;
        esac
else
        mkdir $letter/nuc900bsp;
fi

echo 'Please wait for a while, it will take some time';
tar --directory=$letter/nuc900bsp -z -x -f rootfs.tar.gz;
tar --directory=$letter/nuc900bsp -z -x -f applications.tar.gz&
pid=`echo $!`
str="| / - \\"
count=1
while [ $? == 0 ]
do
        if [ $count == 4 ]; then
                count=1
        fi
        echo -e -n '\r'`echo $str|awk '{print $'$count'}'`
        count=`expr $count + 1`
        sleep 1
        ps aux|awk '$2=='$pid' {print $0}'|grep $pid > /dev/null
done
tar --directory=$letter/nuc900bsp -z -x -f image.tar.gz;
tar --directory=$letter/nuc900bsp -z -x -f $KERNEL_VER&
pid=`echo $!`
str="| / - \\"
count=1
while [ $? == 0 ]
do
        if [ $count == 4 ]; then
                count=1
        fi
        echo -e -n '\r'`echo $str|awk '{print $'$count'}'`
        count=`expr $count + 1`
        sleep 1
        ps aux|awk '$2=='$pid' {print $0}'|grep $pid > /dev/null
done

find $letter -type d|xargs chmod 755
if [ -z $SUDO_USER ];then
        chown -R $USER:$USER $letter/nuc900bsp;
else
        chown -R $SUDO_USER:$SUDO_USER $letter/nuc900bsp;
fi
echo -e '\rNUC900 BSP installation complete'
