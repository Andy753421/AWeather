#ifndef LEVEL2_H
#define LEVEL2_H

/* http://www.ncdc.noaa.gov/oa/radar/leveliidoc.html */
typedef struct {
	/* 24 bytes */
	char version[4];
	char unknown0[16];
	char station[4];
} __attribute__ ((packed)) level2_header_t;

typedef struct {
	/**
	 * Channel Terminal Manager:
	 *
	 * Archive II (the data tape) is a copy of messages or data packets
	 * prepared for transmission from the RDA to the RPG.  CTM information
	 * is attached to a message or data packet for checking data integrity
	 * during the transmission process and is of no importance to the base
	 * data (omit or read past these bytes).
	 */
	short ctm[6]; // ignore

	/**
	 * Message Header:
	 *
	 * This information is used to identify either base data or one of
	 * thirteen types of messages that may follow in bytes 28 - 2431.  This
	 * header includes the information indicated below:
	 */
	// Message size in halfwords measure from this halfword to the end of
	// record.
	unsigned short size;

	// Channel ID:
	//   0 = Non-Redundant Site
	//   1 = Redundant Site Channel 1
	//   2 = Redundant Site Channel 2
	unsigned char id;

	// Message type, where:
	//   1  = DIGITAL RADAR DATA (This message  may contain a combination of
	//        either reflectivity, aliased velocity, or spectrum width)
	//   2  = RDA STATUS DATA.
	//   3  = PERFORMANCE/MAINTENANCE DATA.
	//   4  = CONSOLE MESSAGE - RDA TO RPG.
	//   5  = MAINTENANCE LOG DATA.
	//   6  = RDA CONTROL COMMANDS.
	//   7  = VOLUME COVERAGE PATTERN.
	//   8  = CLUTTER CENSOR ZONES.
	//   9  = REQUEST FOR DATA.
	//   10 = CONSOLE MESSAGE - RPG TO RDA.
	//   11 = LOOP BACK TEST  - RDA TO RPG.
	//   12 = LOOP BACK TEST  - RPG TO RDA.
	//   13 = CLUTTER FILTER BYPASS MAP - RDA to RPG.
	//   14 = EDITED CLUTTER FILTER BYPASS MAP - RPG to RDA.
	unsigned char type;

	// I.D. Sequence = 0 to 7FFF, then roll over back to 0.
	unsigned short seq;

	// Modified Julian date starting from 1/1/70
	unsigned short gen_date;

	// Generation time of messages in milliseconds of day past midnight
	// (UTC). This time may be different than time listed in halfwords
	// 15-16 defined below.
	unsigned int   gen_time;

	// Number of message segments.  Messages larger than message size
	// (halfword 7 defined above) are segmented and recorded in separate
	// data packets.
	unsigned short num_seg;

	// Message segment number.
	unsigned short seg;

	/**
	 * Digital Radar Data Header:
         * 
	 * This information describes the date, time, azimuth, elevation, and
	 * type of base data included in the radial. This header includes the
	 * following information:
	 */
	// Collection time for this radial in milliseconds of the day from
	// midnight (UTC).
	unsigned int   coll_time;

	// Modified Julian date referenced from 1/1/70.
	unsigned short coll_date;

	// Unambiguous range (scaled: Value/10. = KM).
	unsigned short range;
	
	// Azimuth angle (coded: [Value/8.]*[180./4096.] = DEG). An azimuth of
	// "0 degrees" points to true north while "90 degrees" points east.
	// Rotation is always clockwise as viewed from above the radar.
	unsigned short angle;

	// Radial number within the elevation scan.
	unsigned short radial;

	// Radial status where:
	//   0 = START OF NEW ELEVATION.
	//   1 = INTERMEDIATE RADIAL.
	//   2 = END OF ELEVATION.
	//   3 = BEGINNING OF VOLUME SCAN.
	//   4 = END OF VOLUME SCAN.
	unsigned short rad_status;

	// Elevation angle (coded:[Value/8.]*[180./4096.] = DEG). An elevation
	// of "0 degree" is parallel to the pedestal base while "90 degrees" is
	// perpendicular to the pedestal base.
	unsigned short elev_angle;

	// RDA elevation number within the volume scan.
	unsigned short elev_num;

	// Range to first gate of reflectivity data (METERS). Range may be
	// negative to account for system delays in transmitter and/or receiver
	// components. 
	short          first_refl;

	// Range to first gate of Doppler data.  Doppler data - velocity and
	// spectrum width (METERS).  Range may be negative to account for
	// system delays in transmitter and/or receiver components.
	short          first_dopp;

	// Reflectivity data gate size (METERS).
	unsigned short refl_size;

	// Doppler data gate size (METERS).
	unsigned short dopp_size;

	// Number of reflectivity gates.
	unsigned short num_refl_gate;

	// Number of velocity and/or spectrum width data gates.
	unsigned short num_dopp_gate;

	// Sector number within cut.
	unsigned short sector;

	// System gain calibration constant (dB biased).
	float          gain;

	// Reflectivity data pointer (byte # from the start of  digital radar
	// data message header). This pointer locates the beginning of
	// reflectivity data.
	unsigned short refl_ptr;

	// Velocity data pointer (byte # from the start of digital radar data
	// message header). This pointer locates beginning of velocity data.
	unsigned short vel_ptr;

	// Spectrum-width pointer (byte # from the start of  digital radar data
	// message header). This pointer locates beginning of spectrum-width
	// data.
	unsigned short spec_ptr;

	// Doppler velocity resolution.
	//   Value of: 2 = 0.5 m/s
	//             4 = 1.0 
	unsigned short dopp_res;

	// Volume coverage pattern.
	//   Value of: 11 = 16 elev. scans/ 5 mins.
	//             21 = 11 elev. scans/ 6 mins.
	//             31 = 8 elev. scans/ 10 mins.
	//             32 = 7 elev. scans/ 10 mins.
	unsigned short pattern;

	// Unused.  Reserved for V&V Simulator.
	short          vv_sim[4];

	// Reflectivity data pointer for Archive II playback. Archive II
	// playback pointer used exclusively by RDA.
	unsigned short refl_ptr_rda;
	
	// Velocity data pointer for Archive II playback. Archive II playback
	// pointer used exclusively by RDA.
	unsigned short vel_ptr_rda;

	// Spectrum-width data pointer for Archive II playback. Archive II
	// playback pointer used exclusively by RDA.
	unsigned short spec_ptr_rda;

	// Nyquist velocity (scaled: Value/100. = M/S).
	unsigned short nyquist;

	// Atmospheric attenuation factor (scaled: [Value/1000. = dB/KM]).
	unsigned short atten;

	// Threshold parameter for minimum difference in echo power between two
	// resolution volumes for them not to be labeled range ambiguous
	// (i.e.,overlaid) [Value/10. = Watts].
	short          thresh;

	// Unused.
	short          unused0[17];

	/**
	 * Base Data:
	 *
	 * This information includes the three base data moments; reflectivity,
	 * velocity and spectrum width.  Depending on the collection method, up
	 * to three base data moments may exist in this section of the packet.
	 * (For this example, only reflectivity is present.) Base data is coded
	 * and placed in a single byte and is archived in the following format:
	 */
	// A bit confsued by these docs.
	// Note: Need to check size field to get correct size.
	unsigned char data[2300];

	// Reflectivity data (0 - 460 gates) (coded: [((Value-2)/2.)-32. =
	// dBZ], for Value of 0 or 1 see note below).
        //unsigned char refl[460];

	// Doppler velocity data (coded: for doppler velocity resolution of 0.5
	// M/S, [((Value-2)/2.)-63.5 = M/S]; for doppler resolution of 1.0 M/S,
	// [(Value-2)-127.] = M/S], for Value of 0 or 1 see note below), (0 -
	// 92 gates). Starting data location depends on length of the
	// reflectivity field, stop location depends on length of the velocity
	// field. Velocity data is range unambiguous out to 230 KM.
        //unsigned char vel[920];
        
	// Doppler spectrum width (coded: [((Value - 2)/2.)-63.5 = M/S], for
	// Value of 0 or 1 see note below), (0 - 920 gates). Starting data
	// location depends on length of the reflectivity and velocity fields,
	// stop location depends on length of the spectrum width field.
	// Spectrum width is range unambiguous out to 230 KM.
        //unsigned char spec[920];

	// Four bytes of trailer characters referred to the Frame Check
	// Sequence (FCS) follow the data. In cases where the three moments are
	// not all present or the number of gates for each moment have been
	// reduced, the record is padded out to a constant size of 1216
	// halfwords (2432 bytes) following the trailer characters.
	unsigned int fcs;
} __attribute__ ((packed)) level2_packet_t;

#endif
