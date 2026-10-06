# ESCompass

EuroScope overlay that draws two geographic compasses on the normal radar screen.

Bearings are true north on the picture, so both compasses turn when the scope is rotated. They match ground track on the map. They are not magnetic: a magnetic heading differs from the compass by the sectorfile variation.

## Commands

| Command | Effect |
|---|---|
| `.cmprose` | Toggle the round compass rose |
| `.cmprose on` / `.cmprose off` | Force the rose on or off |
| `.cmpsquare` | Toggle the square compass around the radar edge |
| `.cmpsquare on` / `.cmpsquare off` | Force the square compass on or off |
| `.cmptop` | Show the reserved strip at the top, in pixels |
| `.cmptop 20` | Keep both compasses this many pixels below the top of the radar |
| `.cmptop 0` | Draw up to the top edge again |

Each command is acknowledged in the `ESCompass` chat tab. The choice is stored in the ASR.

Both start off. They can be on together.

## What is drawn

The rose is a true circle on the centre of the radar area. Ticks are every degree, longer at 5°, 10° and the cardinals. Labels are `000`–`350`, centred on the ray.

The square compass puts each bearing where that true-bearing ray meets the edge of the radar area. Tick spacing follows the angle from the centre, so it stays right on a wide, square or rotated scope. vParkAir instead split the border into fixed ranges (56°, 125°, 236°, 305°), which only approximated one window shape and mirrored the left-hand labels.

Chat and the EuroScope toolbar are left outside the frame when EuroScope reports them overlapping the radar area.

TopSky's global menu is not one of those areas. It is painted on the radar afterwards, so a compass drawn underneath it is covered. Both compasses are fitted below a 20 pixel strip, which clears the one-line menu in the UK pack. If a taller menu still covers the arc, raise it with `.cmptop`. Docked lists, such as Sector Inbound, are separate windows and will still cover anything under them.

## Install

EuroScope is 32-bit. Build **Release | Win32** and load `bin\Win32\Release\ESCompass.dll`.

1. Other SET → Plug-ins → Load, and select `ESCompass.dll`.
2. Select **ESCompass** and move **Standard ES radar screen** (and any other scope you use) from *Forbidden to draw on types* to *Allowed to draw on types*.
3. Close the dialog and reopen the radar screen so the plugin can attach.

## Build

Requires Visual Studio 2022 or 2026 with the C++ desktop workload. The project uses that install's own platform toolset (v143 or v145), not the old VS2019 v142 tools.

The EuroScope plugin header and import library are already in [ESCompass/sdk](ESCompass/sdk). You do not need a separate SDK install. EuroScope itself stopped shipping those files next to the program.

Open [ESCompass.sln](ESCompass.sln), choose **Release** and **Win32**, and build. Do not build x64: EuroScope will not load it.
