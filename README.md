# RatArena gamecode

![RatArena Logo](ratmod_logo.svg)

RatArena (or Ratmod) is a mod for OpenArena for both competitive and casual gameplay.

It adds many new features and quality improvements, among them are:

- delagged projectiles
- improved local prediction of various game events as well as other anti-lag features
- latency equalizer for maximum fairness (optional)
- new gametypes: Multitournament (multiple simultaneous duels), Extermination (Wipeout),
  FreezeTag, CoinFFA, CoinTDM, Treasure Hunter
- missiles can go through teleporters
- grenades are pushed by jumppads
- highly configurable brightskins or bright player outlines (can be disabled)
- ping feature to help coordinating in team games
- friend markers that are visible through walls and indicate health/armor status
- improved map vote menu
- configurable map vote at the end of games
- improved HUD with widescreen support
- new, high-res icons
- new font
- other 2d upgrades
- new awards (medals)
- new announcer
- team queue system to enforce equal team sizes
- team balance system
- improved weapon visuals (e.g. rail, rockets, lightning gun, grenades, ..)
- pause/unpause feature
- physics improvements
- physics settings: additive jump, rampjump, ratmode (more aircontrol), etc.
- taunts
- numerous bugfixes and quality improvements
- ...and much more!

# Documentation

See the [documentation for players](https://ratmod.github.io).

## Running a server

Generally, the OpenArena documentation on how to run a server applies.
Check out additional settings in ratmod using `\cvarlist g_*` from the server console.

Some important/recommended settings are (incomplete list):

```
set sv_pure 1 // pure server, only allow clients to load pk3s also loaded on the server
set sv_fps 40 // ratmod is intended to be used with 40 server frames per second.
set pmove_float 1 // floating-point physics
set pmove_fixed 0

// sv_floodprotect may cause issues (chats not going through etc)
// you should use the g_flood* settings to limit flooding instead
set sv_floodProtect 0 

// enable hitscan delag
set g_delagHitscan 1
set g_lagLightning 0

// recommended:
set sv_timeout 60 // quick timeout
set sv_maxping 0 // may cause problems if set to any other value
set com_ansiColor 1 // colored console
set videoflags 0 // allow clients to use things like vertex light, picmip etc
```

Many of the mod settings listed are defaults already, but may have different defaults in OA itself. Be careful not to mix up configs from different games.

# Building

If you want to build ratmod from source, you should check out
the [ratarena_release repository](https://github.com/rdntcntrl/ratarena_release), which can be used to compile the gamecode and assets in a reproducible manner.

**Note:** Building Ratmod is only tested on Linux. It *might* work on BSDs, but will likely fail on Windows.

For developers, you can build the QVM + native code from this repository using:

```sh
make WITH_MULTITOURNAMENT=1 BUILD_GAME_QVM=1 BUILD_STANDALONE=1 BUILD_GAME_SO=1 BUILD_MISSIONPACK=0
```

# Donate

If you enjoy RatArena/Ratmod, please consider supporting the project by donating to one of the following cryptocurrency addresses:

Bitcoin (BTC): bc1q70e95n73l24r7j0phr79ejkpzhrqmqgch3atdf

Monero (XMR): 83bfSYwMRhUGeRkPkpM3W8SWYv194ud1aUeu5rRJNmXYGNjZTF4zTKCJ9ydgABVAa7NQQD2hLvjiQFGNJVvDSGZhRKaLx5t

