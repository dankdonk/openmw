/*
  Copyright (C) 2015, 2016 cc9cii

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  cc9cii cc9c@iinet.net.au

  Much of the information on the data structures are based on
  http://www.uesp.net/wiki/Tes5Mod:Mod_File_Format but also refined by trial & error.

  MKTAG macro was adapated from ScummVM.

*/
#ifndef ESM4_COMMON_H
#define ESM4_COMMON_H

#include <cstdint>
#include <string>

// From ScummVM's endianness.h but for little endian
#define MKTAG(a0,a1,a2,a3) ((std::uint32_t)((a0) | ((a1) << 8) | ((a2) << 16) | ((a3) << 24)))

namespace ESM4
{
    enum ESMVersions
    {
        VER_080 = 0x3f800000,
        VER_094 = 0x3f70a3d7,
        VER_170 = 0x3fd9999a
    };

    // Based on http://www.uesp.net/wiki/Tes5Mod:Mod_File_Format
    enum RecordTypes
    {
        REC_AACT = MKTAG('A','A','C','T'), // Action
        REC_ACHR = MKTAG('A','C','H','R'), // Actor Reference
        REC_ACTI = MKTAG('A','C','T','I'), // Activator
        REC_ADDN = MKTAG('A','D','D','N'), // Addon Node
        REC_ALCH = MKTAG('A','L','C','H'), // Potion
        REC_AMMO = MKTAG('A','M','M','O'), // Ammo
        REC_ANIO = MKTAG('A','N','I','O'), // Animated Object
        REC_APPA = MKTAG('A','P','P','A'), // Apparatus (probably unused)
        REC_ARMA = MKTAG('A','R','M','A'), // Armature (Model)
        REC_ARMO = MKTAG('A','R','M','O'), // Armor
        REC_ARTO = MKTAG('A','R','T','O'), // Art Object
        REC_ASPC = MKTAG('A','S','P','C'), // Acoustic Space
        REC_ASTP = MKTAG('A','S','T','P'), // Association Type
        REC_AVIF = MKTAG('A','V','I','F'), // Actor Values/Perk Tree Graphics
        REC_BOOK = MKTAG('B','O','O','K'), // Book
        REC_BPTD = MKTAG('B','P','T','D'), // Body Part Data
        REC_CAMS = MKTAG('C','A','M','S'), // Camera Shot
        REC_CELL = MKTAG('C','E','L','L'), // Cell
        REC_CLAS = MKTAG('C','L','A','S'), // Class
        REC_CLFM = MKTAG('C','L','F','M'), // Color
        REC_CLMT = MKTAG('C','L','M','T'), // Climate
        REC_CLOT = MKTAG('C','L','O','T'), // Clothing
        REC_COBJ = MKTAG('C','O','B','J'), // Constructible Object (recipes)
        REC_COLL = MKTAG('C','O','L','L'), // Collision Layer
        REC_CONT = MKTAG('C','O','N','T'), // Container
        REC_CPTH = MKTAG('C','P','T','H'), // Camera Path
        REC_CSTY = MKTAG('C','S','T','Y'), // Combat Style
        REC_DEBR = MKTAG('D','E','B','R'), // Debris
        REC_DIAL = MKTAG('D','I','A','L'), // Dialog Topic
        REC_DLBR = MKTAG('D','L','B','R'), // Dialog Branch
        REC_DLVW = MKTAG('D','L','V','W'), // Dialog View
        REC_DOBJ = MKTAG('D','O','B','J'), // Default Object Manager
        REC_DOOR = MKTAG('D','O','O','R'), // Door
        REC_DUAL = MKTAG('D','U','A','L'), // Dual Cast Data (possibly unused)
      //REC_ECZN = MKTAG('E','C','Z','N'), // Encounter Zone
        REC_EFSH = MKTAG('E','F','S','H'), // Effect Shader
        REC_ENCH = MKTAG('E','N','C','H'), // Enchantment
        REC_EQUP = MKTAG('E','Q','U','P'), // Equip Slot (flag-type values)
        REC_EXPL = MKTAG('E','X','P','L'), // Explosion
        REC_EYES = MKTAG('E','Y','E','S'), // Eyes
        REC_FACT = MKTAG('F','A','C','T'), // Faction
        REC_FLOR = MKTAG('F','L','O','R'), // Flora
        REC_FLST = MKTAG('F','L','S','T'), // Form List (non-leveled list)
        REC_FSTP = MKTAG('F','S','T','P'), // Footstep
        REC_FSTS = MKTAG('F','S','T','S'), // Footstep Set
        REC_FURN = MKTAG('F','U','R','N'), // Furniture
        REC_GLOB = MKTAG('G','L','O','B'), // Global Variable
        REC_GMST = MKTAG('G','M','S','T'), // Game Setting
        REC_GRAS = MKTAG('G','R','A','S'), // Grass
        REC_GRUP = MKTAG('G','R','U','P'), // Form Group
        REC_HAIR = MKTAG('H','A','I','R'), // Hair
      //REC_HAZD = MKTAG('H','A','Z','D'), // Hazard
        REC_HDPT = MKTAG('H','D','P','T'), // Head Part
        REC_IDLE = MKTAG('I','D','L','E'), // Idle Animation
        REC_IDLM = MKTAG('I','D','L','M'), // Idle Marker
        REC_IMAD = MKTAG('I','M','A','D'), // Image Space Modifier
        REC_IMGS = MKTAG('I','M','G','S'), // Image Space
        REC_INFO = MKTAG('I','N','F','O'), // Dialog Topic Info
        REC_INGR = MKTAG('I','N','G','R'), // Ingredient
        REC_IPCT = MKTAG('I','P','C','T'), // Impact Data
        REC_IPDS = MKTAG('I','P','D','S'), // Impact Data Set
        REC_KEYM = MKTAG('K','E','Y','M'), // Key
        REC_KYWD = MKTAG('K','Y','W','D'), // Keyword
        REC_LAND = MKTAG('L','A','N','D'), // Land
        REC_LCRT = MKTAG('L','C','R','T'), // Location Reference Type
        REC_LCTN = MKTAG('L','C','T','N'), // Location
        REC_LGTM = MKTAG('L','G','T','M'), // Lighting Template
        REC_LIGH = MKTAG('L','I','G','H'), // Light
        REC_LSCR = MKTAG('L','S','C','R'), // Load Screen
        REC_LTEX = MKTAG('L','T','E','X'), // Land Texture
        REC_LVLI = MKTAG('L','V','L','I'), // Leveled Item
        REC_LVLN = MKTAG('L','V','L','N'), // Leveled Actor
        REC_LVSP = MKTAG('L','V','S','P'), // Leveled Spell
        REC_MATO = MKTAG('M','A','T','O'), // Material Object
        REC_MATT = MKTAG('M','A','T','T'), // Material Type
        REC_MESG = MKTAG('M','E','S','G'), // Message
        REC_MGEF = MKTAG('M','G','E','F'), // Magic Effect
        REC_MISC = MKTAG('M','I','S','C'), // Misc. Object
        REC_MOVT = MKTAG('M','O','V','T'), // Movement Type
        REC_MSTT = MKTAG('M','S','T','T'), // Movable Static
        REC_MUSC = MKTAG('M','U','S','C'), // Music Type
        REC_MUST = MKTAG('M','U','S','T'), // Music Track
        REC_NAVI = MKTAG('N','A','V','I'), // Navigation (master data)
        REC_NAVM = MKTAG('N','A','V','M'), // Nav Mesh
        REC_NPC_ = MKTAG('N','P','C','_'), // Actor (NPC, Creature)
        REC_OTFT = MKTAG('O','T','F','T'), // Outfit
        REC_PACK = MKTAG('P','A','C','K'), // AI Package
        REC_PERK = MKTAG('P','E','R','K'), // Perk
        REC_PGRE = MKTAG('P','G','R','E'), // Placed grenade
        REC_PHZD = MKTAG('P','H','Z','D'), // Placed hazard
        REC_PROJ = MKTAG('P','R','O','J'), // Projectile
        REC_QUST = MKTAG('Q','U','S','T'), // Quest
        REC_RACE = MKTAG('R','A','C','E'), // Race / Creature type
        REC_REFR = MKTAG('R','E','F','R'), // Object Reference
        REC_REGN = MKTAG('R','E','G','N'), // Region (Audio/Weather)
        REC_RELA = MKTAG('R','E','L','A'), // Relationship
        REC_REVB = MKTAG('R','E','V','B'), // Reverb Parameters
        REC_RFCT = MKTAG('R','F','C','T'), // Visual Effect
        REC_SCEN = MKTAG('S','C','E','N'), // Scene
        REC_SCRL = MKTAG('S','C','R','L'), // Scroll
        REC_SGST = MKTAG('S','G','S','T'), // Sigil Stone
        REC_SHOU = MKTAG('S','H','O','U'), // Shout
        REC_SLGM = MKTAG('S','L','G','M'), // Soul Gem
        REC_SMBN = MKTAG('S','M','B','N'), // Story Manager Branch Node
        REC_SMEN = MKTAG('S','M','E','N'), // Story Manager Event Node
        REC_SMQN = MKTAG('S','M','Q','N'), // Story Manager Quest Node
        REC_SNCT = MKTAG('S','N','C','T'), // Sound Category
        REC_SNDR = MKTAG('S','N','D','R'), // Sound Reference
        REC_SOPM = MKTAG('S','O','P','M'), // Sound Output Model
        REC_SOUN = MKTAG('S','O','U','N'), // Sound
        REC_SPEL = MKTAG('S','P','E','L'), // Spell
        REC_SPGD = MKTAG('S','P','G','D'), // Shader Particle Geometry
        REC_STAT = MKTAG('S','T','A','T'), // Static
        REC_TACT = MKTAG('T','A','C','T'), // Talking Activator
        REC_TES4 = MKTAG('T','E','S','4'), // Plugin info
        REC_TREE = MKTAG('T','R','E','E'), // Tree
        REC_TXST = MKTAG('T','X','S','T'), // Texture Set
        REC_VTYP = MKTAG('V','T','Y','P'), // Voice Type
        REC_WATR = MKTAG('W','A','T','R'), // Water Type
        REC_WEAP = MKTAG('W','E','A','P'), // Weapon
        REC_WOOP = MKTAG('W','O','O','P'), // Word Of Power
        REC_WRLD = MKTAG('W','R','L','D'), // World Space
        REC_WTHR = MKTAG('W','T','H','R'), // Weather
        REC_ACRE = MKTAG('A','C','R','E'), // Placed Creature (Oblivion only?)
        REC_PGRD = MKTAG('P','G','R','D'), // Pathgrid (Oblivion only?)
        REC_ROAD = MKTAG('R','O','A','D')  // Road (Oblivion only?)
    };

    enum SubRecordTypes
    {
        // below appear in TES4 record
        SUB_HEDR = MKTAG('H','E','D','R'),
        SUB_CNAM = MKTAG('C','N','A','M'),
        SUB_SNAM = MKTAG('S','N','A','M'),
        SUB_MAST = MKTAG('M','A','S','T'),
        SUB_DATA = MKTAG('D','A','T','A'),
        SUB_ONAM = MKTAG('O','N','A','M'),
        SUB_INTV = MKTAG('I','N','T','V'),
        SUB_INCC = MKTAG('I','N','C','C'),
        SUB_OFST = MKTAG('O','F','S','T'), // Oblivion only?
        SUB_DELE = MKTAG('D','E','L','E'), // Oblivion only?


        // below appear in WRLD records
      //SUB_CNAM = MKTAG('C','N','A','M'),
      //SUB_DATA = MKTAG('D','A','T','A'),
        SUB_DNAM = MKTAG('D','N','A','M'),
        SUB_EDID = MKTAG('E','D','I','D'),
        SUB_FULL = MKTAG('F','U','L','L'),
        SUB_LTMP = MKTAG('L','T','M','P'),
        SUB_MHDT = MKTAG('M','H','D','T'),
        SUB_MNAM = MKTAG('M','N','A','M'),
        SUB_MODL = MKTAG('M','O','D','L'),
        SUB_NAM0 = MKTAG('N','A','M','0'),
        SUB_NAM2 = MKTAG('N','A','M','2'),
        SUB_NAM3 = MKTAG('N','A','M','3'),
        SUB_NAM4 = MKTAG('N','A','M','4'),
        SUB_NAM9 = MKTAG('N','A','M','9'),
        SUB_NAMA = MKTAG('N','A','M','A'),
      //SUB_OFST = MKTAG('O','F','S','T'),
      //SUB_ONAM = MKTAG('O','N','A','M'),
        SUB_PNAM = MKTAG('P','N','A','M'),
        SUB_RNAM = MKTAG('R','N','A','M'),
        SUB_TNAM = MKTAG('T','N','A','M'),
        SUB_UNAM = MKTAG('U','N','A','M'),
        SUB_WCTR = MKTAG('W','C','T','R'),
        SUB_WNAM = MKTAG('W','N','A','M'),
        SUB_XEZN = MKTAG('X','E','Z','N'),
        SUB_XLCN = MKTAG('X','L','C','N'),
      //SUB_XWEM = MKTAG('X','W','E','M'),
        SUB_XXXX = MKTAG('X','X','X','X'),
        SUB_ZNAM = MKTAG('Z','N','A','M'),
        SUB_MODT = MKTAG('M','O','D','T'),
      //SUB_SNAM = MKTAG('S','N','A','M'), // Oblivion only?
        SUB_ICON = MKTAG('I','C','O','N'), // Oblivion only?


        // below appear in NAVI records
      //SUB_EDID = MKTAG('E','D','I','D'),
        SUB_NVER = MKTAG('N','V','E','R'),
        SUB_NVMI = MKTAG('N','V','M','I'),
        SUB_NVPP = MKTAG('N','V','P','P'),
        SUB_NVSI = MKTAG('N','V','S','I'),

        // below appear in NAVM records
        SUB_NVNM = MKTAG('N','V','N','M'),
      //SUB_ONAM = MKTAG('O','N','A','M'),
      //SUB_PNAM = MKTAG('P','N','A','M'),
        SUB_NNAM = MKTAG('N','N','A','M'),

        // below appear in CELL records
      //SUB_EDID = MKTAG('E','D','I','D'),
      //SUB_FULL = MKTAG('F','U','L','L'),
      //SUB_DATA = MKTAG('D','A','T','A'),
        SUB_XCLC = MKTAG('X','C','L','C'),
        SUB_XCLL = MKTAG('X','C','L','L'),
        SUB_TVDT = MKTAG('T','V','D','T'),
      //SUB_MHDT = MKTAG('M','H','D','T'),
        SUB_XCGD = MKTAG('X','C','G','D'),
      //SUB_LTMP = MKTAG('L','T','M','P'),
        SUB_LNAM = MKTAG('L','N','A','M'),
        SUB_XCLW = MKTAG('X','C','L','W'),
        SUB_XNAM = MKTAG('X','N','A','M'),
        SUB_XCLR = MKTAG('X','C','L','R'),
      //SUB_XLCN = MKTAG('X','L','C','N'),
        SUB_XWCS = MKTAG('X','W','C','S'),
        SUB_XWCN = MKTAG('X','W','C','N'),
        SUB_XWCU = MKTAG('X','W','C','U'),
        SUB_XCWT = MKTAG('X','C','W','T'),
        SUB_XOWN = MKTAG('X','O','W','N'),
        SUB_XILL = MKTAG('X','I','L','L'),
        SUB_XWEM = MKTAG('X','W','E','M'),
        SUB_XCCM = MKTAG('X','C','C','M'),
        SUB_XCAS = MKTAG('X','C','A','S'),
      //SUB_XEZN = MKTAG('X','E','Z','N'),
        SUB_XCMO = MKTAG('X','C','M','O'),
        SUB_XCIM = MKTAG('X','C','I','M'),
        SUB_XCMT = MKTAG('X','C','M','T'), // Oblivion only?
        SUB_XRNK = MKTAG('X','R','N','K'), // Oblivion only?
        SUB_XGLB = MKTAG('X','G','L','B'), // Oblivion only?

        // below appear in LAND records
      //SUB_DATA = MKTAG('D','A','T','A'),
        SUB_VNML = MKTAG('V','N','M','L'),
        SUB_VHGT = MKTAG('V','H','G','T'),
        SUB_VCLR = MKTAG('V','C','L','R'),
        SUA_BTXT = MKTAG('B','T','X','T'),
        SUB_ATXT = MKTAG('A','T','X','T'),
        SUB_VTXT = MKTAG('V','T','X','T'),
        SUB_VTEX = MKTAG('V','T','E','X'),

        // below appear in LTEX records
      //SUB_EDID = MKTAG('E','D','I','D'),
      //SUB_ICON = MKTAG('I','C','O','N'), // Oblivion only?
      //SUB_SNAM = MKTAG('S','N','A','M'), // Oblivion only?
      //SUB_TNAM = MKTAG('T','N','A','M'),
      //SUB_MNAM = MKTAG('M','N','A','M'),
        SUB_HNAM = MKTAG('H','N','A','M'),
        SUB_GNAM = MKTAG('G','N','A','M'),

        // below appear in REGN records
      //SUB_EDID = MKTAG('E','D','I','D'),
      //SUB_ICON = MKTAG('I','C','O','N'),
      //SUB_WNAM = MKTAG('W','N','A','M'),
        SUB_RCLR = MKTAG('R','C','L','R'),
        SUB_RPLI = MKTAG('R','P','L','I'),
        SUB_RPLD = MKTAG('R','P','L','D'),
        SUB_RDAT = MKTAG('R','D','A','T'),
        SUB_RDMD = MKTAG('R','D','M','D'), // Oblivion only?
        SUB_RDSD = MKTAG('R','D','S','D'), // Oblivion only?
        SUB_RDGS = MKTAG('R','D','G','S'), // Oblivion only?
        SUB_RDMO = MKTAG('R','D','M','O'),
        SUB_RDSA = MKTAG('R','D','S','A'),
        SUB_RDWT = MKTAG('R','D','W','T'),
        SUB_RDOT = MKTAG('R','D','O','T'),
        SUB_RDMP = MKTAG('R','D','M','P'),

        // below appear in STAT records
      //SUB_EDID = MKTAG('E','D','I','D'),
      //SUB_MODL = MKTAG('M','O','D','L'),
        SUB_MODB = MKTAG('M','O','D','B'),
        SUB_OBND = MKTAG('O','B','N','D'),
      //SUB_MODT = MKTAG('M','O','D','T'),
      //SUB_MNAM = MKTAG('M','N','A','M'),
      //SUB_DNAM = MKTAG('D','N','A','M'),
        SUB_MODS = MKTAG('M','O','D','S'),

        // below appear in REFR records
      //SUB_EDID = MKTAG('E','D','I','D'),
      //SUB_FULL = MKTAG('F','U','L','L'),
      //SUB_DATA = MKTAG('D','A','T','A'),
        SUB_NAME = MKTAG('N','A','M','E'),
        SUB_XMRK = MKTAG('X','M','R','K'),
        SUB_FNAM = MKTAG('F','N','A','M'),
      //SUB_XOWN = MKTAG('X','O','W','N'),
      //SUB_XRNK = MKTAG('X','R','N','K'),
      //SUB_XGLB = MKTAG('X','G','L','B'),
        SUB_XSCL = MKTAG('X','S','C','L'),
        SUB_XTEL = MKTAG('X','T','E','L'),
        SUB_XTRG = MKTAG('X','T','R','G'),
        SUB_XSED = MKTAG('X','S','E','D'),
        SUB_XLOD = MKTAG('X','L','O','D'),
        SUB_XPCI = MKTAG('X','P','C','I'),
        SUB_XLOC = MKTAG('X','L','O','C'),
        SUB_XESP = MKTAG('X','E','S','P'),
        SUB_XLCM = MKTAG('X','L','C','M'),
        SUB_XRTM = MKTAG('X','R','T','M'),
        SUB_XACT = MKTAG('X','A','C','T'),
        SUB_XCNT = MKTAG('X','C','N','T'),
      //SUB_TNAM = MKTAG('T','N','A','M'),
      //SUB_ONAM = MKTAG('O','N','A','M'),
        SUB_VMAD = MKTAG('V','M','A','D'),
        SUB_XPRM = MKTAG('X','P','R','M'),
        SUB_XMBO = MKTAG('X','M','B','O'),
        SUB_XPOD = MKTAG('X','P','O','D'),
        SUB_XRMR = MKTAG('X','R','M','R'),
      //SUB_LNAM = MKTAG('L','N','A','M'),
        SUB_INAM = MKTAG('I','N','A','M'),
        SUB_SCHR = MKTAG('S','C','H','R'),
        SUB_XLRM = MKTAG('X','L','R','M'),
        SUB_XRGD = MKTAG('X','R','G','D'),
        SUB_XRDS = MKTAG('X','R','D','S'),
        SUB_XEMI = MKTAG('X','E','M','I'),
        SUB_XLIG = MKTAG('X','L','I','G'),
        SUB_XALP = MKTAG('X','A','L','P'),
        SUB_XNDP = MKTAG('X','N','D','P'),
        SUB_XAPD = MKTAG('X','A','P','D'),
        SUB_XAPR = MKTAG('X','A','P','R'),
        SUB_XLIB = MKTAG('X','L','I','B'),
        SUB_XLKR = MKTAG('X','L','L','R'),
        SUB_XLRT = MKTAG('X','L','R','T'),
        SUB_XCVL = MKTAG('X','C','V','L'),
        SUB_XCVR = MKTAG('X','C','V','R'),
        SUB_XCZA = MKTAG('X','C','Z','A'),
        SUB_XCZC = MKTAG('X','C','Z','C'),
      //SUB_XEZN = MKTAG('X','E','Z','N'),
        SUB_XFVC = MKTAG('X','F','V','C'),
        SUB_XHTW = MKTAG('X','H','T','W'),
        SUB_XIS2 = MKTAG('X','I','S','2'),
        SUB_XMBR = MKTAG('X','M','B','R'),
        SUB_XCCP = MKTAG('X','C','C','P'),
        SUB_XPWR = MKTAG('X','P','W','R'),
        SUB_XTRI = MKTAG('X','T','R','I'),
        SUB_XATR = MKTAG('X','A','T','R'),
      //SUB_XWCN = MKTAG('X','W','C','N'),
      //SUB_XWCU = MKTAG('X','W','C','U'),
        SUB_XPRD = MKTAG('X','P','R','D'),
        SUB_XPPA = MKTAG('X','P','P','A'),
        SUB_PDTO = MKTAG('P','D','T','O'),
        SUB_XLRL = MKTAG('X','L','R','L'),

        // below appear in CONT records
      //SUB_MODB = MKTAG('M','O','D','B'),
      //SUB_MODT = MKTAG('M','O','D','T'),
      //SUB_MODS = MKTAG('M','O','D','S'),
        SUB_QNAM = MKTAG('Q','N','A','M'),
      //SUB_VMAD = MKTAG('V','M','A','D'),
        SUB_COCT = MKTAG('C','O','C','T'),
        SUB_COED = MKTAG('C','O','E','D'),
        SUB_CNTO = MKTAG('C','N','T','O'),
        SUB_SCRI = MKTAG('S','C','R','I'),

        // below appear in ANIO records
        SUB_BNAM = MKTAG('B','N','A','M'),

        // below appear in ARMO records
        SUB_BMDT = MKTAG('B','M','D','T'),
      //SUB_SCRI = MKTAG('S','C','R','I'),
        SUB_MOD2 = MKTAG('M','O','D','2'),
        SUB_MOD3 = MKTAG('M','O','D','3'),
        SUB_MOD4 = MKTAG('M','O','D','4'),
      //SUB_MODB = MKTAG('M','O','D','B'),
        SUB_MO2B = MKTAG('M','O','2','B'),
        SUB_MO3B = MKTAG('M','O','3','B'),
        SUB_MO4B = MKTAG('M','O','4','B'),
      //SUB_MODT = MKTAG('M','O','D','T'),
        SUB_MO2T = MKTAG('M','O','2','T'),
        SUB_MO3T = MKTAG('M','O','3','T'),
        SUB_MO4T = MKTAG('M','O','4','T'),
        SUB_ANAM = MKTAG('A','N','A','M'),
        SUB_ENAM = MKTAG('E','N','A','M'),
        SUB_ICO2 = MKTAG('I','C','O','2'),

        // below appear in NPC_ records
      //SUB_MODB = MKTAG('M','O','D','B'),
        SUB_ACBS = MKTAG('A','C','B','S'),
      //SUB_SNAM = MKTAG('S','N','A','M'),
      //SUB_INAM = MKTAG('I','N','A','M'),
      //SUB_RNAM = MKTAG('R','N','A','M'),
        SUB_SPLO = MKTAG('S','P','L','O'),
      //SUB_SCRI = MKTAG('S','C','R','I'),
      //SUB_CNTO = MKTAG('C','N','T','O'),
        SUB_AIDT = MKTAG('A','I','D','T'),
        SUB_PKID = MKTAG('P','K','I','D'),
      //SUB_CNAM = MKTAG('C','N','A','M'),
      //SUB_HNAM = MKTAG('H','N','A','M'),
      //SUB_LNAM = MKTAG('L','N','A','M'),
      //SUB_ENAM = MKTAG('E','N','A','M'),
        SUB_HCLR = MKTAG('H','C','L','R'),
      //SUB_ZNAM = MKTAG('Z','N','A','M'),
        SUB_FGGS = MKTAG('F','G','G','S'),
        SUB_FGGA = MKTAG('F','G','G','A'),
      //SUB_FNAM = MKTAG('F','N','A','M'),
        SUB_FGTS = MKTAG('F','G','T','S'),
        SUB_KFFZ = MKTAG('K','F','F','Z'),

        // below appear in FLOR records
        SUB_PFIG = MKTAG('P','F','I','G'),
        SUB_PFPC = MKTAG('P','F','P','C'),

        // below appear in ACHR records
        SUB_XHRS = MKTAG('X','H','R','S'),
        SUB_XMRC = MKTAG('X','M','R','C'),

        // below appear in SOUN records
        SUB_SNDD = MKTAG('S','N','D','D'),
        SUB_SNDX = MKTAG('S','N','D','X'),

        SUB_DESC = MKTAG('D','E','S','C'),

        SUB_ENIT = MKTAG('E','N','I','T'),
        SUB_EFID = MKTAG('E','F','I','D'),
        SUB_EFIT = MKTAG('E','F','I','T'),
        SUB_SCIT = MKTAG('S','C','I','T')
    };

    // Based on http://www.uesp.net/wiki/Tes5Mod:Mod_File_Format#Groups
    enum GroupType
    {
        Grp_RecordType           = 0,
        Grp_WorldChild           = 1,
        Grp_InteriorCell         = 2,
        Grp_InteriorSubCell      = 3,
        Grp_ExteriorCell         = 4,
        Grp_ExteriorSubCell      = 5,
        Grp_CellChild            = 6,
        Grp_TopicChild           = 7,
        Grp_CellPersistentChild  = 8,
        Grp_CellTemporaryChild   = 9,
        Grp_CellVisibleDistChild = 10
    };

    // Based on http://www.uesp.net/wiki/Tes5Mod:Mod_File_Format#Records
    enum RecordFlag
    {
        Rec_ESM        = 0x00000001, // (TES4 record only) Master (ESM) file.
        Rec_Deleted    = 0x00000020, // Deleted
        Rec_Constant   = 0x00000040, // Constant
        Rec_HiddenLMap = 0x00000040, // (REFR) Hidden From Local Map (Needs Confirmation: Related to shields)
        Rec_Localized  = 0x00000080, // (TES4 record only) Is localized. This will make Skyrim load the
                                     //   .STRINGS, .DLSTRINGS, and .ILSTRINGS files associated with the mod.
                                     //   If this flag is not set, lstrings are treated as zstrings.
        Rec_FireOff    = 0x00000080, // (PHZD) Turn off fire
        Rec_UpdateAnim = 0x00000100, // Must Update Anims
        Rec_NoAccess   = 0x00000100, // (REFR) Inaccessible
        Rec_Hidden     = 0x00000200, // (REFR) Hidden from local map
        Rec_StartDead  = 0x00000200, // (ACHR) Starts dead /(REFR) MotionBlurCastsShadows
        Rec_Persistent = 0x00000400, // Quest item / Persistent reference
        Rec_DispMenu   = 0x00000400, // (LSCR) Displays in Main Menu
        Rec_Disabled   = 0x00000800, // Initially disabled
        Rec_Ignored    = 0x00001000, // Ignored
        Rec_DistVis    = 0x00008000, // Visible when distant
        Rec_RandAnim   = 0x00010000, // (ACTI) Random Animation Start
        Rec_Danger     = 0x00020000, // (ACTI) Dangerous / Off limits (Interior cell)
                                     //   Dangerous Can't be set withough Ignore Object Interaction
        Rec_Compressed = 0x00040000, // Data is compressed
        Rec_CanNotWait = 0x00080000, // Can't wait
        Rec_IgnoreObj  = 0x00100000, // (ACTI) Ignore Object Interaction
                                     //   Ignore Object Interaction Sets Dangerous Automatically
        Rec_Marker     = 0x00800000, // Is Marker
        Rec_Obstacle   = 0x02000000, // (ACTI) Obstacle / (REFR) No AI Acquire
        Rec_NavMFilter = 0x04000000, // NavMesh Gen - Filter
        Rec_NavMBBox   = 0x08000000, // NavMesh Gen - Bounding Box
        Rec_ExitToTalk = 0x10000000, // (FURN) Must Exit to Talk
        Rec_Refected   = 0x10000000, // (REFR) Reflected By Auto Water
        Rec_ChildUse   = 0x20000000, // (FURN/IDLM) Child Can Use
        Rec_NoHavok    = 0x20000000, // (REFR) Don't Havok Settle
        Rec_NavMGround = 0x40000000, // NavMesh Gen - Ground
        Rec_NoRespawn  = 0x40000000, // (REFR) NoRespawn
        Rec_MultiBound = 0x80000000  // (REFR) MultiBound
    };

    enum Flags
    {
        // NVNM/NVMI world space field
        FLG_Interior   = 0x00000000, // cell formid follows
        FLG_Tamriel    = 0x0000003c, // grid info follows, possibly Tamriel?
        FLG_Morrowind  = 0x01380000, // grid info follows, probably Skywind

        // NVMI island flags (not certain)
        FLG_Island     = 0x00000020,
        FLG_Modified   = 0x00000000, // not island
        FLG_Unmodified = 0x00000040  // not island
    };

    typedef std::uint32_t FormId;

#pragma pack(push, 1)
    // NOTE: the label field of a group is not reliable (http://www.uesp.net/wiki/Tes4Mod:Mod_File_Format)
    union GroupLabel
    {
        std::uint32_t value;     // formId, blockNo or raw int representation of type
        char recordType[4];      // record type in ascii
        std::int16_t grid[2];    // grid y, x (note the reverse order)
    };

    union TypeId
    {
        std::uint32_t value;
        char name[4];            // record type in ascii
    };

    struct GroupTypeHeader
    {
        std::uint32_t typeId;
        std::uint32_t groupSize; // includes the 24 bytes of header (i.e. this struct)
        GroupLabel    label;     // format based on type
        std::int32_t  type;
        std::uint16_t stamp;     // & 0xff for day, & 0xff00 for months since Dec 2002 (i.e. 1 = Jan 2003)
        std::uint16_t unknown;
        std::uint16_t version;   // not in Oblivion
        std::uint16_t unknown2;  // not in Oblivion
    };

    struct RecordTypeHeader
    {
        std::uint32_t typeId;
        std::uint32_t dataSize;  // does *not* include 24 bytes of header
        std::uint32_t flags;
        FormId        id;
        std::uint32_t revision;
        std::uint16_t version;  // not in Oblivion
        std::uint16_t unknown;  // not in Oblivion
    };

    union RecordHeader
    {
        struct GroupTypeHeader  group;
        struct RecordTypeHeader record;
    };

    struct SubRecordHeader
    {
        std::uint32_t typeId;
        std::uint16_t dataSize;
    };

    // Grid, CellGrid and Vertex are shared by NVMI(NAVI) and NVNM(NAVM)

    struct Grid
    {
        std::int16_t x;
        std::int16_t y;
    };

    union CellGrid
    {
        FormId cellId;
        Grid   grid;
    };

    struct Vector3
    {
        float x;
        float y;
        float z;
    };

    typedef Vector3 Vertex;

    struct Position
    {
        Vector3 pos;
        Vector3 rot; // angles are in radian, rz applied first and rx applied last
    };
#pragma pack(pop)

    // For pretty printing GroupHeader labels
    std::string printLabel(const GroupLabel& label, const std::uint32_t type);

    std::string printName(const std::uint32_t typeId);

    void formIdToString(FormId formId, std::string& str);
    std::string formIdToString(FormId formId);

    void gridToString(std::int16_t x, std::int16_t y, std::string& str);
}

#endif // ESM4_COMMON_H
