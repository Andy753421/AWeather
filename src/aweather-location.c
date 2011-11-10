/*
 * Copyright (C) 2009-2011 Andy Spencer <andy753421@gmail.com>
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

#include <config.h>
#include <gtk/gtk.h>

#include "aweather-location.h"

city_t cities[] = {
	{LOCATION_STATE, NULL,   "Alabama",           {0,        0,       0}, 0.0},
	{LOCATION_NOP,   "KMXX", "Maxwell AFB",       {32.533,  -85.786,  0}, 0.1},
	{LOCATION_CITY,  "KBMX", "Birmingham",        {33.1722, -86.766,  0}, 0.3},
	{LOCATION_CITY,  "KHTX", "N Alabama",         {34.9306, -86.0833, 0}, 0.3},
	{LOCATION_CITY,  "KMOB", "Mobile",            {30.6794, -88.2397, 0}, 0.3},
	{LOCATION_NOP,   "KEOX", "Fort Rucker",       {31.456,  -85.455,  0}, 0.1},

	{LOCATION_NOP,   NULL,   "Alaska",            {0,        0,       0}, 0.0},
	{LOCATION_NOP,   "KABC", "Bethel",            {60.78,   -161.87,  0}, 0.1},
	{LOCATION_NOP,   "KAHG", "Nikiski",           {60.72,   -151.35,  0}, 0.1},
	{LOCATION_NOP,   "KAIH", "Middleton Island",  {59.45,   -146.3,   0}, 0.1},
	{LOCATION_NOP,   "KAKC", "King Salmon",       {58.67,   -156.62,  0}, 0.1},
	{LOCATION_NOP,   "KAEC", "Nome",              {64.5,    -165.28,  0}, 0.1},
	{LOCATION_NOP,   "KAPD", "Pedro Dome",        {65.03,   -147.5,   0}, 0.1},
	{LOCATION_NOP,   "KACG", "Juneau",            {56.85,   -135.52,  0}, 0.1},

	{LOCATION_STATE, NULL,   "Arizona",           {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KEMX", "Tucson",            {31.8936, -110.63,  0}, 0.3},
	{LOCATION_CITY,  "KFSX", "Flagstaff",         {34.5744, -111.198, 0}, 0.5},
	{LOCATION_CITY,  "KIWA", "Phoenix",           {33.2892, -111.67,  0}, 0.8},
	{LOCATION_CITY,  "KYUX", "Yuma",              {32.4953, -114.657, 0}, 0.3},

	{LOCATION_STATE, NULL,   "Arkansas",          {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KSRX", "W Arkansas",        {35.28,   -94.35,   0}, 0.1},
	{LOCATION_CITY,  "KLZK", "Little Rock",       {34.8367, -92.2622, 0}, 0.8},

	{LOCATION_STATE, NULL,   "California",        {0,        0,       0}, 0.0},
	{LOCATION_NOP,   "KEYX", "Edwards AFB",       {35.08,   -117.55,  0}, 0.1},
	{LOCATION_NOP,   "KVBX", "Orcutt Oil Field",  {34.83,   -120.38,  0}, 0.1},
	{LOCATION_CITY,  "KBBX", "Beale AFB",         {39.4961, -121.632, 0}, 0.3},
	{LOCATION_CITY,  "KBHX", "Eureka",            {40.4983, -124.292, 0}, 0.3},
	{LOCATION_CITY,  "KDAX", "Sacramento",        {38.5011, -121.678, 0}, 0.5},
	{LOCATION_CITY,  "KHNX", "Hanford",           {36.3142, -119.632, 0}, 0.3},
	{LOCATION_CITY,  "KMUX", "San Francisco",     {37.155,  -121.898, 0}, 0.8},
	{LOCATION_CITY,  "KNKX", "San Diego",         {32.9189, -117.042, 0}, 0.5},
	{LOCATION_CITY,  "KSOX", "Santa Ana Mtns",    {33.8178, -117.636, 0}, 0.3},
	{LOCATION_CITY,  "KVTX", "Los Angeles",       {34.4117, -119.18,  0}, 1.5},

	{LOCATION_STATE, NULL,   "Colorado",          {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KFTG", "Denver/Boulder",    {39.7867, -104.546, 0}, 0.8},
	{LOCATION_CITY,  "KGJX", "Grand Junction",    {39.0622, -108.214, 0}, 0.3},
	{LOCATION_CITY,  "KPUX", "Pueblo",            {38.4594, -104.181, 0}, 0.3},

	{LOCATION_STATE, NULL,   "Delaware",          {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KDOX", "Dover AFB",         {38.8256, -75.44,   0}, 0.3},

	{LOCATION_STATE, NULL,   "Florida",           {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KAMX", "Miami",             {25.6111, -80.4128, 0}, 0.8},
	{LOCATION_CITY,  "KBYX", "Key West",          {24.5975, -81.7031, 0}, 0.3},
	{LOCATION_CITY,  "KEVX", "NW Florida",        {30.5644, -85.9214, 0}, 0.3},
	{LOCATION_CITY,  "KJAX", "Jacksonville",      {30.4847, -81.7019, 0}, 0.5},
	{LOCATION_CITY,  "KMLB", "Melbourne",         {28.1133, -80.6542, 0}, 0.3},
	{LOCATION_CITY,  "KTBW", "Tampa Bay Area",    {27.7056, -82.4017, 0}, 0.5},
	{LOCATION_CITY,  "KTLH", "Tallahassee",       {30.3975, -84.3289, 0}, 0.3},

	{LOCATION_STATE, NULL,   "Georgia",           {0,        0,       0}, 0.0},
	{LOCATION_NOP,   "KVAX", "Tallahassee",       {30.88,   -83.0,    0}, 0.1},
	{LOCATION_CITY,  "KFFC", "Atlanta",           {33.3636, -84.5658, 0}, 0.8},
	{LOCATION_CITY,  "KJGX", "Robins AFB",        {32.675,  -83.3511, 0}, 0.3},

	{LOCATION_NOP,   NULL,   "Guam",              {0,        0,       0}, 0.0},
	{LOCATION_NOP,   "KGUA", "Barrigada Comm.",   {13.45,    144.8,   0}, 0.1},

	{LOCATION_STATE, NULL,   "Hawaii",            {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "PHKI", "Kauai",             {21.88,   -159.55,  0}, 0.5},
	{LOCATION_CITY,  "PHKM", "Kohala",            {20.12,   -155.77,  0}, 0.5},
	{LOCATION_CITY,  "PHMO", "Molokai",           {21.12,   -157.17,  0}, 0.8},
	{LOCATION_CITY,  "PHWA", "Hawaii",            {19.08,   -155.57,  0}, 1.5},

	{LOCATION_STATE, NULL,   "Idaho",             {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KCBX", "Boise",             {43.4908, -116.236, 0}, 0.5},
	{LOCATION_CITY,  "KSFX", "Pocatello",         {43.1058, -112.686, 0}, 0.3},

	{LOCATION_STATE, NULL,   "Illinois",          {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KILX", "Central IL",        {40.1506, -89.3369, 0}, 0.3},
	{LOCATION_CITY,  "KLOT", "Chicago",           {41.6047, -88.0847, 0}, 0.8},

	{LOCATION_STATE, NULL,   "Indiana",           {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KIND", "Indianapolis",      {39.707,  -86.2803, 0}, 0.5},
	{LOCATION_CITY,  "KIWX", "N Indiana",         {41.35,   -85.7,    0}, 0.3},
	{LOCATION_CITY,  "KVWX", "Evansville",        {38.27,   -87.72,   0}, 0.3},

	{LOCATION_STATE, NULL,   "Iowa",              {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KDMX", "Des Moines",        {41.731,  -93.7228, 0}, 0.3},
	{LOCATION_CITY,  "KDVN", "Quad Cities",       {41.611,  -90.5808, 0}, 0.3},

	{LOCATION_STATE, NULL,   "Kansas",            {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KDDC", "Dodge City",        {37.760,  -99.9686, 0}, 0.3},
	{LOCATION_CITY,  "KGLD", "Goodland",          {39.366,  -101.701, 0}, 0.3},
	{LOCATION_CITY,  "KICT", "Wichita",           {37.654,  -97.4428, 0}, 0.5},
	{LOCATION_CITY,  "KTWX", "Topeka",            {38.996,  -96.2325, 0}, 0.5},

	{LOCATION_STATE, NULL,   "Kentucky",          {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KJKL", "Jackson",           {37.590,  -83.3131, 0}, 0.5},
	{LOCATION_CITY,  "KLVX", "Louisville",        {37.975,  -85.9439, 0}, 0.5},
	{LOCATION_CITY,  "KPAH", "Paducah",           {37.068,  -88.7719, 0}, 0.3},
	{LOCATION_NOP,   "KHPX", "Hopkinsville",      {36.733,  -87.281,  0}, 0.1},

	{LOCATION_STATE, NULL,   "Louisiana",         {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KLCH", "Lake Charles",      {30.125,  -93.2158, 0}, 0.3},
	{LOCATION_CITY,  "KLIX", "New Orleans",       {30.336,  -89.8256, 0}, 0.8},
	{LOCATION_NOP,   "KPOE", "Fort Polk",         {31.155,  -92.9758, 0}, 0.1},
	{LOCATION_CITY,  "KSHV", "Shreveport",        {32.450,  -93.8414, 0}, 0.3},

	{LOCATION_STATE, NULL,   "Maine",             {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KCBW", "Caribou",           {46.039,  -67.8067, 0}, 0.3},
	{LOCATION_CITY,  "KGYX", "Portland",          {43.891,  -70.2567, 0}, 0.3},

	{LOCATION_STATE, NULL,   "Maryland",          {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KLWX", "Baltimore",         {38.975,  -77.4778, 0}, 0.5},

	{LOCATION_STATE, NULL,   "Massachusetts",     {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KBOX", "Boston",            {41.955,  -71.1369, 0}, 0.8},

	{LOCATION_STATE, NULL,   "Michigan",          {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KAPX", "Gaylord",           {44.907,  -84.7197, 0}, 0.3},
	{LOCATION_CITY,  "KDTX", "Detroit",           {42.699,  -83.4717, 0}, 0.5},
	{LOCATION_CITY,  "KGRR", "Grand Rapids",      {42.893,  -85.5447, 0}, 0.3},
	{LOCATION_CITY,  "KMQT", "Marquette",         {46.531,  -87.5483, 0}, 0.3},

	{LOCATION_STATE, NULL,   "Minnesota",         {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KDLH", "Duluth",            {46.836,  -92.2097, 0}, 0.3},
	{LOCATION_CITY,  "KMPX", "Minneapolis",       {44.848,  -93.5656, 0}, 0.5},

	{LOCATION_STATE, NULL,   "Mississippi",       {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KDGX", "Jackson/Brandon",   {32.275,  -89.98,   0}, 0.5},
	{LOCATION_CITY,  "KGWX", "Columbus AFB",      {33.896,  -88.3289, 0}, 0.3},

	{LOCATION_STATE, NULL,   "Missouri",          {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KEAX", "Kansas City",       {38.810,  -94.2644, 0}, 0.5},
	{LOCATION_CITY,  "KLSX", "St. Louis",         {38.698,  -90.6828, 0}, 1.5},
	{LOCATION_CITY,  "KSGF", "Springfield",       {37.235,  -93.4006, 0}, 0.3},

	{LOCATION_STATE, NULL,   "Montana",           {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KBLX", "Billings",          {45.853,  -108.607, 0}, 0.3},
	{LOCATION_CITY,  "KGGW", "Glasgow",           {48.206,  -106.625, 0}, 0.5},
	{LOCATION_CITY,  "KMSX", "Missoula",          {47.041,  -113.986, 0}, 0.3},
	{LOCATION_CITY,  "KTFX", "Great Falls",       {47.459,  -111.385, 0}, 0.3},

	{LOCATION_STATE, NULL,   "Nebraska",          {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KLNX", "North Platte",      {41.957,  -100.576, 0}, 0.3},
	{LOCATION_CITY,  "KOAX", "Omaha",             {41.320,  -96.3667, 0}, 0.5},
	{LOCATION_CITY,  "KUEX", "Hastings",          {40.320,  -98.4419, 0}, 0.3},

	{LOCATION_STATE, NULL,   "Nevada",            {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KESX", "Las Vegas",         {35.701,  -114.891, 0}, 0.8},
	{LOCATION_CITY,  "KLRX", "Elko",              {40.739,  -116.803, 0}, 0.3},
	{LOCATION_CITY,  "KRGX", "Reno",              {39.755,  -119.462, 0}, 0.5},

	{LOCATION_STATE, NULL,   "New Jersey",        {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KDIX", "Mt. Holly",         {39.946,  -74.4108, 0}, 0.3},

	{LOCATION_STATE, NULL,   "New Mexico",        {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KABX", "Albuquerque",       {35.149,  -106.824, 0}, 0.5},
	{LOCATION_CITY,  "KFDX", "Cannon AFB",        {34.635,  -103.63,  0}, 0.3},
	{LOCATION_CITY,  "KHDX", "Holloman AFB",      {33.076,  -106.123, 0}, 0.3},

	{LOCATION_STATE, NULL,   "New York",          {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KTYX", "Montague",          {43.751,  -75.675,  0}, 0.3},
	{LOCATION_CITY,  "KBGM", "Binghamton",        {42.199,  -75.9847, 0}, 0.3},
	{LOCATION_CITY,  "KBUF", "Buffalo",           {42.948,  -78.7367, 0}, 0.3},
	{LOCATION_CITY,  "KENX", "Albany",            {42.586,  -74.0639, 0}, 0.3},
	{LOCATION_CITY,  "KOKX", "New York City",     {40.865,  -72.8639, 0}, 1.5},

	{LOCATION_STATE, NULL,   "North Carolina",    {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KLTX", "Wilmington",        {33.989,  -78.4289, 0}, 0.3},
	{LOCATION_CITY,  "KMHX", "Morehead City",     {34.776,  -76.8761, 0}, 0.3},
	{LOCATION_CITY,  "KRAX", "Raleigh",           {35.665,  -78.4897, 0}, 0.5},

	{LOCATION_STATE, NULL,   "North Dakota",      {0,        0,       0}, 0.0},
	{LOCATION_NOP,   "KMBX", "Minot",             {48.388,  -100.859, 0}, 0.1},
	{LOCATION_CITY,  "KBIS", "Bismarck",          {46.770,  -100.761, 0}, 0.5},
	{LOCATION_CITY,  "KMVX", "Grand Forks",       {47.527,  -97.3256, 0}, 0.3},

	{LOCATION_STATE, NULL,   "Ohio",              {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KCLE", "Cleveland",         {41.413,  -81.8597, 0}, 0.5},
	{LOCATION_CITY,  "KILN", "Cincinnati",        {39.420,  -83.8217, 0}, 0.8},

	{LOCATION_STATE, NULL,   "Oklahoma",          {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KFDR", "Frederick",         {34.362,  -98.9764, 0}, 0.3},
	{LOCATION_CITY,  "KINX", "Tulsa",             {36.17,   -95.5647, 0}, 0.5},
	{LOCATION_CITY,  "KTLX", "Oklahoma City",     {35.333,  -97.2778, 0}, 0.8},
	{LOCATION_CITY,  "KVNX", "Vance AFB",         {36.740,  -98.1278, 0}, 0.3},
	{LOCATION_CITY,  "KOUN", "Norman (KOUN)",     {35.246,  -97.4721, 0}, 0.1},

	{LOCATION_STATE, NULL,   "Oregon",            {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KMAX", "Medford",           {42.081,  -122.717, 0}, 0.3},
	{LOCATION_CITY,  "KPDT", "Pendleton",         {45.690,  -118.853, 0}, 0.3},
	{LOCATION_CITY,  "KRTX", "Portland",          {45.714,  -122.966, 0}, 0.8},

	{LOCATION_STATE, NULL,   "Pennsylvania",      {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KCCX", "State College",     {40.923,  -78.0036, 0}, 0.3},
	{LOCATION_NOP,   "KPHI", "Philadelphia",      {39.946,  -74.4108, 0}, 0.1},
	{LOCATION_CITY,  "KPBZ", "Pittsburgh",        {40.531,  -80.2183, 0}, 0.5},

	{LOCATION_STATE, NULL,   "Puerta Rico",       {0,        0,       0}, 0.0},
	{LOCATION_NOP,   "KJUA", "Cayey",             {18.1,    -66.07,   0}, 0.1},
	{LOCATION_CITY,  "TJUA", "San Juan",          {18.12,   -66.08,   0}, 0.8},

	{LOCATION_STATE, NULL,   "South Carolina",    {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KCAE", "Columbia",          {33.948,  -81.1183, 0}, 0.5},
	{LOCATION_CITY,  "KCLX", "Charleston",        {32.655,  -81.0419, 0}, 0.3},
	{LOCATION_CITY,  "KGSP", "Greenville",        {34.866,  -82.22,   0}, 0.3},

	{LOCATION_STATE, NULL,   "South Dakota",      {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KABR", "Aberdeen",          {45.455,  -98.4131, 0}, 0.3},
	{LOCATION_CITY,  "KFSD", "Sioux falls",       {43.587,  -96.7294, 0}, 0.5},
	{LOCATION_CITY,  "KUDX", "Rapid City",        {44.12,  -102.83,   0}, 0.3},

	{LOCATION_STATE, NULL,   "Tennessee",         {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KMRX", "Knoxville",         {36.168,  -83.4017, 0}, 1.5},
	{LOCATION_CITY,  "KNQA", "Memphis",           {35.344,  -89.8733, 0}, 0.3},
	{LOCATION_CITY,  "KOHX", "Nashville",         {36.247,  -86.5625, 0}, 0.5},

	{LOCATION_STATE, NULL,   "Texas",             {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KAMA", "Amarillo",          {35.233,  -101.709, 0}, 0.5},
	{LOCATION_CITY,  "KBRO", "Brownsville",       {25.916,  -97.4189, 0}, 0.3},
	{LOCATION_CITY,  "KCRP", "Corpus Christi",    {27.784,  -97.5111, 0}, 0.3},
	{LOCATION_CITY,  "KDFX", "Laughlin AFB",      {29.272,  -100.281, 0}, 0.3},
	{LOCATION_CITY,  "KDYX", "Dyess AFB",         {32.538,  -99.2542, 0}, 0.3},
	{LOCATION_CITY,  "KEPZ", "El Paso",           {31.873,  -106.698, 0}, 0.3},
	{LOCATION_CITY,  "KEWX", "Austin",            {29.703,  -98.0283, 0}, 1.5},
	{LOCATION_CITY,  "KFWS", "Dallas",            {32.573,  -97.3031, 0}, 0.8},
	{LOCATION_CITY,  "KGRK", "Central Texas",     {30.721,  -97.3831, 0}, 0.3},
	{LOCATION_CITY,  "KHGX", "Houston",           {29.471,  -95.0792, 0}, 0.5},
	{LOCATION_CITY,  "KLBB", "Lubbock",           {33.653,  -101.814, 0}, 0.3},
	{LOCATION_CITY,  "KMAF", "Midland/Odessa",    {31.943,  -102.189, 0}, 0.3},
	{LOCATION_CITY,  "KSJT", "San Angelo",        {31.371,  -100.493, 0}, 0.3},

	{LOCATION_STATE, NULL,   "Utah",              {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KMTX", "Salt Lake City",    {41.258,  -112.442, 0}, 0.8},
	{LOCATION_CITY,  "KICX", "Cedar City",        {37.586,  -112.856, 0}, 0.3},

	{LOCATION_STATE, NULL,   "Vermont",           {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KCXX", "Burlington",        {44.511,  -73.1669, 0}, 0.3},

	{LOCATION_STATE, NULL,   "Virginia",          {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KAKQ", "Richmond",          {36.983,  -77.0072, 0}, 0.5},
	{LOCATION_CITY,  "KFCX", "Blacksburg",        {37.024,  -80.2739, 0}, 0.3},

	{LOCATION_STATE, NULL,   "Washington",        {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KATX", "Seattle",           {48.194,  -122.496, 0}, 1.5},
	{LOCATION_CITY,  "KOTX", "Spokane",           {47.680,  -117.627, 0}, 0.5},

	{LOCATION_STATE, NULL,   "West Virginia",     {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KRLX", "Charleston",        {38.311,  -81.7231, 0}, 0.5},

	{LOCATION_STATE, NULL,   "Wisconsin",         {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KARX", "La Crosse",         {43.822,  -91.1911, 0}, 0.3},
	{LOCATION_CITY,  "KGRB", "Green Bay",         {44.498,  -88.1114, 0}, 0.5},
	{LOCATION_CITY,  "KMKX", "Milwaukee",         {42.967,  -88.5506, 0}, 0.3},

	{LOCATION_STATE, NULL,   "Wyoming",           {0,        0,       0}, 0.0},
	{LOCATION_CITY,  "KCYS", "Cheyenne",          {41.151,  -104.806, 0}, 0.5},
	{LOCATION_CITY,  "KRIW", "Riverton",          {43.066,  -108.477, 0}, 0.3},

	{LOCATION_NOP,   NULL,   "Other",             {0,        0,       0}, 0.0},
	{LOCATION_NOP,   "DOP1", "DOP1",              {0,        0,       0}, 0.1},
	{LOCATION_NOP,   "FOP1", "FOP1",              {0,        0,       0}, 0.1},
	{LOCATION_NOP,   "NOP3", "NOP3",              {0,        0,       0}, 0.1},
	{LOCATION_NOP,   "NOP4", "NOP4",              {0,        0,       0}, 0.1},
	{LOCATION_NOP,   "ROP4", "ROP4",              {0,        0,       0}, 0.1},

	{LOCATION_END,   NULL,   NULL,                {0,        0,       0}, 0.0},
};
