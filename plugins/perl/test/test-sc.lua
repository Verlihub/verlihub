-- PerlScript test suite - ScriptCommand hook in lua
function VH_OnScriptCommand(cmd, data, plug, script)
  VH:SendToClass("    (test-sc.lua:VH_OnScriptCommand) cmd=["..cmd.."] data=["..data.."] plug=["..plug.."] script=["..script.."]|", 10, 10)
  return 1
end
