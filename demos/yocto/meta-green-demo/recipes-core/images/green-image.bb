SUMMARY = "Small reference image of the green distribution"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit core-image

IMAGE_INSTALL = "packagegroup-core-boot packagegroup-green ${CORE_IMAGE_EXTRA_INSTALL}"
