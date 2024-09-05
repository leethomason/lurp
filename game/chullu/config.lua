Config = {
    gameVersion = "0.1",
    gameAuthor = "Game Author",
    gameDescription = "A text adventure game",

    -- config


    -- config 2D
    size = { 1920, 1080},
    textColor = 0xffaa00,

    navNormalColor = 0x51a6f5,
    navOverColor = 0xbddfff,
    navDownColor = 0x3d77ad,
    navDisabledColor = 0x586775,

    optionNormalColor = 0x62cc69,
    optionOverColor = 0xb1f0b7,
    optionDownColor = 0x5c9160,
    optionDisabledColor = 0x4b664d,

    backgroundColor = { 0, 0, 0, 255 },

    fontName = "Lora-Medium.ttf",
    fontSize = 28,
    uiFont = "Roboto-Regular.ttf",
    uiFontSize = 48,

    regions = {
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
