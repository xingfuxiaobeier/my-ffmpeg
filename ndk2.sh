#!/bin/bash
NDK=/Users/mtime/Library/Android/sdk/ndk-bundle 
SYSROOT=$NDK/platforms/android-16/arch-arm/ 
TOOLCHAIN=$NDK/toolchains/arm-linux-androideabi-4.9/prebuilt/darwin-x86_64 
CPU=arm 
PREFIX=$(pwd)/android
ADDI_CFLAGS="-marm" 
function build_one(){ 
./configure \
 --prefix=/Users/mtime/IdeaProjects/FFmpeg/android/arm2 \
 --enable-shared \
 --disable-static \
 --disable-doc \
 --disable-ffmpeg \
 --disable-ffplay \
 --disable-ffprobe \
 --disable-ffserver \
 --disable-doc \
 --disable-symver \
 --enable-small \
 --enable-memalign-hack \
 --enable-jni \
 --enable-mediacodec \
 --enable-jni \
 --enable-mediacodec \
 --enable-decoder=h264_mediacodec \
 --enable-hwaccel=h264_mediacodec \
 --enable-decoder=hevc_mediacodec \
 --enable-decoder=mpeg4_mediacodec \
 --enable-decoder=vp8_mediacodec \
 --enable-decoder=vp9_mediacodec \
 --enable-gpl \
 --enable-nonfree \
 --enable-version3 \
 --cross-prefix=$TOOLCHAIN/bin/arm-linux-androideabi- \
 --target-os=linux \
 --arch=arm \
 --enable-cross-compile \
 --sysroot=$SYSROOT \
 --extra-cflags="-Os -fpic $ADDI_CFLAGS" \
 --extra-ldflags="$ADDI_LDFLAGS" \
 $ADDITIONAL_CONFIGURE_FLAG 
 make clean 
 make -j8
 make install 
} 
build_one
