HEADERS = $$files(*.h, true)
SOURCES = $$files(*.cpp, true)

lupdate_only {
    SOURCES += $$files(*.qml, true)
}

TRANSLATIONS = $$files(StandAlone/share/ts/*.ts)
