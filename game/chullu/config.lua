Config = {
    gameVersion = "0.1",
    gameAuthor = "Game Author",
    gameDescription = "A text adventure game",

    size = { 1920, 1080},
    clearColor = { 0, 0, 0 },
    frames = {
        { "background", pos = {0, 0, 1920, 1080 }},
        { "image", pos = { 50, 50, 528, 980 }},
        { "text", pos = { 620, 50, 760, 980 },
            bg = { 64, 64, 64, 255 }
        },
        { "info", pos = { 1422, 50, 448, 980 },
            bg = { 64, 64, 64, 255 },
        }
    }
}
