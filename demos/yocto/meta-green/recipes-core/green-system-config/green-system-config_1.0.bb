SUMMARY = "Limited log size and a watchdog for long-lived systems"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

SRC_URI = "file://10-green-journal.conf file://10-green-watchdog.conf"
S = "${UNPACKDIR}"

inherit allarch

do_install() {
    install -d ${D}${systemd_unitdir}/journald.conf.d ${D}${systemd_unitdir}/system.conf.d
    install -m 0644 ${UNPACKDIR}/10-green-journal.conf ${D}${systemd_unitdir}/journald.conf.d/
    install -m 0644 ${UNPACKDIR}/10-green-watchdog.conf ${D}${systemd_unitdir}/system.conf.d/
}

FILES:${PN} = "${systemd_unitdir}/journald.conf.d ${systemd_unitdir}/system.conf.d"
