-- Example MOD for QuentlamEngine
-- Demonstrates the Lua MOD framework

local mod = {
    id = "example-mod",
    name = "Example Mod",
    loaded = false
}

function mod:load()
    QL_Log("Example Mod: load() called")
    QL_Log("Engine: " .. QL.ENGINE_NAME)
    QL_Log("Version: " .. QL.VERSION)
    self.loaded = true
end

function mod:update(dt)
    -- Called every frame with delta time
end

function mod:onSave()
    QL_Log("Example Mod: onSave() called")
end

function mod:onLoad()
    QL_Log("Example Mod: onLoad() called")
end

return mod
