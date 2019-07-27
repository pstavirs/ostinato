QMAKE_CXXFLAGS += -isystem $$[QT_INSTALL_HEADERS] -std=c++11
CONFIG(debug, debug|release): QMAKE_CXXFLAGS_WARN_ON += -Wall -W -Wextra -Werror
CONFIG(debug, debug|release): QMAKE_CXXFLAGS_WARN_ON += -Wno-deprecated-declarations
