#pragma once
// Include after the Azel prelude. PC points just after opcode 30.
struct s_fieldScriptWorkArea;
enum class LagiGarilChoiceStep {Unsupported,Continue,Yield};
LagiGarilChoiceStep lagiAzelRunGarilChoice30(s_fieldScriptWorkArea*,sSaturnPtr& pc);
