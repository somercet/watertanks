/* Copyright 2023-2024 somercet
 * Released under GPL2.
 * Author: google mail com at somercet */

#ifndef _xc_search_flags_h_
#define _xc_search_flags_h_

enum xc_search_flags {
	SF_CASE_MATCH = 1,
	SF_BACKWARD = 2,
	SF_HIGHLIGHT = 4,
	SF_FOLLOW = 8,
	SF_REGEXP = 16
};

#endif

