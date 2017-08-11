#!/bin/bash
# ##########################################################
# ALPS(Android4.1 based) build environment profile setting
# ##########################################################

# Dirty hack for Fedora
unset module scl

# Overwrite PATH environment setting for JDK & arm-eabi if already exists
PATH=$JAVA_HOME/bin:$PWD/prebuilts/gcc-linaro-7.1.1-2017.05-x86_64_arm-linux-gnueabihf/bin/:$PATH
export PATH

# Add MediaTek developed Python libraries path into PYTHONPATH
if [ -z "$PYTHONPATH" ]; then
  PYTHONPATH=$PWD/mediatek/build/tools
else
  PYTHONPATH=$PWD/mediatek/build/tools:$PYTHONPATH
fi
export PYTHONPATH

# MTK customized functions
# Adatping MTK build system with custmer(L)'s setting
function m()
{
    export MY_BUILD_CMD="m $@"
    local HERE=$PWD
    T=$(gettop)
    if [ "$T" ]; then
        local LOG_FILE=$T/build_log.txt
        cd $T
        time ./makeMtk -t -o=TARGET_BUILD_VARIANT=$TARGET_BUILD_VARIANT,TARGET_BUILD_TYPE=$TARGET_BUILD_TYPE,$@ \
            $TARGET_PRODUCT r | tee -a $LOG_FILE
        if [ ${PIPESTATUS[0]} -ne 0 ]; then
            cd $HERE > /dev/null
            echo -e "\nBuild log was written to '$LOG_FILE'."
            echo "Error: Building failed"
            return 1
        fi
        cd $HERE > /dev/null
        echo -e "\nBuild log was written to '$LOG_FILE'."
    else
        echo "Couldn't locate the top of the tree.  Try setting TOP."
    fi
}



function create_link
{
    MAKEFILE_PATH=`/bin/ls device/*/*/$1.mk 2>/dev/null`

    TARGET_PRODUCT=`cat $MAKEFILE_PATH | grep PRODUCT_NAME | awk '{print $3}'`
    BASE_PROJECT=`cat $MAKEFILE_PATH | grep BASE_PROJECT | awk '{print $3}'`

    if [ ! "x$BASE_PROJECT" == x ]; then
        echo "Create symbolic link - $TARGET_PRODUCT based on $BASE_PROJECT"
        if [ ! -e "./mediatek/config/$TARGET_PRODUCT" ]; then
            ln -s `pwd`/mediatek/config/$BASE_PROJECT ./mediatek/config/$TARGET_PRODUCT
        fi
        if [ ! -e "./mediatek/custom/$TARGET_PRODUCT" ]; then
            ln -s `pwd`/mediatek/custom/$BASE_PROJECT ./mediatek/custom/$TARGET_PRODUCT
        fi
    else
        echo ./mediatek/config/$TARGET_PRODUCT
        if [ ! -e "./mediatek/config/$TARGET_PRODUCT" ]; then
            echo "NO BASE_PROJECT!!"
            return 1
        fi
    fi
}

function mtk_custgen()
{
    T=$(gettop)
    if [ "$T" ]; then
        rm -rf mediatek/config/out/$TARGET_PRODUCT
        ./makeMtk -o=TARGET_BUILD_VARIANT=$TARGET_BUILD_VARIANT,TARGET_BUILD_TYPE=$TARGET_BUILD_TYPE $TARGET_PRODUCT custgen
    else
        echo "Couldn't locate the top of the tree.  Try setting TOP."
    fi
}
