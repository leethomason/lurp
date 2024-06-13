Config = {
    gameVersion = "0.1",
    gameAuthor = "Game Author",
    gameDescription = "A text adventure game",

    size = { 1920, 1080},
    clearColor = { 0, 0, 0 },
    frames = {
        { 0, 0, 1920, 1080, "background" },
        { 40, 40, 500, 1000, "image"},          -- fixme
        { 540, 40, 1340, 1000, "text", type="text", opaque=true},  -- fixme
        { 40, 1040, 1840, 40, "status"},         -- fixme
    }
}
