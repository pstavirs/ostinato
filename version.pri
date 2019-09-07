APP_VERSION = 1.0
APP_REVISION = $(shell git rev-parse --short=12 --verify HEAD)
#uncomment the below line in a source package and fill-in the correct revision
#APP_REVISION = <rev-hash>@

ver_info {
    APP_VERSION_FILE = version.cpp
    revtarget.target = $$APP_VERSION_FILE
    win32:revtarget.commands = echo "const char *version = \"$$APP_VERSION\";" \
            "const char *revision = \"$$APP_REVISION\";" \
            > $$APP_VERSION_FILE
    unix:revtarget.commands = echo \
            "\"const char *version = \\\"$$APP_VERSION\\\";" \
            "const char *revision = \\\"$$APP_REVISION\\\";\"" \
            > $$APP_VERSION_FILE
    revtarget.depends = $$SOURCES $$HEADERS $$FORMS $$POST_TARGETDEPS

    SOURCES += $$APP_VERSION_FILE
    QMAKE_EXTRA_TARGETS += revtarget
    POST_TARGETDEPS += $$APP_VERSION_FILE 
    QMAKE_DISTCLEAN += $$APP_VERSION_FILE
}
