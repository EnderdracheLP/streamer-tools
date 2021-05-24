# Streamer Tools

Makes information available which can then be used by a Streamer-Tools PC application.

## Installation
- To install the mod, download the mod below, drop it into BMBF then sync to Beat Saber.
- To get the mod working on your PC you may want to download the official client. The client and installation instructions are [here](https://github.com/wiresboy-exe/streamToolsDesktop).

## Downloads
[Download .QMOD](https://github.com/EnderdracheLP/streamer-tools/releases/latest)

[Download .SO](https://wiresdev.ga/projects/bs/streamer-tools/download/libstreamer-tools.so)

## Documentation
### Retrieving data
You have 2 methods to get the data which streamer-tools gives you:
a) Make a normal GET request to `http://[Your Quests ip]:3501`
b) Connect via a WebSocket to `http://[Your Quests ip]:3502`. 4 bytes will be sent which give you the length of the response (as int) and then you can read the rest of the data. The WebSocket will currently close after the data has been read so you have to connect a second time.

### Json format
Example response:
```json
{
    "location": 5,              // 0 = Menu, 1 = in song, 2 = mp in song, 3 = tutorial, 4 = campaign, 5 = in mp lobby
    "isPractice": false,        // If they are practice mode
    "paused": false,            // If the game is paused
    "time": 39,                 // Time of the song (in seconds)
    "endTime": 92,              // Length of the song (in seconds)
    "score": 91605,             // Score
    "rank": "B",                // Rank
    "combo": 0,                 // Combo
    "energy": 0,                // Health (0.0 to 1.0)
    "accuracy": 0.5321077108383179,            // accuracy (0.0 to 1.0)
    "levelName": "(can you) understand me?",   // Song name
    "levelSubName": "",                        // Song Sub name
    "levelAuthor": "Sotarks",                  // Mapper
    "songAuthor": "Komiya Mao",                // Song artist
    "id": "custom_level_DF27F79D91778CC141315B457D621D249C11DC6B", // Song id (remove "custom_level_" to get hash)
    "difficulty": 0,         // Selected difficulty (0 = easy, 1 = normal, 2 = hard, 3 = expert, 4 = expert +)
    "bpm": 185,              // bpm of the selected song
    "njs": 13,               // njs of the selected song
    "players": 1,            // Players in current mp lobby
    "maxPlayers": 5,         // max players of current mp lobby
    "mpGameId": "P7GZM",     // GameId of current multiplayer lobby
    "mpGameIdShown": false   // If the Game Id is shown in game (true) or as ***** (false)
}
```
Currently all values only update and do not reset (What I mean is that after you go out of a mp lobby the game id will still be there)

## Credits
For making the original Quest Discord Presence which this is build on.
* [Lauriethefish](https://github.com/Lauriethefish)

Credits.
* [MadMagic](https://github.com/madmagic007), [zoller27osu](https://github.com/zoller27osu), [Sc2ad](https://github.com/Sc2ad) and [jakibaki](https://github.com/jakibaki) - [beatsaber-hook](https://github.com/sc2ad/beatsaber-hook)
