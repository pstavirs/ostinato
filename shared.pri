#
# Qt qmake integration for installing files other than executables
# Author: Srivats P
#
# To install files other than executables, specify them in SHARED variable
# and include this file AFTER install.pri
#
# Example:
# SHARED = file1 file2
# include(install.pri)
# include(shared.pri)
#

macx {
    shared.path = $${PREFIX}/Ostinato/Ostinato.app/Contents/SharedSupport/
} else: unix {
    shared.path = $${PREFIX}/share/ostinato-controller/
} else {
    shared.path = $${PREFIX}/bin
}
shared.files = $${SHARED}

INSTALLS += shared

# Themes - need a subdir inside shared
themes.files = $${THEMES}
themes.path = $${shared.path}/themes

INSTALLS += themes
