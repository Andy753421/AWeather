/*
 * Copyright (C) 2010-2011 Andy Spencer <andy753421@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Taken from: http://www.weather.gov/wwamap-prd/faq.php */

#include "alert-info.h"

AlertInfo alert_info[] = {
	/* Warnings */
	//                                                                  hide  poly
	// title                            category    abbr          prio  | cur |  color
	{"Ashfall Warning"                , "warning",  "Ashfall",    72  , 0, 0, 0, {169, 169, 169}},  // darkgray
	{"Avalanche Warning"              , "warning",  "Avalanche",  21  , 0, 0, 0, {30 , 144, 255}},  // dodgerblue
	{"Blizzard Warning"               , "warning",  "Blizzard",   12  , 0, 0, 0, {255, 69 , 0  }},  // orangered
	{"Civil Danger Warning"           , "warning",  "Cvl Danger", 7   , 0, 0, 0, {255, 182, 193}},  // lightpink
	{"Coastal Flood Warning"          , "warning",  "C Flood",    24  , 0, 0, 0, {34 , 139, 34 }},  // forestgreen
	{"Dust Storm Warning"             , "warning",  "Dust Storm", 32  , 0, 0, 0, {255, 228, 196}},  // bisque
	{"Earthquake Warning"             , "warning",  "Earthquake", 22  , 0, 0, 0, {139, 69 , 19 }},  // saddlebrown
	{"Excessive Heat Warning"         , "warning",  "Heat",       31  , 0, 0, 0, {199, 21 , 133}},  // mediumvioletred
	{"Extreme Cold Warning"           , "warning",  "Cold",       44  , 0, 0, 0, {0  , 0  , 255}},  // blue
	{"Extreme Wind Warning"           , "warning",  "Ext Wind",   3   , 0, 0, 1, {255, 20 , 147}},  // deeppink
	{"Fire Warning"                   , "warning",  "Fire",       7   , 0, 0, 0, {160, 82 , 45 }},  // sienna
	{"Flash Flood Warning"            , "warning",  "F Flood",    5   , 0, 0, 1, {139, 0  , 0  }},  // darkred
	{"Flood Warning"                  , "warning",  "Flood",      5   , 0, 0, 1, {0  , 255, 0  }},  // lime
	{"Freeze Warning"                 , "warning",  "Freeze",     45  , 0, 0, 0, {0  , 255, 255}},  // cyan
	{"Gale Warning"                   , "warning",  "Gale",       40  , 0, 0, 0, {221, 160, 221}},  // plum
	{"Hard Freeze Warning"            , "warning",  "H Freeze",   45  , 0, 0, 0, {0  , 0  , 255}},  // blue
	{"Hazardous Materials Warning"    , "warning",  "Haz Mat",    7   , 0, 0, 0, {75 , 0  , 130}},  // indigo
	{"Hazardous Seas Warning"         , "warning",  "Haz Seas",   67  , 0, 0, 0, {221, 160, 221}},  // plum
	{"Heavy Freezing Spray Warning"   , "warning",  "Fz Spray",   63  , 0, 0, 0, {0  , 191, 255}},  // deepskyblue
	{"Heavy Snow Warning"             , "warning",  "Heavy Snow", 15  , 0, 0, 0, {138, 43 , 226}},  // blueviolet
	{"High Surf Warning"              , "warning",  "High Surf",  28  , 0, 0, 0, {34 , 139, 34 }},  // forestgreen
	{"High Wind Warning"              , "warning",  "High Wind",  17  , 0, 0, 0, {218, 165, 32 }},  // goldenrod
	{"Hurricane Force Wind Warning"   , "warning",  "Hur F Wind", 9   , 0, 0, 0, {205, 92 , 92 }},  // westernred
	{"Hurricane Warning"              , "warning",  "Hurricane",  9   , 0, 0, 0, {220, 20 , 60 }},  // crimson
	{"Hurricane Wind Warning"         , "warning",  "Hur Wind",   9   , 0, 0, 0, {205, 92 , 92 }},  // westernred
	{"Ice Storm Warning"              , "warning",  "Ice Storm",  13  , 0, 0, 0, {139, 0  , 139}},  // darkmagenta
	{"Inland Hurricane Warning"       , "warning",  "I Hurr",     9   , 0, 0, 0, {220, 20 , 60 }},  // crimson
	{"Inland Tropical Storm Warning"  , "warning",  "I Trop Stm", 14  , 0, 0, 0, {178, 34 , 34 }},  // firebrick
	{"Lake Effect Snow Warning"       , "warning",  "Lake Snow",  30  , 0, 0, 0, {0  , 139, 139}},  // darkcyan
	{"Lakeshore Flood Warning"        , "warning",  "L Flood",    25  , 0, 0, 0, {34 , 139, 34 }},  // forestgreen
	{"Law Enforcement Warning"        , "warning",  "Law Enf.",   8   , 0, 0, 0, {192, 192, 192}},  // silver
	{"Nuclear Power Plant Warning"    , "warning",  "Nuke Pwr",   7   , 0, 0, 0, {75 , 0  , 130}},  // indigo
	{"Radiological Hazard Warning"    , "warning",  "Radiology",  7   , 0, 0, 0, {75 , 0  , 130}},  // indigo
	{"Red Flag Warning"               , "warning",  "Red Flag",   47  , 0, 0, 0, {255, 20 , 147}},  // deeppink
	{"Severe Thunderstorm Warning"    , "warning",  "Svr Storm",  4   , 0, 0, 1, {255, 165, 0  }},  // orange
	{"Shelter In Place Warning"       , "warning",  "Shelter",    6   , 0, 0, 0, {250, 128, 114}},  // salmon
	{"Sleet Warning"                  , "warning",  "Sleet",      29  , 0, 0, 0, {135, 206, 235}},  // skyblue
	{"Special Marine Warning"         , "warning",  "Sp Marine",  11  , 0, 0, 1, {255, 165, 0  }},  // orange
	{"Storm Warning"                  , "warning",  "Storm",      13  , 0, 0, 0, {148, 0  , 211}},  // darkviolet
	{"Tornado Warning"                , "warning",  "Tornado",    2   , 0, 0, 1, {255, 0  , 0  }},  // red
	{"Tropical Storm Warning"         , "warning",  "Trop Storm", 14  , 0, 0, 0, {178, 34 , 34 }},  // firebrick
	{"Tropical Storm Wind Warning"    , "warning",  "Trop Wind",  9   , 0, 0, 0, {178, 34 , 34 }},  // firebrick
	{"Tsunami Warning"                , "warning",  "Tsunami",    1   , 0, 0, 0, {253, 99 , 71 }},  // tomato
	{"Typhoon Warning"                , "warning",  "Typhoon",    10  , 0, 0, 0, {220, 20 , 60 }},  // crimson
	{"Volcano Warning"                , "warning",  "Volcano",    23  , 0, 0, 0, {47 , 79 , 79 }},  // darkslategray
	{"Wind Chill Warning"             , "warning",  "Wind Chill", 43  , 0, 0, 0, {176, 196, 222}},  // lightsteelblue
	{"Winter Storm Warning"           , "warning",  "Wntr Storm", 16  , 0, 0, 0, {255, 105, 180}},  // hotpink

	/* Advisorys */
	{"Air Stagnation Advisory"        , "advisory", "Air Stag",   76  , 0, 0, 0, {128, 128, 128}},  // gray
	{"Ashfall Advisory"               , "advisory", "Ashfall",    73  , 0, 0, 0, {105, 105, 105}},  // dimgray
	{"Blowing Dust Advisory"          , "advisory", "Blow Dust",  71  , 0, 0, 0, {189, 183, 107}},  // darkkhaki
	{"Blowing Snow Advisory"          , "advisory", "Blow Snow",  50  , 0, 0, 0, {173, 216, 230}},  // lightblue
	{"Brisk Wind Advisory"            , "advisory", "Brsk Wind",  66  , 0, 0, 0, {216, 191, 216}},  // thistle
	{"Coastal Flood Advisory"         , "advisory", "C Flood",    58  , 0, 0, 0, {124, 252, 0  }},  // lawngreen
	{"Dense Fog Advisory"             , "advisory", "Fog",        68  , 0, 0, 0, {112, 128, 144}},  // slategray
	{"Dense Smoke Advisory"           , "advisory", "Smoke",      64  , 0, 0, 0, {240, 230, 140}},  // khaki
	{"Flood Advisory"                 , "advisory", "Flood",      57  , 0, 0, 1, {0  , 255, 127}},  // springgreen
	{"Freezing Drizzle Advisory"      , "advisory", "Fz Drzl",    51  , 0, 0, 0, {218, 112, 214}},  // orchid
	{"Freezing Fog Advisory"          , "advisory", "Fz Fog",     74  , 0, 0, 0, {0  , 128, 128}},  // teal
	{"Freezing Rain Advisory"         , "advisory", "Fz Rain",    51  , 0, 0, 0, {218, 112, 214}},  // orchid
	{"Freezing Spray Advisory"        , "advisory", "Fz Spray",   75  , 0, 0, 0, {0  , 191, 255}},  // deepskyblue
	{"Frost Advisory"                 , "advisory", "Frost",      72  , 0, 0, 0, {100, 149, 237}},  // cornflowerblue
	{"Heat Advisory"                  , "advisory", "Heat",       56  , 0, 0, 0, {255, 127, 80 }},  // coral
	{"High Surf Advisory"             , "advisory", "Surf",       60  , 0, 0, 0, {186, 85 , 211}},  // mediumorchid
	{"Hydrologic Advisory"            , "advisory", "Hydro",      57  , 0, 0, 0, {0  , 255, 127}},  // springgreen
	{"Lake Effect Snow Advisory"      , "advisory", "Lake Snow",  54  , 0, 0, 0, {72 , 209, 204}},  // mediumturquoise
	{"Lake Wind Advisory"             , "advisory", "Lake Wind",  69  , 0, 0, 0, {210, 180, 140}},  // tan
	{"Lakeshore Flood Advisory"       , "advisory", "L Flood",    58  , 0, 0, 0, {124, 252, 0  }},  // lawngreen
	{"Low Water Advisory"             , "advisory", "Low Water",  77  , 0, 0, 0, {165, 42 , 42 }},  // brown
	{"Sleet Advisory"                 , "advisory", "Sleet",      52  , 0, 0, 0, {123, 104, 238}},  // mediumslateblue
	{"Small Craft Advisory"           , "advisory", "S Craft",    65  , 0, 0, 0, {216, 191, 216}},  // thistle
	{"Small Stream Flood Advisory"    , "advisory", "Flood",      57  , 0, 0, 1, {0  , 255, 127}},  // springgreen
	{"Snow and Blowing Snow Advisory" , "advisory", "S/B Snow",   50  , 0, 0, 0, {176, 224, 230}},  // powderblue
	{"Tsunami Advisory"               , "advisory", "Tsunami",    42  , 0, 0, 0, {210, 105, 30 }},  // chocolate
	{"Wind Advisory"                  , "advisory", "Wind",       66  , 0, 0, 0, {210, 180, 140}},  // tan
	{"Wind Chill Advisory"            , "advisory", "Wind Chill", 55  , 0, 0, 0, {175, 238, 238}},  // paleturquoise
	{"Winter Weather Advisory"        , "advisory", "Winter Wx",  53  , 0, 0, 0, {123, 104, 238}},  // mediumslateblue
	{"Lake Effect Snow and Blowing Snow Advisory"
	                                  , "advisory", "L/S Snow",   54  , 0, 0, 0, {72 , 209, 204}},  // mediumturquoise
	{"Urban and Small Stream Flood Advisory"
	                                  , "advisory", "Flood",      57  , 0, 0, 1, {0  , 255, 127}},  // springgreen
	{"Arroyo and Small Stream Advisory"
	                                  , "advisory", "Flood",      57  , 0, 0, 1, {0  , 255, 127}},  // springgreen


	/* Watches */
	{"Tornado Watch"                  , "watch",    "Tornado",    33  , 0, 0, 0, {255, 255, 0  }},  // yellow
	{"Severe Thunderstorm Watch"      , "watch",    "Svr Storm",  35  , 0, 0, 0, {219, 112, 147}},  // palevioletred
	{"Avalanche Watch"                , "watch",    "Avalanche",  79  , 0, 0, 0, {244, 164, 96 }},  // sandybrown
	{"Blizzard Watch"                 , "watch",    "Blizzard",   80  , 0, 0, 0, {173, 255, 47 }},  // greenyellow
	{"Coastal Flood Watch"            , "watch",    "C Flood",    84  , 0, 0, 0, {102, 205, 170}},  // mediumaquamarine
	{"Excessive Heat Watch"           , "watch",    "Heat",       87  , 0, 0, 0, {128, 0  , 0  }},  // maroon
	{"Extreme Cold Watch"             , "watch",    "Ext Cold",   88  , 0, 0, 0, {0  , 0  , 255}},  // blue
	{"Fire Weather Watch"             , "watch",    "Fire Wx",    93  , 0, 0, 0, {255, 222, 173}},  // navajowhite
	{"Flash Flood Watch"              , "watch",    "F Flood",    37  , 0, 0, 0, {50 , 205, 50 }},  // limegreen
	{"Flood Watch"                    , "watch",    "Flood",      37  , 0, 0, 0, {46 , 139, 87 }},  // seagreen
	{"Freeze Watch"                   , "watch",    "Freeze",     91  , 0, 0, 0, {65 , 105, 225}},  // royalblue
	{"Gale Watch"                     , "watch",    "Gale",       1000, 0, 0, 0, {255, 192, 203}},  // pink
	{"Hard Freeze Watch"              , "watch",    "H Freeze",   91  , 0, 0, 0, {65 , 105, 225}},  // royalblue
	{"Hazardous Seas Watch"           , "watch",    "Haz Seas",   1000, 0, 0, 0, {72 , 61 , 139}},  // darkslateblue
	{"Heavy Freezing Spray Watch"     , "watch",    "Fz Spray",   1000, 0, 0, 0, {188, 143, 143}},  // rosybrown
	{"High Wind Watch"                , "watch",    "Wind",       86  , 0, 0, 0, {184, 134, 11 }},  // darkgoldenrod
	{"Hurricane Force Wind Watch"     , "watch",    "Hur Wind",   1000, 0, 0, 0, {153, 50 , 204}},  // darkorchid
	{"Hurricane Watch"                , "watch",    "Hurricane",  48  , 0, 0, 0, {255, 0  , 255}},  // magenta
	{"Hurricane Wind Watch"           , "watch",    "Hur F Wind", 1000, 0, 0, 0, {255, 160, 122}},  // lightsalmon
	{"Lake Effect Snow Watch"         , "watch",    "Lake Snow",  1000, 0, 0, 0, {135, 206, 250}},  // lightskyblue
	{"Lakeshore Flood Watch"          , "watch",    "L Flood",    84  , 0, 0, 0, {102, 205, 170}},  // mediumaquamarine
	{"Storm Watch"                    , "watch",    "Storm",      81  , 0, 0, 0, {238, 130, 238}},  // violet
	{"Tropical Storm Watch"           , "watch",    "Trop Storm", 81  , 0, 0, 0, {240, 128, 128}},  // lightcoral
	{"Tropical Storm Wind Watch"      , "watch",    "Trop Wind",  1000, 0, 0, 0, {240, 128, 128}},  // lightcoral
	{"Typhoon Watch"                  , "watch",    "Typhoon",    48  , 0, 0, 0, {255, 0  , 255}},  // magenta
	{"Wind Chill Watch"               , "watch",    "Wind Chill", 89  , 0, 0, 0, {95 , 158, 160}},  // cadetblue
	{"Winter Storm Watch"             , "watch",    "Wntr Storm", 83  , 0, 0, 0, {70 , 130, 180}},  // steelblue

	/* Statements */
	{"Coastal Flood Statement"        , "other",    "C Flood",    97  , 0, 0, 0, {107, 142, 35 }},  // olivedrab
	{"Flash Flood Statement"          , "other",    "F Flood",    39  , 0, 0, 1, {154, 205, 50 }},  // yellowgreen
	{"Flood Statement"                , "other",    "Flood",      39  , 0, 0, 1, {0  , 255, 0  }},  // lime
	{"Hurricane Statement"            , "other",    "Hurricane",  49  , 0, 0, 0, {147, 112, 219}},  // mediumpurple
	{"Lakeshore Flood Statement"      , "other",    "L Flood",    98  , 0, 0, 0, {107, 142, 35 }},  // olivedrab
	{"Marine Weather Statement"       , "other",    "Marine Wx",  11  , 0, 0, 1, {255, 239, 213}},  // peachpuff
	{"Rip Current Statement"          , "other",    "Rip Curnt",  1000, 0, 0, 0, {64 , 224, 208}},  // turquoise
	{"Severe Weather Statement"       , "other",    "Severe Wx",  38  , 0, 0, 1, {0  , 255, 255}},  // aqua
	{"Special Weather Statement"      , "other",    "Special Wx", 99  , 0, 0, 0, {255, 228, 181}},  // moccasin
	{"Typhoon Statement"              , "other",    "Typhoon",    49  , 0, 0, 0, {147, 112, 219}},  // mediumpurple

	/* Misc */
	{"Air Quality Alert"              , "other",    "Air Qual",   1000, 0, 0, 0, {128, 128, 128}},  // gray

	{"Extreme Fire Danger"            , "other",    "Fire Dngr",  94  , 0, 0, 0, {233, 150, 122}},  // darksalmon

	{"Child Abduction Emergency"      , "other",    "Child Abd",  95  , 0, 0, 0, {255, 215, 0  }},  // gold
	{"Local Area Emergency"           , "other",    "Local",      78  , 0, 0, 0, {192, 192, 192}},  // silver

	{"Short Term Forecast"            , "other",    "S Term Fc",  102 , 0, 0, 0, {152, 251, 152}},  // palegreen

	{"Evacuation Immediate"           , "other",    "Evacuate",   6   , 0, 0, 0, {127, 255, 0  }},  // chartreuse

	{"Administrative Message"         , "other",    "Admin",      103 , 0, 0, 0, {255, 255, 255}},  // white
	{"Civil Emergency Message"        , "other",    "Civil Em",   7   , 0, 0, 0, {255, 182, 193}},  // lightpink

	{"911 Telephone Outage"           , "other",    "911 Out",    96  , 0, 0, 0, {192, 192, 192}},  // silver

	{"Hazardous Weather Outlook"      , "other",    "Haz Wx Ol",  101 , 0, 0, 0, {238, 232, 170}},  // palegoldenrod
	{"Hydrologic Outlook"             , "other",    "Hydro Ol",   1000, 0, 0, 0, {144, 238, 144}},  // lightgreen

	{"Test"                           , "other",    "Test",       104 , 0, 0, 0, {240, 255, 255}},  // azure

	/* End of list */
	{NULL, NULL, NULL, 0, 0, 0, 0, {0, 0, 0}},
};

AlertInfo *alert_info_find(gchar *title)
{
	for (int i = 0; alert_info[i].title; i++)
		if (g_str_has_prefix(title, alert_info[i].title))
			return &alert_info[i];
	return NULL;
}
