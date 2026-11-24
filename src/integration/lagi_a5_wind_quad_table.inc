// Native 06058800..06 indexes a table of Saturn pointers, then loads the
// pointed-to quad list. The table storage itself is never sprite data.
static const std::vector<sVdp1Quad>* lagiA5WindQuadList(u32 table,u32 index) {
    return a5GetOrParseQuadList(gFLD_A5->getSaturnPtr(
        readSaturnU32(gFLD_A5->getSaturnPtr(table+index*4))));
}
