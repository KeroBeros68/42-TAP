# 42-TAP

[Color palette]https://coolors.co/palette/606c38-283618-fefae0-dda15e-bc6c25
#606c38
#283618
#fefae0
#dda15e
#bc6c25

## Notes for the final README

- **NPC lookup (`Room::findNpc` in `game/room.cpp`)**: the team first wrote a simple version, where the player had to give the exact instance id of the NPC (for example `npc.guard#10`). The version in the code now was proposed and improved with the help of Claude (an AI assistant): a player can also designate an NPC by the id of its definition, its short id, its name, or one word of its name, in any case (`TALK guard`, `TALK Guard Hector`, `TALK hector`). The team reviewed and kept it.
