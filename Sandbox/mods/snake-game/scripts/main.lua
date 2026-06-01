-- ================================================================
-- Snake Game - Quentlam Engine Lua Demo (ECS Edition)
-- ================================================================
-- Game logic in Lua, rendering via C++ Renderer2D through Entity_*
-- ================================================================

-- ---- Game Constants ----
local GRID_COLS = 20
local GRID_ROWS = 20
local TICK_RATE = 0.15

local CELL_SIZE = 0.5

local GRID_OFFSET_X = (GRID_COLS / 2.0) * CELL_SIZE
local GRID_OFFSET_Y = (GRID_ROWS / 2.0) * CELL_SIZE

local function gridToWorld(gx, gy)
    local wx = (gx - 0.5) * CELL_SIZE - GRID_OFFSET_X
    local wy = (gy - 0.5) * CELL_SIZE - GRID_OFFSET_Y
    return wx, wy
end

-- Direction vectors
local DIR = {
    UP    = {x = 0,  y = -1},
    DOWN  = {x = 0,  y = 1},
    LEFT  = {x = -1, y = 0},
    RIGHT = {x = 1,  y = 0},
}

local OPPOSITE = {
    UP    = "DOWN",
    DOWN  = "UP",
    LEFT  = "RIGHT",
    RIGHT = "LEFT",
}

-- ---- Game State ----
local gameState = "playing"
local score = 0
local gameTime = 0.0
local gameOver = false

local function getState() return gameState end
local function setState(s) gameState = s end
local function getScore() return score end
local function setScore(s) score = s end
local function getGameOver() return gameOver end
local function setGameOver(g) gameOver = g end

-- ---- Snake State ----
local snake = {}
local direction = {x = 1, y = 0}
local nextDirection = {x = 1, y = 0}
local food = {x = 0, y = 0}
local moveTimer = 0.0

local pendingDirection = nil

-- Entity IDs
local ENTITY_FOOD = 1
local ENTITY_SNAKE_START = 100

-- ================================================================
-- Utility Functions
-- ================================================================

local function isOnSnake(x, y)
    for i = 1, #snake do
        if snake[i].x == x and snake[i].y == y then
            return true
        end
    end
    return false
end

local function isValidPosition(x, y)
    return x >= 1 and x <= GRID_COLS and y >= 1 and y <= GRID_ROWS
end

local function initSnake()
    snake = {}
    local startX = math.floor(GRID_COLS / 2)
    local startY = math.floor(GRID_ROWS / 2)

    for i = 1, 3 do
        table.insert(snake, {x = startX - i + 1, y = startY})
    end

    direction = {x = 1, y = 0}
    nextDirection = {x = 1, y = 0}
end

local function spawnFood()
    local attempts = 0
    repeat
        food.x = Math_RandInt(1, GRID_COLS)
        food.y = Math_RandInt(1, GRID_ROWS)
        attempts = attempts + 1
    until not isOnSnake(food.x, food.y) or attempts > 100
end

-- ================================================================
-- Entity Sync: push game objects to C++ ECS
-- ================================================================

local function syncEntities()
    Entity_Destroy(ENTITY_FOOD)
    for i = 1, #snake do
        Entity_Destroy(ENTITY_SNAKE_START + i)
    end

    for i = 1, #snake do
        local seg = snake[i]
        local wx, wy = gridToWorld(seg.x, seg.y)
        local eid = ENTITY_SNAKE_START + i
        Entity_Create(eid, wx, wy, 0.0)

        if i == 1 then
            Entity_SetColor(eid, 0.2, 0.9, 0.4, 1.0)
        else
            local shade = 0.3 + (0.2 * ((#snake - i) / #snake))
            Entity_SetColor(eid, 0.1, shade, 0.15, 1.0)
        end
    end

    local fwx, fwy = gridToWorld(food.x, food.y)
    Entity_Create(ENTITY_FOOD, fwx, fwy, 0.0)
    Entity_SetColor(ENTITY_FOOD, 1.0, 0.2, 0.2, 1.0)
end

-- ================================================================
-- Input Handling
-- ================================================================

local function queueDirection(newDir)
    local dx, dy
    if newDir == "UP" then
        dx, dy = 0, -1
    elseif newDir == "DOWN" then
        dx, dy = 0, 1
    elseif newDir == "LEFT" then
        dx, dy = -1, 0
    elseif newDir == "RIGHT" then
        dx, dy = 1, 0
    else
        return
    end
    if direction.x == -dx and direction.y == -dy then
        return
    end
    pendingDirection = {x = dx, y = dy}
end

local function pollInput()
    if gameState ~= "playing" then return end
    if Input_IsKeyPressed(Key.Up) or Input_IsKeyPressed(Key.W) then
        queueDirection("UP")
    elseif Input_IsKeyPressed(Key.Down) or Input_IsKeyPressed(Key.S) then
        queueDirection("DOWN")
    elseif Input_IsKeyPressed(Key.Left) or Input_IsKeyPressed(Key.A) then
        queueDirection("LEFT")
    elseif Input_IsKeyPressed(Key.Right) or Input_IsKeyPressed(Key.D) then
        queueDirection("RIGHT")
    elseif Input_IsKeyPressed(Key.Space) and gameState == "gameover" then
        startGame()
    end
end

-- ================================================================
-- Game Logic
-- ================================================================

local function moveSnake()
    if pendingDirection then
        direction = pendingDirection
        pendingDirection = nil
    end

    local head = snake[1]
    local newHead = {
        x = head.x + direction.x,
        y = head.y + direction.y
    }

    if not isValidPosition(newHead.x, newHead.y) then
        gameOver = true
        gameState = "gameover"
        return
    end

    local eat = (newHead.x == food.x and newHead.y == food.y)

    if not eat then
        for i = 1, #snake do
            if snake[i].x == newHead.x and snake[i].y == newHead.y then
                gameOver = true
                gameState = "gameover"
                return
            end
        end
    end

    table.insert(snake, 1, newHead)

    if eat then
        score = score + 10
        spawnFood()
    else
        table.remove(snake)
    end
end

-- ================================================================
-- Lifecycle
-- ================================================================

function init()
    QL_Log("Snake Game Lua: init() called")
end

function load()
    QL_Log("Snake Game Lua: load() called")
    startGame()
end

function update(dt)
    pollInput()

    gameTime = gameTime + dt

    if gameState ~= "playing" then
        syncEntities()
        return
    end

    moveTimer = moveTimer + dt
    if moveTimer >= TICK_RATE then
        moveTimer = moveTimer - TICK_RATE
        moveSnake()
    end

    syncEntities()
end

function startGame()
    QL_Log("Snake Game Lua: startGame() called")
    gameState = "playing"
    score = 0
    gameTime = 0.0
    gameOver = false
    initSnake()
    spawnFood()
    moveTimer = 0.0
    pendingDirection = nil
    syncEntities()
end

function restartGame()
    initSnake()
    spawnFood()
    moveTimer = 0.0
    pendingDirection = nil
    gameOver = false
    syncEntities()
end

function onKeyPressed(keyCode)
    if keyCode == Key.Up or keyCode == Key.W then
        queueDirection("UP")
        return true
    elseif keyCode == Key.Down or keyCode == Key.S then
        queueDirection("DOWN")
        return true
    elseif keyCode == Key.Left or keyCode == Key.A then
        queueDirection("LEFT")
        return true
    elseif keyCode == Key.Right or keyCode == Key.D then
        queueDirection("RIGHT")
        return true
    end
    if keyCode == Key.Space and gameState == "gameover" then
        startGame()
        return true
    end
    return false
end

function destroy()
    QL_Log("Snake Game Lua: destroy() called")
end
