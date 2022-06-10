cmake_minimum_required(VERSION 3.15)

project(qscintilla2_qt)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
else()
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-deprecated-declarations -Werror")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wno-deprecated-declarations -Werror")
endif()
add_definitions(-w)

include (../../cmake/cmake_qt.txt)
include (../../cmake/cmake_tcl.txt)

add_compile_definitions(SCINTILLA_QT=1)
add_compile_definitions(SCI_LEXER=1)

include_directories(
  scintilla/include
  scintilla/lexlib
  scintilla/src
  src/Qsci
  src)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../bin)

set (qscintilla2_qt_HDRS
 src/Qsci/qsciglobal.h
 src/Qsci/qsciscintilla.h
 src/Qsci/qsciscintillabase.h
 src/Qsci/qsciabstractapis.h
 src/Qsci/qsciapis.h
 src/Qsci/qscicommand.h
 src/Qsci/qscicommandset.h
 src/Qsci/qscidocument.h
 src/Qsci/qscilexer.h
 src/Qsci/qscilexertcl.h
 src/Qsci/qscilexerverilog.h
 src/Qsci/qscilexervhdl.h
 src/Qsci/qscimacro.h
 src/Qsci/qscistyle.h
 src/Qsci/qscistyledtext.h
 src/SciClasses.h
 src/ScintillaQt.h
 src/ListBoxQt.h
 src/SciAccessibility.h
 scintilla/include/ILexer.h
 scintilla/include/ILoader.h
 scintilla/include/Platform.h
 scintilla/include/Sci_Position.h
 scintilla/include/SciLexer.h
 scintilla/include/Scintilla.h
 scintilla/include/ScintillaWidget.h
 scintilla/lexlib/Accessor.h
 scintilla/lexlib/CharacterCategory.h
 scintilla/lexlib/CharacterSet.h
 scintilla/lexlib/DefaultLexer.h
 scintilla/lexlib/LexAccessor.h
 scintilla/lexlib/LexerBase.h
 scintilla/lexlib/LexerModule.h
 scintilla/lexlib/LexerNoExceptions.h
 scintilla/lexlib/LexerSimple.h
 scintilla/lexlib/OptionSet.h
 scintilla/lexlib/PropSetSimple.h
 scintilla/lexlib/SparseState.h
 scintilla/lexlib/StringCopy.h
 scintilla/lexlib/StyleContext.h
 scintilla/lexlib/SubStyles.h
 scintilla/lexlib/WordList.h
 scintilla/src/AutoComplete.h
 scintilla/src/CallTip.h
 scintilla/src/CaseConvert.h
 scintilla/src/CaseFolder.h
 scintilla/src/Catalogue.h
 scintilla/src/CellBuffer.h
 scintilla/src/CharClassify.h
 scintilla/src/ContractionState.h
 scintilla/src/DBCS.h
 scintilla/src/Decoration.h
 scintilla/src/Document.h
 scintilla/src/EditModel.h
 scintilla/src/Editor.h
 scintilla/src/EditView.h
 scintilla/src/ElapsedPeriod.h
 scintilla/src/ExternalLexer.h
 scintilla/src/FontQuality.h
 scintilla/src/Indicator.h
 scintilla/src/IntegerRectangle.h
 scintilla/src/KeyMap.h
 scintilla/src/LineMarker.h
 scintilla/src/MarginView.h
 scintilla/src/Partitioning.h
 scintilla/src/PerLine.h
 scintilla/src/Position.h
 scintilla/src/PositionCache.h
 scintilla/src/RESearch.h
 scintilla/src/RunStyles.h
 scintilla/src/ScintillaBase.h
 scintilla/src/Selection.h
 scintilla/src/SparseVector.h
 scintilla/src/SplitVector.h
 scintilla/src/Style.h
 scintilla/src/UniConversion.h
 scintilla/src/UniqueString.h
 scintilla/src/ViewStyle.h
 scintilla/src/XPM.h
	)

set ( qscintilla2_qt_SRCS
	src/qsciscintilla.cpp
	src/qsciscintillabase.cpp
	src/qsciabstractapis.cpp
	src/qsciapis.cpp
	src/qscicommand.cpp
	src/qscicommandset.cpp
	src/qscidocument.cpp
	src/qscilexer.cpp
	src/qscilexertcl.cpp
	src/qscilexerverilog.cpp
	src/qscilexervhdl.cpp
	src/qscimacro.cpp
	src/qscistyle.cpp
	src/qscistyledtext.cpp
	src/InputMethod.cpp
	src/ListBoxQt.cpp
 #	src/MacPasteboardMime.cpp --Temporarily comment out the code applied to the Mac
	src/PlatQt.cpp
	src/SciAccessibility.cpp
	src/SciClasses.cpp
	src/ScintillaQt.cpp
	scintilla/lexers/LexTCL.cpp
	scintilla/lexers/LexVHDL.cpp
	scintilla/lexers/LexVerilog.cpp
	scintilla/lexlib/Accessor.cpp
	scintilla/lexlib/CharacterCategory.cpp
	scintilla/lexlib/CharacterSet.cpp
	scintilla/lexlib/DefaultLexer.cpp
	scintilla/lexlib/LexerBase.cpp
	scintilla/lexlib/LexerModule.cpp
	scintilla/lexlib/LexerNoExceptions.cpp
	scintilla/lexlib/LexerSimple.cpp
	scintilla/lexlib/PropSetSimple.cpp
	scintilla/lexlib/StyleContext.cpp
	scintilla/lexlib/WordList.cpp
	scintilla/src/AutoComplete.cpp
	scintilla/src/CallTip.cpp
	scintilla/src/CaseConvert.cpp
	scintilla/src/CaseFolder.cpp
	scintilla/src/Catalogue.cpp
	scintilla/src/CellBuffer.cpp
	scintilla/src/CharClassify.cpp
	scintilla/src/ContractionState.cpp
	scintilla/src/DBCS.cpp
	scintilla/src/Decoration.cpp
	scintilla/src/Document.cpp
	scintilla/src/EditModel.cpp
	scintilla/src/Editor.cpp
	scintilla/src/EditView.cpp
	scintilla/src/ExternalLexer.cpp
	scintilla/src/Indicator.cpp
	scintilla/src/KeyMap.cpp
	scintilla/src/LineMarker.cpp
	scintilla/src/MarginView.cpp
	scintilla/src/PerLine.cpp
	scintilla/src/PositionCache.cpp
	scintilla/src/RESearch.cpp
	scintilla/src/RunStyles.cpp
	scintilla/src/ScintillaBase.cpp
	scintilla/src/Selection.cpp
	scintilla/src/Style.cpp
	scintilla/src/UniConversion.cpp
	scintilla/src/ViewStyle.cpp
	scintilla/src/XPM.cpp
	)


add_library (qscintilla2_qt STATIC ${qscintilla2_qt_SRCS} ${qscintilla2_qt_HDRS})

target_link_libraries (qscintilla2_qt  PUBLIC Qt5::Core Qt5::Gui Qt5::Widgets)
target_compile_definitions(qscintilla2_qt PRIVATE QSCINTILLA2_QT_LIBRARY)

set (qscintilla2_qt_INCLUDE
 Qsci/qsciglobal.h
 Qsci/qsciscintilla.h
 Qsci/qsciscintillabase.h
 Qsci/qsciabstractapis.h
 Qsci/qsciapis.h
 Qsci/qscicommand.h
 Qsci/qscicommandset.h
 Qsci/qscidocument.h
 Qsci/qscilexer.h
 Qsci/qscilexertcl.h
 Qsci/qscilexerverilog.h
 Qsci/qscilexervhdl.h
 Qsci/qscimacro.h
 Qsci/qscistyle.h
 Qsci/qscistyledtext.h
 )

foreach(headerfile ${qscintilla2_qt_INCLUDE})
add_custom_command(
  TARGET qscintilla2_qt POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
          ${PROJECT_SOURCE_DIR}/src/${headerfile}
          ${CMAKE_CURRENT_BINARY_DIR}/../../include/${headerfile}
)
endforeach()

set(QSCINTILLA_STATIC_LIB libqscintilla2_qt.a)
if (MSVC)
  set(QSCINTILLA_STATIC_LIB qscintilla2_qt.lib)
endif()

install (
  FILES ${CMAKE_CURRENT_BINARY_DIR}/../../lib/${QSCINTILLA_STATIC_LIB}
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/foedag)

foreach(headerfile ${qscintilla2_qt_INCLUDE})
install(
    FILES ${PROJECT_SOURCE_DIR}/src/${headerfile}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/foedag/Qsci)
endforeach()
  

