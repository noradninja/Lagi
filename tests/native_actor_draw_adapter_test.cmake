cmake_minimum_required(VERSION 3.20)
file(READ "${CMAKE_CURRENT_LIST_DIR}/../CMakeLists.txt" adapters)
string(FIND "${adapters}" "  # Report unsupported native actor variants" start)
string(FIND "${adapters}" "  # Matrix-driven models without hotpoints" finish)
if(start LESS 0 OR finish LESS start)
 message(FATAL_ERROR "Native actor diagnostic adapter missing")
endif()
math(EXPR length "${finish} - ${start}")
string(SUBSTRING "${adapters}" ${start} ${length} adapter)
set(source_text [=[void unimplementedDraw(s_3dModel* pDragonStateData1)
{
    assert(0);
}
    p3dModel->m1C_addToDisplayListFunction = addObjectToDrawList;]=])
cmake_language(EVAL CODE "${adapter}")
if(NOT source_text MATCHES "NativeActorDrawMissing" OR source_text MATCHES "assert\\(0\\)")
 message(FATAL_ERROR "Native actor diagnostic replacement failed")
endif()
if(NOT source_text MATCHES "NativeActorModelReady" OR
   NOT source_text MATCHES "actorInitReports < 32u")
 message(FATAL_ERROR "Native actor initialization diagnostic replacement failed")
endif()
message(STATUS "Native actor diagnostic adapter passed")
