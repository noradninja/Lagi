include("${REPO}/cmake/LagiLifeMeterFlags.cmake")
foreach(path "battle/BTL_A3/urchin.cpp" "battle/BTL_A3/baldorQueen.cpp"
 "battle/BTL_A5/BTL_A5_single.cpp" "battle/BTL_A5/BTL_A5_variant.cpp")
 file(READ "${REPO}/extern/Azel/AzelLib/${path}" original)
 lagi_adapt_life_meter_flags("${path}" "${original}" adapted)
 string(REPLACE "/" "_" filename "${path}")
 file(WRITE "${SCRATCH}/${filename}" "${adapted}")
endforeach()
