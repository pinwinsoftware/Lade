# === Lite Engine Data Editor Manual ===

LED files are used to store assets for games made with Lite Engine. The name "LED" stands for "Lite Engine Data"

LED file support was introduced in version 0.1.8.0 of Lite Engine. Previous versions do not support LED files.


# === How LED files are being used ===

LED files are used to store further data type:

* LEM files (Lite Engine Main) - The main file of the game that sets the levels order and where should the player be spawned.
* LWD Files (Lite World) - Format of the maps, aka levels in the game. LWD files can be created using LWD Editor.
* LES files (Lite Engine Script) - Script that store definitions for in-game entities.
* LSP files (Lite Sprite) - Sprites that are being rendered inside the game.

LED file can store any file data you wish, but those 4 file types are currently the only files that are being implemented in Lite Engine.


# === How to use LED Editor ===

The LED Editor uses a command-line interface. All operations are being done via commands supported in the LED Editor.

List of supported commands:

* HELP - Shows a list of supported commands.
* OPEN - Opens an existing LED file. After entering this command, you must enter the path to the LED file.
* NEW  - Creates a fresh, new LED file. After entering this command, you must enter the name of the new LED file. LED files created with Lade are being stored at "led" folder
* INFO - Shows information about Lade.
* EXIT - Exits the Lade.

After creating or opening a LED file, new commands become available:

* HELP    - Shows a list of supported commands during editing.
* INCLUDE - Includes an existing file of any type in the LED archive from your computer. After entering this command, you must type the path to the file.
* EXPORT  - Exports a file inside of LED archive into your computer. After entering this command, you must enter the name of the file, then, path where you want to export it.
* CREATE  - Creates a new file inside of LED archive. After entering this command, enter the file format, then name of the file.
* DELETE  - Deletes a file from LED archive. After entering this command, enter the name of the file.
* BINARY  - Views a file from LED archive as binary. After entering this command, enter the name of the file.
* TEXT    - Views a file from LED archive as text. After entering this command, enter the name of the file.
* VIEW    - Displays a list of all files inside of LED archive
* EDIT    - Edits a file from LED archive as text. After entering this command, enter the name of the file.


# === Description of Lite Engine file formats ===

Lite Engine uses several file formats.

LED is the container format. It can store multiple files inside a
single archive.

LEM, LWD, LES, and LSP are individual asset formats that can be
stored inside an LED file.

# = LED =

Archive that stores assets for games made with Lite Engine.
Currently it stores levels, player positions and angles on levels, level orders, entities stats, entities sprites, weapons stats and weapons sprites.
LED files have a limit of 4gb since its a 32-bit file.

An LED file consists of three main parts:

Header

File entries

File data

The header describes the LED file itself.
The file entries describe each file stored inside the container.
The file data contains the actual bytes of those files.

The header contains:
- The LED signature ("LED1")
- The number of files stored in the archive

Each file entry contains:
- File name
- File size
- File offset

The offset specifies where the file's data begins inside the LED file.
The size specifies how many bytes should be read from that location.


The general structure is:

----------------------
Header
- Signature
- File count         
----------------------
File Entry #1
- File name
- File size
- File offset
----------------------
File Entry #2
- File name
- File size
- File offset         
----------------------
 File data #1         
----------------------
 File data #2         
----------------------


# === LED Signature ===

Every LED file begins with the four-byte signature:

LED1

"LED" identifies the container format.
"1" identifies version 1 of the format.

The signature allows Lite Engine to determine whether a file is
an LED archive and which version of the format it uses.

# = LEM =

LEM file is a file that stores a script of levels order and players positions.

Every led file has to contain a file called GAME.LEM, else the game wouldn't work.

The general structure is:

```
map MAP01
{
    Level_id = 1;
    Name = "First Level";
    Level = MAP01.LWD;
    Next = MAP02;

    x = 3.5; // Player start x
    y = 11.5; // Player start y
    angle = 270; // Player start angle
    ammo = 100; // Player's ammo
    health = 100; //Player's health
}

map MAP02
{
    Level_id = 2;
    Name = "Second Level";
    Level = MAP02.LWD;
    // if this is the last level do not add "Next"

    x = 15;
    y = 3.5;
    angle = 180;
}
```

# = LWD =

A map file of the level in Lite Engine.

Each character in an LWD file represents one tile in the map.

The character's value corresponds to the ID of an object defined by an LES file.

For example:

```
11111
1   1
1 2 1
1   1
11111
```
* 1 = Wall
* 2 = Entity

You can change the id of the objects inside of their LES files

# = LES =

A script that defines an object for use in a Lite Engine game.

Currently, can creates 3 object types:

geometry:

Static Walls/Blocks that collide with player and entities. Currently Lite Engine only supports one type of walls, and les files that define geometry are only used to make the walls being displayed in objects list in LWD Editor.

The general structure is:

```
geometry Wall
{
     Id = 1;

     texture
     {
         WALL.LSP; // Sprite for walls are only used to be displayed as icon in LWD Editor
     }
}
```

entity:

Entities that can be enemy, collectible item, or level exit.
Whatever entity type you want to select, you should define it's "Type" inside of les file.

Entity types that Lite Engine supports right now:

* ENEMY - An npc that will chase and damage player after player traps in his point of view.
* CORPSE - Used as corpses of enemies, but can be used as map decoration without hitbox.
* FIREBALL - An entity that being created by other entity that includes ranged attack in it's properties. It is not intended for being created as a separated entity in les file.
* COLLECTIBLE - Required for items that can be collected.
* AMMO - Collectible entity that restores player's ammo.
* MEDKIT - Collectible entity that heals player.
* EXIT - Entity that calls the levels finish after collision with player.

If you wish you can add more classes inside of Lite Engine source code, then use it inside of les files.

The general structure is:

```
entity Monster
{
    Id = 3;
    Width = 16; // Width of the sprite
    Height = 16; // Height of the sprite
    Health = 50; // Entity's health
    Damage = 1d2+3; // Entity's melee damage (stored as dice) 
    Speed = 1.5; // Entity's movement speed
    Type = ENEMY; // Entity type
    Ranged = true; // Attack type

    // Sprites of the entity in different states
    animation
    {
        state IDLE
        {
            MONSTER_STANDING_HERE_I_REALIZE.LSP; //Note: the entities that don't move, e.g. ammo packs, medkits use idle sprite by default
        }
        state CHASE
        {
            MONSTER_CHASE.LSP;
        }
        state ATTACK
        {
            MONSTER_ATTACK.LSP;
        }
        state DEATH
        {
            CORPSE.LSP;
        }
    }

    // This part is only needed for enemy's with melee attack
    ranged_attack 
    {
        Width = 16; // Fireball sprite width
        Height = 16; // Fireball sprite height
        Speed = 5; // Fireball movement speed
        Damage = 2d3+7; // Fireball damage
        FIREBALL.LSP; // Fireball sprite
    }
}
```

weapon:

Weapons used by player to attack enemies. The weapons can be melee and ranged. Ranged weapons use ammunition, melee not.

The general structure is:

```
weapon Pistol
{
    Id = 2; // The weapon ids are used to select them during the game. If weapon id is 1, then it's corresponding key is "number 1" on keyboard
    Width = 16; // Sprite width
    Height = 8; // Sprite height
    Damage = 3d7+7; // Weapon Damage
    Speed = 0.24; // Reload speed
    Ranged = true; // Attack type

    animation
    {
        state IDLE
        {
            PISTOL.LSP;
        }
        state ATTACK
        {
            PISTOL_ATTACK.LSP;
        }
    }
}
```

The different object classes will change the properties of the les file, but overall structure is:

```
class_name Entity_Name
{
    <Any properties>

    animation
    {
        state State_Name
        {
            Sprite_Used_During_This_State.LSP;
        }
    }
}
```

# = LSP =

Lite Engine Sprite format, used to store entities and weapons sprites using ascii characters.

Current version of Lite engine reads this 3 symbols:

* ' ' - Empty space of the sprite, that draws nothing, but can be overdrawn by another object behind the entity.
* '0' - Void that doesn't let any other sprites behind overdraw this area.
* '1' - A solid pixel of the sprite.

LSP files can be created using LSP Editor app.


# === Credits ===

Developed by Larion Naumenko

Copytight © Pinwin Software 2026 All Rights Reserved
