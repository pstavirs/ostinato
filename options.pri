QMAKE_CXXFLAGS += -isystem $$[QT_INSTALL_HEADERS]
CONFIG(debug, debug|release): QMAKE_CXXFLAGS_WARN_ON += -Wall -W -Wextra -Werror
